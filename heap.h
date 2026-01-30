#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

// Initialize the heap
void heap_init(void);

// Dynamically allocate memory
void *malloc(size_t size);

// Free allocated memory
void free(void *ptr);

#endif // HEAP_H
