SHELL  := /bin/bash
CC     := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2

PROGS := extract version1 version2

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# version1 的输出应与 correct.txt 完全一致 (前 1000 行为逐地址结果, 之后是统计信息)。
# version2 只有 128 帧, 物理地址会不同, 因此只比较逻辑地址和字节值两列。
test: version1 version2
	./version1 addresses.txt | head -n 1000 | diff - correct.txt
	@echo "version1: output matches correct.txt"
	diff <(./version2 addresses.txt | head -n 1000 | cut -d' ' -f3,8) <(cut -d' ' -f3,8 correct.txt)
	@echo "version2: virtual addresses and values match correct.txt"

clean:
	rm -f $(PROGS)

.PHONY: all test clean
