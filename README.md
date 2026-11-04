# 操作系统实验 2026 — Lab2

小组成员：2412509-王毅豪、2413446-黄泓懿。

本分支为 lab2，code/ 保存实现后的源码，report/ 保存报告、提示词位置、真实日志和结果图。Lab1 的源码及报告仍在 lab1 分支，未改动。

## 实验内容

- [x] 物理内存探测与 page_init 初始化。
- [x] First-Fit 分析、Best-Fit 实现与接入。
- [x] Buddy 实现及分裂、合并、耗尽、回收测试。
- [x] 简化 SLUB 与三种下层页分配器的组合验证。
- [x] QEMU 4.1.1 的页表查询、软件 TLB 调试。
- [x] 按 Lab1 最终报告格式整理 Lab2 报告。
- [x] 整理各模块任务提示词，Buddy 和 SLUB 分别保留实现与测试两份。
- [x] 补充小组模块分工与报告分工。
- [x] 纳入黄泓懿提供的 6 张终端截图，区分组员独立工程截图与当前仓库日志。

## 阅读入口

- [实验报告](report/report.md)
- [任务提示词](report/prompt.md)
- [测试证据索引](report/images/README.md)
- [复现与调试说明](code/tools/DEBUGGING.md)

## 本次验证（2026-10-26）

11 项初始化边界测试通过；First-Fit、Best-Fit、Buddy 在 64/128 MiB 下自检和 SLUB 均通过；Best-Fit 内部测试 6/6，课程评分 20/20。评分覆盖初始化和 Best-Fit，Buddy/SLUB 另有自编断言测试。

```bash
cd /mnt/d/Courses/OS/os-labs-2026/code
bash tools/verify_lab2.sh
make qemu
```

默认 Best-Fit，使用 make qemu PMM=buddy 切换 Buddy。自动验证脚本最后恢复默认镜像。已单独验证客体虚拟地址 0xffffffffc2200000 翻译到物理地址 0x82200000，调试脚本和路径说明见上面的调试文档。

源码基于老师提供的 lab2/lab2_new；课程测试与 First-Fit 原实现保持不变。工具链、QEMU 二进制和编译产物不纳入仓库。
