#ifndef PMM_H
#define PMM_H

#include "limine.h"
#include <stdint.h>

// Initialize PMM with memory map and HHDM offset
void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm);

// Allocate a physical page (returns virtual address in HHDM)
void *pmm_alloc_page(void);

// Free a physical page (takes virtual address in HHDM)
void pmm_free_page(void *addr);

// Get total system memory size (bytes)
uint64_t pmm_get_total_memory(void);

#endif // PMM_H
