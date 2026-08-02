#ifndef MEMORY_H_
#define MEMORY_H_

#include <stddef.h>
#include <stdint.h>

#include <utils/printf.h>

#define CHAR_BIT 8
#define FORMATSIZE_BUFSIZE                                                     \
  ((sizeof(size_t) * CHAR_BIT * 30103 + 99999) / 100000 + 2)

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char *str);

// formats some amount of bytes into a nice looking string
// 2200 becomes 2.2K
// this null terminates the buffer for you
int formatsize(char *to, size_t s);

#endif
