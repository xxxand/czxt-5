/*
 * Lab05 version2: 带页面置换的虚拟内存管理器
 *
 * 在 version1 的基础上把物理内存缩小为 128 帧, 空闲帧用完后缺页需要
 * 选出牺牲页进行置换。页面置换和 TLB 都采用 LRU 策略。
 *
 * 用法: ./version2 addresses.txt
 */
#include <stdio.h>
#include <stdlib.h>

#define OFFSET_BITS 8
#define PAGE_SIZE   (1 << OFFSET_BITS)
#define FRAME_SIZE  PAGE_SIZE
#define OFFSET_MASK (PAGE_SIZE - 1)
#define ADDR_MASK   0xFFFF  /* 只关心逻辑地址的低 16 位 */
#define NUM_PAGES   256
#define NUM_FRAMES  128
#define TLB_SIZE    16

#define BACKING_STORE_FILE "BACKING_STORE.bin"

struct tlb_entry {
    int page;                /* -1 表示该项无效 */
    int frame;
    unsigned long last_used; /* LRU 时间戳; 无效项为 0, 因而最先被替换 */
};

static signed char memory[NUM_FRAMES * FRAME_SIZE];
static int page_table[NUM_PAGES]; /* 页号 -> 帧号, -1 表示不在内存中 */
static int frame_page[NUM_FRAMES]; /* 帧号 -> 页号, 置换时用来找到牺牲页 */
static unsigned long frame_last_used[NUM_FRAMES];
static struct tlb_entry tlb[TLB_SIZE];
static int next_free_frame;
static unsigned long clock_ticks;

static int tlb_lookup(int page)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page == page) {
            tlb[i].last_used = ++clock_ticks;
            return tlb[i].frame;
        }
    }
    return -1;
}

static void tlb_insert(int page, int frame)
{
    int victim = 0;

    for (int i = 1; i < TLB_SIZE; i++) {
        if (tlb[i].last_used < tlb[victim].last_used)
            victim = i;
    }
    tlb[victim].page = page;
    tlb[victim].frame = frame;
    tlb[victim].last_used = ++clock_ticks;
}

/* 页被换出后, TLB 中指向它的表项必须作废, 否则会命中已被覆盖的旧帧 */
static void tlb_invalidate(int page)
{
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].page == page) {
            tlb[i].page = -1;
            tlb[i].last_used = 0;
        }
    }
}

/* 优先使用空闲帧; 没有空闲帧时换出最久未使用页所在的帧 */
static int allocate_frame(void)
{
    if (next_free_frame < NUM_FRAMES)
        return next_free_frame++;

    int victim = 0;

    for (int f = 1; f < NUM_FRAMES; f++) {
        if (frame_last_used[f] < frame_last_used[victim])
            victim = f;
    }
    page_table[frame_page[victim]] = -1;
    tlb_invalidate(frame_page[victim]);
    return victim;
}

/* 缺页处理: 从后备存储读入一页放进分配到的帧, 并更新页表 */
static int load_page(FILE *backing_store, int page)
{
    int frame = allocate_frame();

    if (fseek(backing_store, page * PAGE_SIZE, SEEK_SET) != 0 ||
        fread(&memory[frame * FRAME_SIZE], 1, FRAME_SIZE, backing_store) != FRAME_SIZE) {
        fprintf(stderr, "cannot read page %d from %s\n", page, BACKING_STORE_FILE);
        exit(EXIT_FAILURE);
    }
    frame_page[frame] = page;
    page_table[page] = frame;
    return frame;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s addresses.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *addresses = fopen(argv[1], "r");
    if (addresses == NULL) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    FILE *backing_store = fopen(BACKING_STORE_FILE, "rb");
    if (backing_store == NULL) {
        perror(BACKING_STORE_FILE);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < NUM_PAGES; i++)
        page_table[i] = -1;
    for (int i = 0; i < TLB_SIZE; i++)
        tlb[i].page = -1;

    int logical;
    int total = 0, page_faults = 0, tlb_hits = 0;

    while (fscanf(addresses, "%d", &logical) == 1) {
        int page = (logical & ADDR_MASK) >> OFFSET_BITS;
        int offset = logical & OFFSET_MASK;

        int frame = tlb_lookup(page);
        if (frame >= 0) {
            tlb_hits++;
        } else {
            frame = page_table[page];
            if (frame < 0) {
                page_faults++;
                frame = load_page(backing_store, page);
            }
            tlb_insert(page, frame);
        }
        /* TLB 命中也要刷新, 否则常用页会被误当作最久未使用而换出 */
        frame_last_used[frame] = ++clock_ticks;

        int physical = frame * FRAME_SIZE + offset;
        printf("Virtual address: %d Physical address: %d Value: %d\n",
               logical, physical, memory[physical]);
        total++;
    }

    printf("Number of Translated Addresses = %d\n", total);
    if (total > 0) {
        printf("Page Faults = %d\n", page_faults);
        printf("Page-Fault Rate = %.2f%%\n", 100.0 * page_faults / total);
        printf("TLB Hits = %d\n", tlb_hits);
        printf("TLB Hit Rate = %.2f%%\n", 100.0 * tlb_hits / total);
    }

    fclose(addresses);
    fclose(backing_store);
    return EXIT_SUCCESS;
}
