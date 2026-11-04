// lab2 function2-3 2412509
#include <buddy_pmm.h>
#include <stdio.h>
#include <string.h>

static list_entry_t free_bins[BUDDY_MAX_ORDER + 1];
static size_t free_count;
// >=0: 空闲块首的阶数；-1: 非块首；-2-order: 已分配块首。
static signed char state[LAB2_MAX_PAGES];

static unsigned order_for(size_t n) {
    unsigned order = 0;
    while (((size_t)1 << order) < n && order < BUDDY_MAX_ORDER) order++;
    return order;
}

static void add_block(size_t index, unsigned order) {
    struct Page *p = pages + index;
    state[index] = order;
    p->property = (size_t)1 << order;
    SetPageProperty(p);
    list_add(&free_bins[order], &p->page_link);
}

static void remove_block(size_t index) {
    list_del_init(&pages[index].page_link);
    ClearPageProperty(&pages[index]);
    pages[index].property = 0;
    state[index] = -1;
}

static void buddy_init(void) {
    for (unsigned i = 0; i <= BUDDY_MAX_ORDER; i++) list_init(&free_bins[i]);
    memset(state, -1, sizeof(state));
    free_count = 0;
}

static void merge_block(size_t index, unsigned order) {
    while (order < BUDDY_MAX_ORDER) {
        size_t peer = index ^ ((size_t)1 << order);
        if (peer >= npage - nbase || state[peer] != order) break;
        remove_block(peer);
        if (peer < index) index = peer;
        order++;
    }
    add_block(index, order);
}

static void buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0 && npage - nbase <= LAB2_MAX_PAGES);
    assert(base >= pages && n <= npage - nbase - (base - pages));
    for (struct Page *p = base; p < base + n; p++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
        set_page_ref(p, 0);
    }
    free_count += n;
    size_t index = base - pages;
    // 不对齐、非二次幂的初始区间拆成一组对齐的二次幂块。
    while (n > 0) {
        unsigned order = 0;
        while (order < BUDDY_MAX_ORDER) {
            size_t bigger = (size_t)1 << (order + 1);
            if (bigger > n || index % bigger != 0) break;
            order++;
        }
        merge_block(index, order);
        size_t count = (size_t)1 << order;
        index += count;
        n -= count;
    }
}

static struct Page *buddy_alloc_pages(size_t n) {
    if (n == 0 || n > LAB2_MAX_PAGES) return NULL;
    unsigned wanted = order_for(n), order = wanted;
    size_t count = (size_t)1 << wanted;
    if (count > free_count) return NULL;
    while (order <= BUDDY_MAX_ORDER && list_empty(&free_bins[order])) order++;
    if (order > BUDDY_MAX_ORDER) return NULL;
    struct Page *p = le2page(list_next(&free_bins[order]), page_link);
    size_t index = p - pages;
    remove_block(index);
    while (order > wanted) {
        order--;
        add_block(index + ((size_t)1 << order), order);
    }
    state[index] = -2 - (int)wanted;
    free_count -= count;
    return p;
}

static void buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0 && n <= LAB2_MAX_PAGES);
    assert(base >= pages && base < pages + (npage - nbase));
    size_t index = base - pages;
    unsigned order = order_for(n);
    assert(state[index] == -2 - (int)order);
    size_t count = (size_t)1 << order;
    assert(index % count == 0 && count <= npage - nbase - index);
    for (size_t i = 0; i < count; i++) {
        assert(!PageReserved(base + i));
        set_page_ref(base + i, 0);
    }
    state[index] = -1;
    free_count += count;
    merge_block(index, order);
}

static size_t buddy_nr_free_pages(void) { return free_count; }

void buddy_histogram(size_t out[BUDDY_MAX_ORDER + 1]) {
    for (unsigned order = 0; order <= BUDDY_MAX_ORDER; order++) {
        out[order] = 0;
        for (list_entry_t *le = list_next(&free_bins[order]);
             le != &free_bins[order]; le = list_next(le)) out[order]++;
    }
}

void buddy_validate(void) {
    size_t sum = 0;
    for (unsigned order = 0; order <= BUDDY_MAX_ORDER; order++) {
        size_t count = (size_t)1 << order;
        for (list_entry_t *le = list_next(&free_bins[order]);
             le != &free_bins[order]; le = list_next(le)) {
            struct Page *p = le2page(le, page_link);
            size_t index = p - pages;
            assert(index % count == 0 && index + count <= npage - nbase);
            assert(state[index] == order && PageProperty(p));
            assert(p->property == count);
            size_t peer = index ^ count;
            if (order < BUDDY_MAX_ORDER && peer < npage - nbase)
                assert(state[peer] != order); // 不应残留可以合并的伙伴
            for (size_t i = 0; i < count; i++) assert(!PageReserved(p + i));
            sum += count;
            assert(sum <= free_count);
        }
    }
    assert(sum == free_count);
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_self_test,
};
