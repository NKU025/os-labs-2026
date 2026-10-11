# 操作系统实验 2026 — Lab1

本仓库为王毅豪、黄泓懿小组的操作系统实验仓库。Lab1 使用 `lab1` 分支，源码和报告分别位于根目录的 `code/`、`report/`。

## 小组成员

| 学号 | 姓名 | 分工 |
| --- | --- | --- |
| 2412509 | 王毅豪 | 独立完成两道练习，负责编写报告 |
| 2413446 | 黄泓懿 | 独立完成两道练习，负责整理材料 |

## Lab1 内容

- [实验报告](report/report.md)：入口汇编分析、GDB 启动跟踪、测试结果和总结。
- [提示词整理](report/prompt.md)：两道练习对应的任务提示词整理版。
- [截图索引](report/images/README.md)：15 张原始截图，报告选用其中 6 张关键截图。
- [构建日志](report/build.log)与 [GDB 日志](report/gdb-session.log)：本次编译与调试的原始记录。

已完成最小内核的编译、运行，以及从复位代码经 OpenSBI 到内核入口的跟踪。Lab1 主要阅读和验证课程原始代码，未新增内核功能。软件环境按成员分别列在报告中。

## 提交结构

```text
lab1 分支
├── code/
│   ├── Makefile
│   ├── kern/
│   ├── libs/
│   └── tools/
├── report/
│   ├── report.md
│   ├── prompt.md
│   ├── build.log
│   ├── gdb-session.log
│   └── images/
├── README.md
└── .gitignore
```

一次实验对应一个 `labx` 分支。`main` 保留早期导入记录，Lab1 的正式材料请查看 `lab1`；不以旧版 `labcodes/lab1/` 目录作为提交入口。

编译、运行和调试在 `code/` 目录中进行。工具链、QEMU 安装目录和编译产物不纳入提交。

## 来源

源码来自课程提供的 `lab1.zip`，保留原有版权声明。[课程指导书](http://8.135.34.58/lab2026/_book/)。
