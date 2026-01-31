#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

 
void heap_init(void);

 
void *malloc(size_t size);

 
void free(void *ptr);

#endif  
