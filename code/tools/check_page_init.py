#!/usr/bin/env python3
"""Host boundary tests for the actual page_init body (no kernel-source edits).

Only DTB input, address mapping, panic and init_memmap are substituted.
The allocator integration is separately tested on RISC-V QEMU.
"""
import pathlib
import re
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
source = (root / 'kern/mm/pmm.c').read_text()
body = source[source.index('static void page_init(void) {'):source.index('/* pmm_init -')]
body = re.sub(r'\bend\b', 'test_end', body)
prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#define PGSIZE 4096UL
#define DRAM_BASE 0x80000000UL
#define KERNEL_BEGIN_PADDR 0x80200000UL
#define PHYSICAL_MEMORY_END 0x88000000UL
static unsigned char arena[16 * 1024 * 1024] __attribute__((aligned(4096)));
__asm__(".globl test_end\n.set test_end, arena + 0x240000");
#define PHYSICAL_MEMORY_OFFSET ((uintptr_t)arena - DRAM_BASE)
#define KERNBASE ((uintptr_t)arena + 0x200000)
#define KERNTOP (KERNBASE + 0x7e00000)
#define ROUNDUP(a,n) (((a)+(n)-1)/(n)*(n))
#define ROUNDDOWN(a,n) ((a)/(n)*(n))
#define PPN(a) ((a)/PGSIZE)
#define PADDR(a) ((uintptr_t)(a)-va_pa_offset)
typedef struct link { struct link *prev, *next; } list_entry_t;
struct Page { int ref; uint64_t flags; unsigned property; list_entry_t page_link; };
static struct Page *pages;
static size_t npage;
static const size_t nbase = DRAM_BASE / PGSIZE;
static uint64_t va_pa_offset, input_base, input_size;
static int calls, panicked;
static size_t registered;
static uintptr_t registered_pa;
static jmp_buf trap;
static uint64_t get_memory_base(void) { return input_base; }
static uint64_t get_memory_size(void) { return input_size; }
#define panic(...) do { panicked=1; longjmp(trap,1); } while(0)
#define cprintf(...) ((void)0)
#define SetPageReserved(p) ((p)->flags |= 1)
static void set_page_ref(struct Page *p, int n) { p->ref=n; }
static void list_init(list_entry_t *l) { l->prev=l->next=l; }
static struct Page *pa2page(uintptr_t pa) {
    assert(PPN(pa)>=nbase && PPN(pa)<npage);
    return pages+(PPN(pa)-nbase);
}
static void init_memmap(struct Page *p, size_t n) {
    assert(n>0); calls++; registered=n;
    registered_pa=DRAM_BASE+(p-pages)*PGSIZE;
    for(size_t i=0;i<n;i++) { assert(p[i].flags&1); p[i].flags=0; }
}
'''
suffix = r'''
static void run(const char *name, uint64_t base, uint64_t size, int expect_panic,
                int expect_free) {
    memset(arena,0,sizeof(arena)); input_base=base; input_size=size;
    calls=panicked=0; registered=0; registered_pa=0; pages=NULL; npage=0;
    if(setjmp(trap)==0) page_init();
    assert(panicked==expect_panic);
    if(!panicked) {
        uint64_t top=base+size;
        if(top>PHYSICAL_MEMORY_END) top=PHYSICAL_MEMORY_END;
        top=ROUNDDOWN(top,PGSIZE);
        assert(va_pa_offset==PHYSICAL_MEMORY_OFFSET && npage==PPN(top));
        assert(PADDR(pages)==0x80240000 && (uintptr_t)pages%PGSIZE==0);
        uintptr_t first=ROUNDUP(PADDR(pages)+(npage-nbase)*sizeof(*pages),PGSIZE);
        assert(calls==expect_free);
        if(expect_free) {
            assert(registered_pa==first && registered==(top-first)/PGSIZE);
        }
        for(size_t i=0;i<npage-nbase;i++) {
            uintptr_t pa=DRAM_BASE+i*PGSIZE;
            assert((pages[i].flags&1)==(pa<first ? 1 : 0));
        }
    } else assert(calls==0);
    printf("[PageInit boundary] %s: PASS\n",name);
}
int main(void) {
    run("64 MiB",DRAM_BASE,0x4000000,0,1);
    run("128 MiB",DRAM_BASE,0x8000000,0,1);
    run("256 MiB capped by framework",DRAM_BASE,0x10000000,0,1);
    run("unaligned end excludes partial page",DRAM_BASE,0x400007b,0,1);
    run("zero size",DRAM_BASE,0,1,0);
    run("invalid base",0,0x8000000,1,0);
    run("base beyond kernel",0x80300000,0x8000000,1,0);
    run("range addition overflow",DRAM_BASE,UINT64_MAX,1,0);
    run("RAM ends before kernel tail",DRAM_BASE,0x210000,1,0);
    run("no room for descriptors",DRAM_BASE,0x240000,1,0);
    uintptr_t top=0x80240000;
    for(int i=0;i<16;i++)
        top=ROUNDUP(0x80240000+(PPN(top)-nbase)*sizeof(struct Page),PGSIZE);
    run("descriptors fit but no complete free page",DRAM_BASE,top-DRAM_BASE,0,0);
    puts("[PageInit boundary] ALL 11 TESTS PASSED");
}
'''
with tempfile.TemporaryDirectory(prefix='lab2-page-init-') as temp:
    path = pathlib.Path(temp)
    (path / 'check.c').write_text(prefix + body + suffix)
    subprocess.run(['gcc', '-std=gnu99', '-Wall', '-Werror', '-O0', '-g',
                    '-fsanitize=address,undefined', str(path/'check.c'),
                    '-o', str(path/'check')], check=True)
    subprocess.run([str(path/'check')], check=True)
