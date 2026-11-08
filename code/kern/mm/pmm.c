#include <default_pmm.h>
#include <best_fit_pmm.h>
#include <defs.h>
#include <error.h>
#include <memlayout.h>
#include <mmu.h>
#include <pmm.h>
#include <sbi.h>
#include <stdio.h>
#include <string.h>
#include <../sync/sync.h>
#include <riscv.h>
#include <dtb.h>

// virtual address of physical page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;
// the kernel image is mapped at VA=KERNBASE and PA=info.base
uint64_t va_pa_offset;
// memory starts at 0x80000000 in RISC-V
// DRAM_BASE defined in riscv.h as 0x80000000
const size_t nbase = DRAM_BASE / PGSIZE;

// virtual address of boot-time page directory
uintptr_t *satp_virtual = NULL;
// physical address of boot-time page directory
uintptr_t satp_physical;

// physical memory management
const struct pmm_manager *pmm_manager;


static void check_alloc_page(void);

// init_pmm_manager - initialize a pmm_manager instance
static void init_pmm_manager(void) {
    pmm_manager = &default_pmm_manager;
    cprintf("memory management: %s\n", pmm_manager->name);
    pmm_manager->init();
}

// init_memmap - call pmm->init_memmap to build Page struct for free memory
static void init_memmap(struct Page *base, size_t n) {
    pmm_manager->init_memmap(base, n);
}

// alloc_pages - call pmm->alloc_pages to allocate a continuous n*PAGESIZE
// memory
struct Page *alloc_pages(size_t n) {
    struct Page *page = NULL;
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        page = pmm_manager->alloc_pages(n);
    }
    local_intr_restore(intr_flag);
    return page;
}

// free_pages - call pmm->free_pages to free a continuous n*PAGESIZE memory
void free_pages(struct Page *base, size_t n) {
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        pmm_manager->free_pages(base, n);
    }
    local_intr_restore(intr_flag);
}

// nr_free_pages - call pmm->nr_free_pages to get the size (nr*PAGESIZE)
// of current free memory
size_t nr_free_pages(void) {
    size_t ret;
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        ret = pmm_manager->nr_free_pages();
    }
    local_intr_restore(intr_flag);
    return ret;
}

//lab2 function1 2412509
static void page_init(void) {
    // 1. 使用启动汇编已经建立的映射关系；本函数不负责创建页表。
    va_pa_offset = PHYSICAL_MEMORY_OFFSET;
    const uintptr_t kernel_begin_pa = PADDR(KERNBASE);
    uint64_t mem_base = get_memory_base();
    uint64_t mem_size = get_memory_size();

    // 本框架的页号从 DRAM_BASE 算起，内核固定装入 kernel_begin_pa。
    // 设备树给出的单段 RAM 必须覆盖该内核位置，且相加不能溢出。
    if (mem_base < DRAM_BASE || mem_base > kernel_begin_pa ||
        mem_size == 0 || mem_size > (uint64_t)-1 - mem_base) {
        panic("page_init: invalid physical memory range");
    }
    uint64_t mem_end = mem_base + mem_size;  // 右端点不包含在内存范围内
    cprintf("physical memory: [0x%016lx, 0x%016lx), size 0x%lx\n",
            mem_base, mem_end, mem_size);

    // 2. 只管理 RAM 和框架允许范围的交集，末尾不足一页的部分不使用。
    uintptr_t maxpa = PADDR(KERNTOP);
    if (mem_end < maxpa) {
        maxpa = mem_end;
    }
    maxpa = ROUNDDOWN(maxpa, PGSIZE);
    if (maxpa <= kernel_begin_pa) {
        panic("page_init: no memory for the kernel");
    }

    // npage 是物理页号的排他上界，不是 RAM 的页数。
    // pages[0] 对应 DRAM_BASE，故描述符个数为 npage - nbase。
    npage = PPN(maxpa);
    size_t page_count = npage - nbase;

    // 3. 把 Page 数组放在内核末尾之后，数组起点按页向上对齐。
    extern char end[];
    uintptr_t kernel_end_pa = PADDR((uintptr_t)end);
    if (kernel_end_pa < kernel_begin_pa || kernel_end_pa > maxpa) {
        panic("page_init: kernel lies outside usable memory");
    }
    uintptr_t pages_pa = ROUNDUP(kernel_end_pa, PGSIZE);
    // 在写入任何描述符之前确认整个数组放得下，同时避免乘法溢出。
    if (pages_pa > maxpa ||
        page_count > (maxpa - pages_pa) / sizeof(struct Page)) {
        panic("page_init: not enough memory for page descriptors");
    }
    pages = (struct Page *)(pages_pa + va_pa_offset);

    // 4. 先把所有页设为保留，再只开放确定可用的页面。
    for (size_t i = 0; i < page_count; i++) {
        struct Page *p = &pages[i];
        p->flags = 0;
        SetPageReserved(p);
        set_page_ref(p, 0);
        p->property = 0;
        list_init(&p->page_link);
    }

    // 5. 空闲区从描述符数组之后的第一个完整页面开始。
    // 这样 OpenSBI、内核、Page 数组及对齐空隙均不会交给分配器。
    uintptr_t free_begin = ROUNDUP(pages_pa + page_count * sizeof(struct Page),
                                   PGSIZE);
    if (free_begin < maxpa) {
        size_t free_count = (maxpa - free_begin) / PGSIZE;
        init_memmap(pa2page(free_begin), free_count);
        cprintf("free memory: [0x%016lx, 0x%016lx), %lu pages\n",
                free_begin, maxpa, (unsigned long)free_count);
    } else {
        // init_memmap 要求 n > 0，因此没有完整空闲页时不调用它。
        cprintf("page_init: no complete free pages\n");
    }
}

/* pmm_init - initialize the physical memory management */
void pmm_init(void) {
    // We need to alloc/free the physical memory (granularity is 4KB or other size).
    // So a framework of physical memory manager (struct pmm_manager)is defined in pmm.h
    // First we should init a physical memory manager(pmm) based on the framework.
    // Then pmm can alloc/free the physical memory.
    // Now the first_fit/best_fit/worst_fit/buddy_system pmm are available.
    init_pmm_manager();

    // detect physical memory space, reserve already used memory,
    // then use pmm->init_memmap to create free page list
    page_init();

    // use pmm->check to verify the correctness of the alloc/free function in a pmm
    check_alloc_page();

    extern char boot_page_table_sv39[];
    satp_virtual = (pte_t*)boot_page_table_sv39;
    satp_physical = PADDR(satp_virtual);
    cprintf("satp virtual address: 0x%016lx\nsatp physical address: 0x%016lx\n", satp_virtual, satp_physical);
}

static void check_alloc_page(void) {
    pmm_manager->check();
    cprintf("check_alloc_page() succeeded!\n");
}
