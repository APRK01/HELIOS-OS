#ifndef PMM_H
#define PMM_H

#include "limine.h"
#include <stdint.h>

 
void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm);

 
void *pmm_alloc_page(void);

 
void pmm_free_page(void *addr);

 
uint64_t pmm_get_total_memory(void);

#endif  
