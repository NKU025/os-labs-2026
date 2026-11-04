// lab2 function2-3 / function2-4 2412509: 自编断言测试，不改课程测试。
#include <buddy_pmm.h>
#include <slub.h>
#include <stdio.h>
#include <string.h>

static struct Page *held[LAB2_MAX_PAGES];
static void *objects[400];

static size_t hold_all(void) {
    size_t count = 0;
    struct Page *p;
    while ((p = alloc_page()) != NULL) {
        assert(count < LAB2_MAX_PAGES);
        held[count++] = p;
    }
    assert(nr_free_pages() == 0);
    return count;
}

static void release_all(size_t count) {
    for (size_t i = 0; i < count; i++) if (held[i] != NULL) free_page(held[i]);
}

void buddy_self_test(void) {
    size_t initial = nr_free_pages();
    size_t before[BUDDY_MAX_ORDER + 1], after[BUDDY_MAX_ORDER + 1];
    buddy_validate();
    buddy_histogram(before);
    assert(alloc_pages(0) == NULL);
    assert(alloc_pages(LAB2_MAX_PAGES + 1) == NULL);
    const size_t requests[] = {1, 2, 3, 4, 5, 8, 17};
    struct Page *blocks[7];
    size_t used = 0;
    for (unsigned i = 0; i < 7; i++) {
        size_t rounded = 1;
        while (rounded < requests[i]) rounded <<= 1;
        blocks[i] = alloc_pages(requests[i]);
        assert(blocks[i] != NULL && page2ppn(blocks[i]) % rounded == 0);
        memset((void *)(page2pa(blocks[i]) + va_pa_offset), 0x30 + i,
               rounded * PGSIZE);
        used += rounded;
        assert(nr_free_pages() == initial - used);
    }
    for (unsigned i = 0; i < 7; i++) {
        size_t rounded = 1;
        while (rounded < requests[i]) rounded <<= 1;
        unsigned char *ptr = (void *)(page2pa(blocks[i]) + va_pa_offset);
        for (size_t j = 0; j < rounded * PGSIZE; j++) assert(ptr[j] == 0x30 + i);
    }
    for (int i = 6; i >= 0; i--) free_pages(blocks[i], requests[i]);
    buddy_validate();
    cprintf("[Buddy] rounding, alignment, split and data isolation: PASS\n");

    size_t count = hold_all();
    assert(count == initial && alloc_page() == NULL);
    size_t a = 0, b = count, c = count;
    while (a < count && ((held[a] - pages) & 1)) a++;
    assert(a < count);
    size_t peer = (held[a] - pages) ^ 1;
    for (size_t i = 0; i < count; i++) {
        if (i == a) continue;
        if ((size_t)(held[i] - pages) == peer) b = i;
        else if (c == count) c = i;
    }
    assert(b < count && c < count);
    struct Page *pa = held[a], *pb = held[b], *pc = held[c];
    free_page(pa); held[a] = NULL;
    free_page(pc); held[c] = NULL;
    assert(alloc_pages(2) == NULL); // 两张空闲页不是伙伴，不能凑成两页块
    struct Page *x = alloc_page(), *y = alloc_page();
    assert((x == pa && y == pc) || (x == pc && y == pa));
    held[a] = pa; held[c] = pc;
    free_page(pa); held[a] = NULL;
    free_page(pb); held[b] = NULL;
    struct Page *pair = alloc_pages(2);
    assert(pair == (pa < pb ? pa : pb));
    assert(nr_free_pages() == 0);
    free_pages(pair, 2);
    release_all(count);
    buddy_validate();
    buddy_histogram(after);
    assert(nr_free_pages() == initial);
    for (unsigned i = 0; i <= BUDDY_MAX_ORDER; i++) assert(before[i] == after[i]);
    cprintf("[Buddy] exhaustion, non-buddy rejection, coalescing and restoration: PASS\n");
    cprintf("[Buddy] ALL TESTS PASSED (%lu free pages restored)\n", initial);
}

void slub_self_test(void) {
    size_t initial = nr_free_pages();
    assert(kmalloc(0) == NULL && kmalloc((size_t)-1) == NULL);
    kfree(NULL);
    const size_t sizes[] = {1, 16, 17, 31, 32, 33, 64, 65, 127, 128,
                           129, 256, 257, 512, 513, 1024, 1025, 2048,
                           2049, 4096, 9000};
    const unsigned n = sizeof(sizes) / sizeof(sizes[0]);
    for (unsigned i = 0; i < n; i++) {
        objects[i] = kmalloc(sizes[i]);
        assert(objects[i] != NULL && (uintptr_t)objects[i] % 16 == 0);
        memset(objects[i], 0x40 + i, sizes[i]);
    }
    for (unsigned i = 0; i < n; i++) {
        unsigned char *p = objects[i];
        for (size_t j = 0; j < sizes[i]; j++) assert(p[j] == 0x40 + i);
    }
    for (unsigned i = 0; i < n; i += 2) kfree(objects[i]);
    for (unsigned i = 1; i < n; i += 2) kfree(objects[i]);
    assert(nr_free_pages() == initial);
    cprintf("[SLUB] size boundaries, alignment, large objects and data isolation: PASS\n");

    // 单个 slab 中复用槽位，另一个存活对象使整页不会提前归还。
    void *anchor = kmalloc(32), *old = kmalloc(32);
    assert(anchor != NULL && old != NULL);
    kfree(old);
    void *reused = kmalloc(32);
    assert(reused == old);
    kfree(reused);
    kfree(anchor);
    // 300 个 32 字节对象必须跨页。
    for (unsigned i = 0; i < 300; i++) {
        objects[i] = kmalloc(32);
        assert(objects[i] != NULL);
        memset(objects[i], i % 251, 32);
    }
    assert(initial - nr_free_pages() >= 3);
    for (unsigned i = 0; i < 300; i++) {
        unsigned char *p = objects[i];
        for (unsigned j = 0; j < 32; j++) assert(p[j] == i % 251);
    }
    for (unsigned i = 0; i < 300; i += 2) kfree(objects[i]);
    for (unsigned i = 1; i < 300; i += 2) kfree(objects[i]);
    assert(nr_free_pages() == initial);
    cprintf("[SLUB] multi-slab growth, slot reuse and empty-slab reclamation: PASS\n");

    void *keep = kmalloc(64);
    assert(keep != NULL);
    memset(keep, 0x5a, 64);
    size_t count = hold_all();
    assert(kmalloc(2048) == NULL && kmalloc(9000) == NULL);
    void *cached = kmalloc(64); // 无新页时，已有 slab 的空槽仍可用
    assert(cached != NULL);
    for (unsigned i = 0; i < 64; i++) assert(((unsigned char *)keep)[i] == 0x5a);
    kfree(cached);
    kfree(keep);
    release_all(count);
    assert(nr_free_pages() == initial);
    cprintf("[SLUB] out-of-memory handling and cached allocation: PASS\n");
    cprintf("[SLUB] ALL TESTS PASSED (%lu free pages restored)\n", initial);
}
