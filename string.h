#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

int k_strcmp(const char *s1, const char *s2);
size_t k_strlen(const char *str);
char *k_strcpy(char *dest, const char *src);
void *k_memmove(void *dest, const void *src, size_t n);
void *k_memcpy(void *dest, const void *src, size_t n);
void *k_memset(void *s, int c, size_t n);
int k_strncmp(const char *s1, const char *s2, size_t n);
void itoa(int n, char s[], int base);

#endif
