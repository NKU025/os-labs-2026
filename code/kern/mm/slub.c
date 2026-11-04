// lab2 function2-4 2412509
// 单核教学版：页内 freelist + 尺寸类缓存；不实现 Linux 的 per-CPU 缓存。
#include <slub.h>
#include <pmm.h>
#include <string.h>

#define CACHE_COUNT 8
#define SLAB_MAGIC 0x534c5542534d414cULL
#define LARGE_MAGIC 0x534c55424c415247ULL
struct cache { size_t size; list_entry_t slabs; };
struct slab {
    uint64_t magic;
    struct cache *cache;
    list_entry_t link;
    void *free;
    size_t used, capacity, npages;
    uint64_t bitmap[4]; // 最小 16 字节对象，一页至多 256 个槽位
};
#define HEADER_SIZE ROUNDUP(sizeof(struct slab), 16)
static struct cache caches[CACHE_COUNT];
static bool ready;

void slub_init(void) {
    assert(!ready); // 初始化后不能在仍有对象时清空缓存
    for (unsigned i = 0; i < CACHE_COUNT; i++) {
        caches[i].size = (size_t)16 << i;
        list_init(&caches[i].slabs);
    }
    ready = 1;
}

static struct slab *new_slab(struct cache *cache) {
    struct Page *p = alloc_page();
    if (p == NULL) return NULL;
    struct slab *s = (struct slab *)(page2pa(p) + va_pa_offset);
    memset(s, 0, HEADER_SIZE);
    s->magic = SLAB_MAGIC;
    s->cache = cache;
    s->npages = 1;
    s->capacity = (PGSIZE - HEADER_SIZE) / cache->size;
    char *start = (char *)s + HEADER_SIZE;
    for (size_t i = s->capacity; i > 0; i--) {
        void *obj = start + (i - 1) * cache->size;
        *(void **)obj = s->free;
        s->free = obj;
    }
    list_add(&cache->slabs, &s->link);
    return s;
}

void *kmalloc(size_t size) {
    assert(ready);
    if (size == 0) return NULL;
    if (size > 2048) {
        // 先限制大小，再加头部和对齐，防止 size_t 加法溢出。
        size_t limit = (npage - nbase) * PGSIZE;
        if (limit < HEADER_SIZE || size > limit - HEADER_SIZE) return NULL;
        size_t n = ROUNDUP(size + HEADER_SIZE, PGSIZE) / PGSIZE;
        struct Page *p = alloc_pages(n);
        if (p == NULL) return NULL;
        struct slab *s = (struct slab *)(page2pa(p) + va_pa_offset);
        memset(s, 0, HEADER_SIZE);
        s->magic = LARGE_MAGIC;
        s->npages = n;
        return (char *)s + HEADER_SIZE;
    }
    unsigned index = 0;
    while (caches[index].size < size) index++;
    struct cache *cache = &caches[index];
    struct slab *s = NULL;
    for (list_entry_t *le = list_next(&cache->slabs); le != &cache->slabs;
         le = list_next(le)) {
        struct slab *candidate = to_struct(le, struct slab, link);
        if (candidate->free != NULL) { s = candidate; break; }
    }
    if (s == NULL && (s = new_slab(cache)) == NULL) return NULL;
    void *obj = s->free;
    s->free = *(void **)obj;
    size_t slot = ((char *)obj - ((char *)s + HEADER_SIZE)) / cache->size;
    assert(!(s->bitmap[slot / 64] & (1ULL << (slot % 64))));
    s->bitmap[slot / 64] |= 1ULL << (slot % 64);
    s->used++;
    return obj;
}

void kfree(void *ptr) {
    if (ptr == NULL) return;
    assert(ready);
    uintptr_t addr = (uintptr_t)ptr;
    assert(addr >= KERNBASE && PADDR(addr) < npage * PGSIZE);
    struct slab *s = (struct slab *)ROUNDDOWN(addr, PGSIZE);
    if (s->magic == LARGE_MAGIC) {
        assert(ptr == (char *)s + HEADER_SIZE);
        size_t n = s->npages;
        s->magic = 0;
        free_pages(pa2page(PADDR(s)), n);
        return;
    }
    assert(s->magic == SLAB_MAGIC);
    struct cache *cache = s->cache;
    assert(cache >= caches && cache < caches + CACHE_COUNT);
    uintptr_t start = (uintptr_t)s + HEADER_SIZE;
    assert(addr >= start && (addr - start) % cache->size == 0);
    size_t slot = (addr - start) / cache->size;
    assert(slot < s->capacity && s->used > 0);
    uint64_t mask = 1ULL << (slot % 64);
    assert(s->bitmap[slot / 64] & mask);
    s->bitmap[slot / 64] &= ~mask;
    *(void **)ptr = s->free;
    s->free = ptr;
    if (--s->used == 0) {
        list_del(&s->link);
        s->magic = 0;
        free_page(pa2page(PADDR(s)));
    }
}
