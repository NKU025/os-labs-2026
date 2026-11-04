#ifndef __KERN_MM_BUDDY_PMM_H__
#define __KERN_MM_BUDDY_PMM_H__
#include <pmm.h>
#define BUDDY_MAX_ORDER 15
#define LAB2_MAX_PAGES ((PHYSICAL_MEMORY_END - DRAM_BASE) / PGSIZE)
extern const struct pmm_manager buddy_pmm_manager;
void buddy_validate(void);
void buddy_histogram(size_t out[BUDDY_MAX_ORDER + 1]);
void buddy_self_test(void);
#endif
