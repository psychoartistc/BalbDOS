#ifndef TCB_H_
#define TCB_H_

#include <stdint.h>

#define THRDSTATE_READY 0
#define THRDSTATE_RUNNING 1
#define THRDSTATE_BLOCKED 2

typedef struct tcb {
  uint64_t rsp;
  uint64_t id;
  uint8_t state;
} tcb_t;

#endif
