#!/usr/bin/env python3
"""Lab2 MMU demo: automatic replay or interactive, staged GDB stepping."""
import argparse
import os
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--auto', action='store_true', help='continuous replay (default)')
    modes.add_argument('--step', action='store_true', help='interactive GDB: demo / demo-next')
    parser.add_argument('--dtb-pa', type=lambda s: int(s, 0), default=0x82200000,
                        help='DTB physical address for this QEMU virt/128 MiB setup')
    args = parser.parse_args()
    if args.step and not os.isatty(0):
        parser.error('--step requires an interactive terminal')
    if not 0x80000000 <= args.dtb_pa < 0x88000000 or args.dtb_pa % 4096:
        parser.error('--dtb-pa must be a page-aligned address within 128 MiB guest RAM')

    root = Path(__file__).resolve().parents[1]
    home = Path.home() / 'os-lab'
    local = home / 'tools/host-gdb-local'
    gdb = Path(os.environ.get('HOST_GDB', local/'usr/bin/gdb'))
    qemu = Path(os.environ.get('DEBUG_QEMU', home/'build/qemu-4.1.1-lab2-debug/riscv64-softmmu/qemu-system-riscv64'))
    source = Path(os.environ.get('QEMU_SOURCE', home/'src/qemu-4.1.1'))
    for path in (gdb, qemu, source/'target/riscv/cpu_helper.c', source/'accel/tcg/cputlb.c'):
        if not path.is_file():
            parser.error('Missing dependency: ' + str(path))
    env = os.environ.copy()
    env['PATH'] = str(home/'tools/riscv-gcc-10.2.0/bin') + ':' + env.get('PATH', '')
    env['LD_LIBRARY_PATH'] = str(local/'usr/lib/x86_64-linux-gnu') + ':' + env.get('LD_LIBRARY_PATH', '')
    env['LC_ALL'] = 'C.UTF-8'

    # Like Lab1: independent build and unique logs; leave report evidence intact.
    runs = root.parent.parent / 'lab2-demo-runs'
    runs.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='run-', dir=runs))
    build = run/'code'
    shutil.copytree(root, build, ignore=shutil.ignore_patterns('bin', 'obj', '__pycache__', '.git'))
    print('Mode:', 'step' if args.step else 'auto', '\nLogs:', run, flush=True)
    with (run/'build.log').open('w') as output:
        result = subprocess.run(['make', '-j2', 'PMM=best_fit'], cwd=build,
                                env=env, stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        print((run/'build.log').read_text())
        return result.returncode
    print('Build OK. Starting QEMU source debugging; no make grade.', flush=True)
    env.update(LAB2_DEMO_MODE='step' if args.step else 'auto', LAB2_DEMO_RUN=str(run),
               LAB2_QEMU_SOURCE=str(source), LAB2_DTB_PA=str(args.dtb_pa))
    command = [str(gdb), '-nx', '--data-directory='+str(local/'usr/share/gdb')]
    if not args.step:
        command.append('-batch')
    command += ['-x', str(root/'tools/mmu_trace.gdb'), '--args', str(qemu),
                '-machine', 'virt', '-m', '128', '-nographic', '-bios', 'default',
                '-serial', 'file:'+str(run/'qemu.log'), '-monitor', 'none',
                '-device', 'loader,file=bin/ucore.img,addr=0x80200000']
    if args.step:
        process = subprocess.Popen(command, cwd=build, env=env)
        while True:
            try:
                return process.wait()
            except KeyboardInterrupt:
                continue
    process = subprocess.Popen(command, cwd=build, env=env, start_new_session=True)
    try:
        rc = process.wait(timeout=120)
    except (subprocess.TimeoutExpired, KeyboardInterrupt):
        os.killpg(process.pid, signal.SIGKILL)
        process.wait()
        print('Demo interrupted. Logs:', run)
        return 1
    if rc or not (run/'COMPLETE').exists():
        print('Demo did not complete. Inspect:', run/'gdb.log')
        return rc or 1
    print('MMU TRACE COMPLETED. Logs:', run)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
