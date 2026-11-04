# Lab2 复现与调试

## 自动验证

在操作系统课程的 WSL Ubuntu 中：

```bash
cd /mnt/d/Courses/OS/os-labs-2026/code
bash tools/verify_lab2.sh
```

脚本执行 11 项 page_init 宿主边界测试、First-Fit / Best-Fit / Buddy 各 64/128 MiB 的 QEMU 运行、课程 make grade，最后恢复默认 Best-Fit 镜像。会更新 report 下日志；保留历史结果时请先复制。依赖课程工具链、QEMU、Python 3、原生 GCC 及 ASan/UBSan 运行库。

单独演示：

```bash
make qemu                 # 默认 Best-Fit + SLUB
make qemu PMM=buddy        # Buddy + SLUB
make qemu PMM=first_fit    # First-Fit + SLUB
```

退出 QEMU：Ctrl+A，松开，再按 X。切换 PMM 自动使对象文件失效，避免误运行旧配置。`make grade` 默认对应 Best-Fit；不要用 `PMM=buddy make grade` 来判断课程的 Best-Fit 测试。

## QEMU 源码调试环境

本机额外构建在用户目录中，未覆盖原 QEMU：

- 源码：`/home/wyh/os-lab/src/qemu-4.1.1`
- 调试构建：`/home/wyh/os-lab/build/qemu-4.1.1-lab2-debug`
- 宿主 GDB：`/home/wyh/os-lab/tools/host-gdb-local/usr/bin/gdb`

调试构建采用：

```bash
mkdir -p ~/os-lab/build/qemu-4.1.1-lab2-debug
cd ~/os-lab/build/qemu-4.1.1-lab2-debug
~/os-lab/src/qemu-4.1.1/configure --target-list=riscv64-softmmu --enable-debug --disable-werror
make -j4
```

不要在原有构建目录运行清理，也不需要 make install。宿主 GDB 用 apt download 下载 gdb 及缺少的动态库，然后以 dpkg-deb -x 解压到独立目录；未执行系统级安装。调试脚本仅为子进程设置 LD_LIBRARY_PATH。

## MMU 核心演示：自动 / 交互两种模式

仅演示指导书中的 QEMU 源码调试主线：TLB 查询、未命中调用路径、页表单步翻译、物理地址与 TLB 回填。没有 make grade、分配器全套测试、Bare 模式对比、内核启动过程或宿主地址计算。

### 交互演示（答辩推荐）

```bash
cd /mnt/d/Courses/OS/os-labs-2026/code
python3 tools/trace_mmu.py --step
```

启动后自动定位到第一段 TLB 查询处，并停在 `(lab2-gdb)` 提示符。可讲解或截图。

| 输入 | 作用 |
| --- | --- |
| `demo` | 执行下一段；若已开始某段，则执行该段的剩余内容，再暂停 |
| `demo-next` | 执行下一条预设命令；进入页表单步区域后，每次执行一次真实的 `next` |
| 回车 | 重复刚刚输入的 GDB 命令；在 `demo-next` 后可连续回车逐条推进 |
| `list` / `p/x 变量` / `bt` | 查看源码、变量、调用栈，不推进执行 |
| `quit` | 退出 GDB，结束本次脚本启动的 QEMU |

推荐：第一段看 TLB 标签；用 `demo` 到第二段；第三段用 `demo-next` 展示几次关键单步，再用 `demo` 完成剩余页表检查；最后用 `demo` 展示回填。需要最短的分段演示时，全程用 `demo`。

`demo-next` 是按预设命令推进，不是每次都执行一条 CPU 指令：打印命令只查看状态，`next` 按宿主 QEMU 的 C 源码行执行，`until` / `continue` 用来定位关键断点。这里调试的是 QEMU 的 C 源码，不是 guest 内核的 RISC-V 指令。

可直接输入原生 `next` / `step` 自由调试；改变执行路径或选中其他栈帧后，建议重新启动预设演示。使用查看命令后，先重新输入 `demo-next`，再用回车重复，避免回车重复了查看命令。Ctrl+C 中断当前运行；需要结束时再输入 `quit`。

### 自动模式

```bash
python3 tools/trace_mmu.py --auto
```

不带参数仍为自动模式，保持原来的启动习惯。两种模式共用相同的执行和校验逻辑；自动模式连续运行并实时输出。

### 四段结果与截图点

1. **TLB 查询**：读取地址、MMU 模式、TLB 索引和缓存标签，查看查询分支。可截图。
2. **未命中路径**：单步经过主 TLB 和 victim TLB 判断，进入 `get_physical_address`；查看调用栈、satp、根页表地址。
3. **页表翻译**：逐行计算索引、读取 PTE、检查叶子类型和权限、计算物理地址。显示 PTE 和最终物理地址时适合截图。
4. **TLB 回填**：确认 `vaddr`、`paddr`、权限和缓存粒度；执行回填后显示 `MMU TRACE COMPLETED`。可截图。

本机 QEMU 4.1.1 virt / 128 MiB 配置的预期结果：

- DTB 虚拟地址 `0xffffffffc2200000` → 物理地址 `0x82200000`。
- 根级索引 `511`，PTE `0x200000cf`，为 1 GiB 叶子映射；本次不继续访问下两级页表。
- 软件 TLB 的本次填充 `size=4096`，与根级叶子映射大小属于不同概念。

这些是当前配置的核查值，实际数据由正在运行的 QEMU 读取；核心结果会做断言检查。QEMU 软件 TLB 与真实硬件 TLB 的区别仍需口头解释，未加入额外的 Bare 运行对比。

### 日志、环境与边界

参考 Lab1 的隔离方式，每次在 `D:\Courses\OS\lab2-demo-runs\run-*` 中复制源码并构建，记录 `build.log`、`gdb.log`、`qemu.log`，完整完成时生成 `COMPLETE`。不覆盖仓库中的镜像、报告日志和截图，也不依赖旧的 `best_fit-128M.log`。内核只执行到本次调试点，QEMU 日志不是分配器完整测试记录。

脚本自动为子进程添加课程编译器路径，不修改系统配置，不占用远程 GDB 端口，不附加到已有 QEMU。`HOST_GDB`、`DEBUG_QEMU`、`QEMU_SOURCE` 可指定对应路径；GDB 数据目录及动态库仍按本机用户目录布局配置。需要支持 Python 的宿主 GDB。

默认 DTB 地址针对上述固定配置；如更换固件或镜像布局，先核实实际 DTB 地址，再用 `--dtb-pa 0x...` 指定。源码断点从本地 QEMU 4.1.1 的语句定位，仍要求源码与调试二进制匹配，非任意版本通用。

## 原始终端截图

报告正文已采用 6 张终端截图，原有日志排版图也保留供对照，具体对应关系见 report/images/README.md。复现当前代码时，可另行记录以下画面：

1. make grade 的两项 OK 和 20/20。
2. make qemu 的管理器名称、内存范围、自检和 SLUB 结果。
3. make qemu PMM=buddy 的 Buddy、SLUB 全部通过及页数恢复。
4. 使用 `trace_mmu.py --step`，截取 TLB 查询、页表项、最终物理地址和回填结果。

截取时包含命令与关键输出，放入 report/images/，再更新报告引用。
