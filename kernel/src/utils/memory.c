#include <utils/memory.h>

void *memset(void *s, int c, size_t n) {

  unsigned char *p = s;

  while (n > 0 && ((uintptr_t)p & (sizeof(long) - 1)) != 0) {
    *p++ = (unsigned char)c;
    n--;
  }

  unsigned long word_value = (unsigned char)c;
  word_value |= word_value << 8;
  word_value |= word_value << 16;
  if (sizeof(long) == 8) {
    word_value |= (word_value << 16) << 16;
  }

  unsigned long *aligned_p = (unsigned long *)p;
  while (n >= sizeof(long)) {
    *aligned_p++ = word_value;
    n -= sizeof(long);
  }

  p = (unsigned char *)aligned_p;
  while (n > 0) {
    *p++ = (unsigned char)c;
    n--;
  }

  return s;
}

void *memcpy(void *dest, const void *src, size_t n) {

  if (!dest || !src || n == 0) {
    return dest;
  }

  char *d = (char *)dest;
  const char *s = (const char *)src;

  typedef uintptr_t word_t;
  const size_t word_size = sizeof(word_t);
  const size_t word_mask = word_size - 1;

  if (n >= word_size) {
    while (((uintptr_t)d & word_mask) != 0) {
      *d++ = *s++;
      n--;
    }
  }

  word_t *wd = (word_t *)d;
  const word_t *ws = (const word_t *)s;

  while (n >= word_size) {
    *wd++ = *ws++;
    n -= word_size;
  }

  d = (char *)wd;
  s = (const char *)ws;

  while (n--) {
    *d++ = *s++;
  }

  return dest;
}

int formatsize(char *to, size_t bytes) {
  static const char units[] = "BKMGTPE";
  size_t value = bytes;
  size_t unit = 0;
  int outputted = 0;

  while (value >= 1024 && unit < sizeof(units) - 2) {
    value /= 1024;
    unit++;
  }

  if (unit == 0) {
    outputted += sprintf_(to, "%zuB", bytes);
    to += outputted;

    return outputted;
  }

  size_t divisor = 1;
  for (size_t i = 0; i < unit; i++)
    divisor *= 1024;

  size_t whole = bytes / divisor;
  size_t rem = bytes % divisor;
  size_t tenth = (rem * 10) / divisor;

  outputted += sprintf_(to, "%zu.%zu%c", whole, tenth, units[unit]);

  return outputted;
}

size_t strlen(const char *str) {
  const char *p = str;

  while ((uintptr_t)p & (sizeof(uintptr_t) - 1)) {
    if (*p == '\0') {
      return p - str;
    }
    p++;
  }

  const uintptr_t *word_ptr = (const uintptr_t *)p;

  const uintptr_t low_bits = (uintptr_t)-1 / 0xFF;
  const uintptr_t high_bits = low_bits << 7;

  for (;;) {
    uintptr_t v = *word_ptr;

    if ((v - low_bits) & ~v & high_bits) {

      p = (const char *)word_ptr;
      if (p[0] == '\0')
        return p - str;
      if (p[1] == '\0')
        return p + 1 - str;
      if (p[2] == '\0')
        return p + 2 - str;
      if (p[3] == '\0')
        return p + 3 - str;
      if (p[4] == '\0')
        return p + 4 - str;
      if (p[5] == '\0')
        return p + 5 - str;
      if (p[6] == '\0')
        return p + 6 - str;
      return p + 7 - str;
    }
    word_ptr++;
  }
}
