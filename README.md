# Lab05：设计虚拟内存管理器

本项目用 C 语言模拟虚拟内存管理中的地址转换。程序从文件中读取逻辑地址（32 位整数，只取低 16 位），先查 TLB，未命中再查页表，页面不在内存时从后备存储 `BACKING_STORE.bin` 按需调入，最后输出逻辑地址、物理地址以及物理地址处的有符号字节值，并统计缺页率和 TLB 命中率。

- 实验要求：[Lab05.zh.md](Lab05.zh.md)（英文原文：[Lab05.md](Lab05.md)）
- 设计、实验数据与分析：[REPORT.md](REPORT.md)

## 版本说明

| | version1 | version2 |
|---|---|---|
| 源文件 | `version1.c` | `version2.c` |
| 物理内存 | 256 帧（64 KB，与虚拟地址空间等大） | 128 帧（32 KB） |
| 页面置换 | 不需要，缺页时总有空闲帧 | 空闲帧用尽后按 LRU 置换 |
| 页表 | 256 项 | 256 项 |
| TLB | 16 项，LRU 替换 | 16 项，LRU 替换 |

两个版本互相独立，version2 是在 version1 的基础上增加页面置换得到的。另有 `extract.c`，用来对 `1, 256, 32768, 32769, 128, 65534, 33153` 提取页号和页内偏移。

## 文件说明

| 文件 | 说明 |
|---|---|
| `version1.c` | version1 源码 |
| `version2.c` | version2 源码 |
| `extract.c` | 页号与页内偏移提取程序 |
| `Makefile` | 构建与校验 |
| `gen_locality.py` | 生成带局部性的地址序列，用法见 [REPORT.md](REPORT.md) 3.4 节 |
| `addresses.txt` | 测试输入，1000 个逻辑地址 |
| `correct.txt` | `addresses.txt` 在 256 帧下的正确输出 |
| `BACKING_STORE.bin` | 后备存储，65536 字节（256 页 × 256 字节） |
| `Lab05.md`、`Lab05.zh.md` | 实验指导书（英文、中文） |
| `REPORT.md` | 实验报告 |
| `screenshots/` | 实验报告中的运行截图 |
| `CONTRIBUTIONS.md` | 工作分工与 AI 使用说明 |

## 环境要求

- Linux，需要 gcc（支持 C11）、GNU make 和 bash（`make test` 用到 bash 的进程替换）。
- 生成局部性序列（`gen_locality.py`）还需要 python3。
- 已在 Ubuntu 26.04.1 LTS（WSL2）、gcc 15.2.0、GNU Make 4.4.1、Python 3.14.4 下验证，使用 `-std=c11 -Wall -Wextra -O2` 编译无警告。

## 编译

```bash
make          # 生成 extract、version1、version2
make clean    # 删除生成的可执行文件
```

## 运行

```bash
./version1 addresses.txt
./version2 addresses.txt
./extract
```

程序用相对路径打开 `BACKING_STORE.bin`，因此需要在该文件所在目录运行。`version1`、`version2` 必须且只能带一个参数（地址文件）；参数个数不对、文件无法打开或读取页面失败时，会向标准错误输出原因并以状态 1 退出。

## 输出格式

每个逻辑地址输出一行：

```text
Virtual address: 16916 Physical address: 20 Value: 0
```

三项依次是从文件读入的逻辑地址、转换得到的物理地址，以及该物理地址处的有符号字节值（-128 到 127）。所有地址处理完后输出统计信息，下面是 version1 在 `addresses.txt` 上的结果：

```text
Number of Translated Addresses = 1000
Page Faults = 244
Page-Fault Rate = 24.40%
TLB Hits = 55
TLB Hit Rate = 5.50%
```

## 校验

```bash
make test
```

- version1：前 1000 行输出必须与 `correct.txt` 完全一致。
- version2：物理内存只有 128 帧，物理地址会与 `correct.txt` 不同，因此只比较逻辑地址和字节值两列。

两项都通过时依次打印：

```text
version1: output matches correct.txt
version2: virtual addresses and values match correct.txt
```

有差异时 `diff` 会列出差异，`make` 返回非零状态。统计信息接在 1000 行结果之后，手工比较时需要先截取前 1000 行：

```bash
./version1 addresses.txt | head -n 1000 | diff - correct.txt
```

## 设计概要

**地址划分**　逻辑地址只取低 16 位：高 8 位是页号，低 8 位是页内偏移。物理地址 = 帧号 × 256 + 页内偏移。

**地址转换**

1. 用页号查 TLB，命中则直接得到帧号。
2. 未命中则查页表 `page_table[页号]`，值为 -1 表示该页不在内存。
3. 页表中也没有则发生缺页：用 `fseek` 和 `fread` 从 `BACKING_STORE.bin` 读入该页的 256 字节，放进一个帧并更新页表。
4. 把（页号，帧号）写入 TLB，再读出物理地址处的字节。

**TLB**　16 项，全相联。每项带一个 LRU 时间戳，未命中后插入时替换时间戳最小（最久未使用）的项。无效项的时间戳为 0，因此总是最先被替换。

**帧分配**

- version1：按调入顺序依次分配帧 0、1、2……，所以物理地址与 `correct.txt` 一致。
- version2：先使用空闲帧；用尽后选出最久未使用的帧，将原来的页在页表中置为无效，并作废 TLB 中指向该页的表项，再把新页读入这个帧。每次访问（包括 TLB 命中）都会刷新所用帧的 LRU 时间戳。

更详细的设计、实验结果与分析见 [REPORT.md](REPORT.md)。
