"""Loaded inside host GDB. Shared live plan for automatic and staged demos."""
import os
from pathlib import Path
import gdb

SOURCE = Path(os.environ['LAB2_QEMU_SOURCE'])
RUN = Path(os.environ['LAB2_DEMO_RUN'])
PA = int(os.environ['LAB2_DTB_PA'])
VA = (PA + 0xffffffff40000000) & ((1 << 64) - 1)
CPU = SOURCE/'target/riscv/cpu_helper.c'
TLB = SOURCE/'accel/tcg/cputlb.c'


def line_of(path, snippet):
    matches = [i for i, s in enumerate(path.read_text().splitlines(), 1) if snippet in s]
    if len(matches) != 1:
        raise gdb.GdbError('Expected one QEMU 4.1.1 source anchor: ' + snippet)
    return matches[0]


def value(expression):
    return int(gdb.parse_and_eval(expression))


def require(expression, label):
    if not value(expression):
        raise gdb.GdbError('[FAIL] ' + label)
    print('[PASS] ' + label)


def walk_to(path, destination, function):
    """Actually execute next once per source step, not a run-until shortcut."""
    for _ in range(100):
        frame = gdb.selected_frame()
        sal = frame.find_sal()
        if not sal.symtab or Path(sal.symtab.fullname()).name != path.name or frame.name() != function:
            raise gdb.GdbError('Unexpected frame; restart the demo after changing execution manually.')
        if sal.line == destination:
            return
        yield 'next'
    raise gdb.GdbError('Source-step limit reached; check QEMU source/build consistency.')


def plan():
    lines = TLB.read_text().splitlines()
    load = line_of(TLB, 'load_helper(CPUArchState *env, target_ulong addr, TCGMemOpIdx oi,')
    query = next(i for i in range(load, load + 60) if 'if (!tlb_hit(tlb_addr, addr))' in lines[i-1])
    fill = next(i for i in range(query + 1, query + 12) if 'tlb_fill(env_cpu(env)' in lines[i-1])
    index = line_of(CPU, 'target_ulong idx = (addr >> (PGSHIFT + ptshift))')
    valid = line_of(CPU, 'if (!(pte & PTE_V))')
    physical = line_of(CPU, '*physical = (ppn | (vpn & ((1L << ptshift) - 1)))')

    yield ('stage', '1/4 TLB 查询：定位这次内存读取')
    yield f'set $target = (unsigned long long)0x{VA:x}'
    yield 'tbreak load_helper if addr == $target && !code_read'
    yield 'run'
    yield f'until {TLB}:{query}'
    yield 'list'
    yield 'p/x addr'
    yield 'p mmu_idx'
    yield 'p index'
    yield 'p/x tlb_addr'
    yield 'bt 5'
    yield ('note', '当前停在 TLB 标签比较之前。下一段单步观察主 TLB、victim TLB 未命中后的分支。')

    yield ('stage', '2/4 TLB 未命中：进入页表翻译')
    yield from walk_to(TLB, fill, 'load_helper')
    yield 'tbreak get_physical_address if addr == $target && access_type == 0'
    yield 'continue'
    yield 'bt 7'
    yield 'p/x addr'
    yield 'p/x env->satp'
    yield 'p mmu_idx'
    require('addr == $target && (env->satp >> 60) == 8', 'Target address and Sv39 mode')
    yield f'until {CPU}:{index}'
    yield 'p/x base'
    yield 'p levels'
    yield 'p ptshift'
    yield ('note', '从 satp 得到根页表；下一段逐行计算索引、读取 PTE，并检查叶子项与权限。')

    yield ('stage', '3/4 页表单步：读取 PTE，检查权限，计算物理地址')
    yield from walk_to(CPU, valid, 'get_physical_address')
    for command in ('p i', 'p idx', 'p/x pte_addr', 'p/x pte', 'p/x ppn'):
        yield command
    require('i == 0 && ptshift == 18 && (pte & 0xf) == 0xf', 'Root-level 1 GiB leaf mapping')
    yield ('note', '本次根级 PTE 已是叶子：1 GiB 大页，不再访问下两级。')
    yield from walk_to(CPU, physical, 'get_physical_address')
    yield 'next'
    yield 'p/x *physical'
    require(f'*physical == 0x{PA:x}', 'Translated physical page matches expected DTB address')
    yield ('note', f'实际翻译结果：0x{VA:x} -> 0x{PA:x}。本次地址页内偏移为 0。')

    yield ('stage', '4/4 TLB 回填：核对同一映射')
    yield 'tbreak tlb_set_page if vaddr == ($target & ~0xfffULL)'
    yield 'continue'
    for command in ('p/x vaddr', 'p/x paddr', 'p prot', 'p mmu_idx', 'p size'):
        yield command
    require(f'vaddr == $target && paddr == 0x{PA:x} && size == 4096 && (prot & 1)',
            'TLB fill uses the translated page with read permission')
    yield 'finish'
    yield ('note', '回填完成。size=4096 是本次软件 TLB 缓存粒度，不是前面叶子映射的大小。')


class Demo:
    def __init__(self):
        self.steps = iter(plan())
        self.pending = None
        self.done = False
        self.failed = False

    def peek(self):
        if self.pending is None:
            self.pending = next(self.steps, ('done', ''))
        return self.pending

    def one(self):
        if self.failed:
            raise gdb.GdbError('Demo stopped after an error; quit and restart to retry.')
        if self.done:
            print('演示已完成；输入 quit 退出。')
            return
        try:
            while True:
                item = self.peek()
                self.pending = None
                if isinstance(item, tuple):
                    kind, message = item
                    if kind == 'done':
                        self.done = True
                        (RUN/'COMPLETE').write_text('MMU TRACE COMPLETED\n')
                        print('\nMMU TRACE COMPLETED：四段核心演示完成。quit 退出并结束本次 QEMU。')
                        return
                    print('\n' + ('==== ' + message + ' ====' if kind == 'stage' else message))
                    if kind == 'note':
                        return
                    continue
                print('(gdb) ' + item, flush=True)
                gdb.execute(item)
                return
        except Exception:
            self.failed = True
            raise

    def stage(self):
        self.one()
        while not self.done:
            item = self.peek()
            if isinstance(item, tuple) and item[0] == 'stage':
                print('\n可讲解/截图。demo 继续下一段；demo-next 执行下一条预设命令。')
                return
            self.one()


demo = Demo()


class DemoCommand(gdb.Command):
    def __init__(self, name, single):
        super().__init__(name, gdb.COMMAND_USER)
        self.single = single

    def invoke(self, arg, from_tty):
        if arg.strip():
            raise gdb.GdbError('This command takes no arguments.')
        try:
            demo.one() if self.single else demo.stage()
        except Exception as exc:
            demo.failed = True
            raise gdb.GdbError(str(exc))


DemoCommand('demo', False)
DemoCommand('demo-next', True)
if os.environ['LAB2_DEMO_MODE'] == 'auto':
    try:
        while not demo.done:
            demo.stage()
    except Exception as exc:
        print('[FAIL]', exc)
        gdb.execute('quit 1')
    gdb.execute('kill')
    gdb.execute('quit')
else:
    print('交互模式：demo 按段继续；demo-next 按命令继续（之后按回车可重复）。')
    print('可随时使用 list / p / bt 观察。预设流程中请用 demo-next 推进，以保持执行位置一致。')
    print('自由手动 next/step 也可使用；改变执行路径后，重新启动预设演示更稳妥。')
    gdb.execute('set prompt (lab2-gdb) ')
    demo.stage()
