#ifndef ARCHNAME_H_
#define ARCHNAME_H_

// gets the current architecture's name
static inline const char *arch_name() {
#if defined(__x86_64__)
  return "x86-64";
#elif defined(__aarch64__)
  return "aarch64";
#elif defined(__riscv)
  return "riscv64";
#elif defined(__loongarch64)
  return "loongarch64";
#endif
}

#endif
