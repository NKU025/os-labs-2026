# Lab1 实验截图

本目录保存本次实验的 15 张原始截图。报告正文选用 01、02、03、08、09、10，其余截图保留为补充记录。

| 文件 | 内容 |
| --- | --- |
| 01-build.png | 编译退出状态和生成文件 |
| 02-kernel-boot.png | 正常运行时的内核输出 |
| 03-gdb-reset.png | 初始 PC 与复位处反汇编 |
| 04-reset-step1.png | auipc 设置 t0 |
| 05-reset-step2.png | 设置设备树地址参数 a1 |
| 06-reset-step3.png | 读取 hart 编号 |
| 07-reset-step4.png | 读取 OpenSBI 启动地址 |
| 08-enter-opensbi.png | 跳转进入 OpenSBI |
| 09-kernel-entry.png | 命中内核入口断点 |
| 10-kernel-stack.png | 内核栈指针初始化 |
| 11-enter-kern-init.png | 进入 C 函数 kern_init |
| 12-memset-args.png | 清零调用参数 |
| 13-cprintf-args.png | 打印调用参数 |
| 14-kernel-loop.png | 打印后进入无限循环 |
| 15-debug-boot-output.png | 调试运行时的内核输出 |

文本记录见 [GDB 日志](../gdb-session.log)和[构建日志](../build.log)。
