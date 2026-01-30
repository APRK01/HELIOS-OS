#include "heap.h"
#include "console.h"
#include "pmm.h"

// Simple Linked List Allocator

struct block_header {
  size_t size; // Size of the data block (excluding header)
  int is_free; // 1 if free, 0 if used
  struct block_header *next;
};

// 1MB Heap
#define HEAP_SIZE (1024 * 1024)
static uint8_t heap_memory[HEAP_SIZE];
static struct block_header *head = NULL;

void heap_init(void) {
  head = (struct block_header *)heap_memory;
  head->size = HEAP_SIZE - sizeof(struct block_header);
  head->is_free = 1;
  head->next = NULL;

  console_print("HEAP: Initialized (1MB Static).\n");
}

void *malloc(size_t size) {
  if (size == 0 || head == NULL)
    return NULL;

  // Align size to 8 bytes
  size = (size + 7) & ~7;

  struct block_header *curr = head;
  while (curr) {
    if (curr->is_free && curr->size >= size) {
      // Found a fit. Can we split it?
      if (curr->size >= size + sizeof(struct block_header) + 8) {
        // Split
        struct block_header *new_block =
            (struct block_header *)((uint8_t *)curr +
                                    sizeof(struct block_header) + size);
        new_block->size = curr->size - size - sizeof(struct block_header);
        new_block->is_free = 1;
        new_block->next = curr->next;

        curr->size = size;
        curr->next = new_block;
      }
      curr->is_free = 0;
      return (void *)((uint8_t *)curr + sizeof(struct block_header));
    }
    curr = curr->next;
  }

  return NULL; // Out of memory
}

void free(void *ptr) {
  if (!ptr)
    return;

  struct block_header *block =
      (struct block_header *)((uint8_t *)ptr - sizeof(struct block_header));
  block->is_free = 1;

  // Simple coalesce (merge with next if free)
  struct block_header *curr = head;
  while (curr) {
    if (curr->is_free && curr->next && curr->next->is_free) {
      // Merge curr and curr->next
      curr->size += sizeof(struct block_header) + curr->next->size;
      curr->next = curr->next->next;
      // Don't advance 'curr' in case we can merge again with the NEW next
    } else {
      curr = curr->next;
    }
  }
}
