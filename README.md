# 操作系统实验 2026 — Lab2

小组成员：2412509-王毅豪、2413446-黄泓懿。

当前为 lab2 分支，仅导入课程初始框架，尚未实现实验任务。Lab1 的源码、报告和测试记录保留在 lab1 分支。

## 提交目录

- code/：Lab2 源码，编译和运行均在此目录进行。
- report/report.md：实验报告框架，按实际过程补充。
- report/prompt.md：本实验实际使用的提示词。
- report/images/：本实验测试截图。

源码来自本地课程资料 lab2/lab2_new，导入时未修改源码和测试。
[课程指导书](http://8.135.34.58/lab2026/_book/)

## 开始实验

在操作系统课程的 WSL Ubuntu 中执行：

```bash
cd /mnt/d/Courses/OS/os-labs-2026/code
make
```

初始框架的 page_init() 和 Best-Fit 模块尚未实现，首次编译出现相关错误属于待完成项，不是测试通过记录。不要修改框架测试来绕过错误。

实现并成功编译后，用 make qemu 运行；调试时分别在两个终端用 make debug 和 make gdb。退出 QEMU：按 Ctrl+A，松开，再按 X。

本框架提供 make grade；其覆盖范围须结合 tools/grade.sh、tools/inject.py 和测试源码检查，不能替代 Buddy、SLUB 所需的自编测试。

## 任务顺序

- [ ] 实现 page_init()，初始化物理页描述符和空闲区。
- [ ] 阅读 First-Fit，解释初始化、分配、释放和合并过程。
- [ ] 实现并接入 Best-Fit，保持框架的两个测试函数不变。
- [ ] 实现并接入 Buddy，设计测试并说明边界覆盖。
- [ ] 实现并接入简化 SLUB，设计对象分配与释放测试。
- [ ] 整理真实提示词、调试过程、测试截图和报告。

Buddy 和 SLUB 分别记录实现与测试两份提示词。可选 Challenge 按实际完成情况记录。提交前保留任务标记，将相应 YOUR CODE 标记按课程要求替换为学号。

工具链、QEMU、编译产物和其他实验材料不纳入本次提交。

## 初始检查记录（2026-10-24）

- 39 个源码文件与课程提供的初始目录逐一进行 SHA-256 校验，内容一致。
- 已核对 WSL Ubuntu 26.04（x86_64）、RISC-V GCC 10.2.0、RISC-V GDB 10.1、QEMU 4.1.1 和 GNU Make 4.4.1。
- 执行 `make -B -k -j2` 做初始编译检查，make 退出码为 2。错误涉及尚未实现的 `page_init()` 及 Best-Fit 的 `free_list`、`nr_free`，未为绕过这些错误修改源码。
- 尚未启动本次内核，未运行 make grade 或分配器测试；报告中的实验结果仍待填写。
