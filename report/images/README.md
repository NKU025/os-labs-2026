# Lab2 测试证据索引

## 报告使用的终端截图

以下图片来自黄泓懿提供的 `lab2报告.zip`，复制时保留原始图片内容。对应其独立工程，原始报告记录验证日期为 2026-10-23。不同工程的空闲页数、指令地址及脚本命名分别记录，未将截图当作当前仓库构建的输出。

| 文件 | 内容 | 压缩包内原图时间 |
| --- | --- | --- |
| teammate-01-boot.png | 默认 Best-Fit 启动及自检 | 131924 |
| teammate-02-grade.png | 课程评分 20/20 | 131956 |
| teammate-03-configurations.png | 多配置检查通过 | 132054 |
| teammate-04-buddy.png | Buddy 主分配器及 SLUB | 132130 |
| teammate-05-kernel-gdb.png | dtb_init 断点与反汇编 | 132245 |
| teammate-06-dual-gdb.png | 真实 DTB 读取及双 GDB 检查 | 132307 |

132326 与 132307 展示的结果基本重复，正文只使用后者。压缩包中未附组员工程源码与完整宿主日志，当前仓库仍以自己的源码和日志为复现依据。

## 保留的日志排版图

以下原有 PNG 是当前仓库真实日志的排版摘录，**不是桌面截图**。未修改其中的结果或地址；正文已使用上面的终端截图，日志排版图保留供对照。

| 图 | 对应原始记录 |
| --- | --- |
| 01-build-grade.png | [构建](../build.log)、[评分](../grade.log)、[六项测试](../best-fit-test.log) |
| 02-page-init.png | [初始化边界测试](../page-init-boundaries.log) |
| 03-buddy-slub.png | [Buddy 128 MiB](../buddy-128M.log) |
| 04-mmu-trace.png | [GDB 完整记录](../mmu-gdb.log) |

六组正常运行日志：

- First-Fit：[64 MiB](../first_fit-64M.log)、[128 MiB](../first_fit-128M.log)
- Best-Fit：[64 MiB](../best_fit-64M.log)、[128 MiB](../best_fit-128M.log)
- Buddy：[64 MiB](../buddy-64M.log)、[128 MiB](../buddy-128M.log)

这些 QEMU 运行在输出通过结果后被 timeout 停止；原始终止信息保留。MMU 调试日志来自宿主 GDB 启动的独立模拟器，不与正常运行日志混用。
