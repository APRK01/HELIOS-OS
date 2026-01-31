#include "pmm.h"
#include "console.h"
#include "limine.h"
#include <stddef.h>

static uint64_t total_memory = 0;
static uint64_t usable_memory = 0;
static uint64_t hhdm_offset = 0;
static void *free_list = NULL;

#define PAGE_SIZE 4096

 
struct free_page {
  struct free_page *next;
};

void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm) {
  if (memmap == NULL || memmap->entries == NULL) {
    console_print("PMM: Error - Invalid memory map response\n");
    return;
  }

  hhdm_offset = hhdm;
  console_print("PMM: Parsing Memory Map (HHDM: ");
  console_print_hex(hhdm_offset);
  console_print(")...\n");

  for (uint64_t i = 0; i < memmap->entry_count; i++) {
    struct limine_memmap_entry *entry = memmap->entries[i];

     
    uint64_t end = entry->base + entry->length;
    if (end > total_memory) {
      total_memory = end;
    }

     
    if (entry->type == LIMINE_MEMMAP_USABLE) {
      usable_memory += entry->length;

       
      uint64_t aligned_base = (entry->base + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
       
      uint64_t aligned_end = (entry->base + entry->length) & ~(PAGE_SIZE - 1);

      for (uint64_t addr = aligned_base; addr < aligned_end;
           addr += PAGE_SIZE) {
         
        struct free_page *page = (struct free_page *)(addr + hhdm_offset);
         
         
         
         

         
         
        page->next = free_list;
        free_list = page;
      }
    }
  }

  console_print("PMM: Total RAM detected: ");
  console_print_dec(usable_memory / (1024 * 1024));
  console_print(" MB\n");
  console_print("Scan Complete. PMM Initialized.\n");
}

void *pmm_alloc_page(void) {
  if (free_list == NULL) {
    return NULL;  
  }

  struct free_page *page = free_list;
  free_list = page->next;

   
  return (void *)page;
}

void pmm_free_page(void *addr) {
  if (addr == NULL)
    return;

  struct free_page *page = (struct free_page *)addr;
  page->next = free_list;
  free_list = page;
}

uint64_t pmm_get_total_memory(void) { return usable_memory; }
