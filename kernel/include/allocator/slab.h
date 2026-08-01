#ifndef SLAB_H_
#define SLAB_H_

#include <allocator/vmm.h>

// fixed size buckets (the buckets must be a power of two)
// a bucket has some pages, divided into slots of equal size
// these should be tracked with a free list, the same way pmm does

// however we also need to find which buckets a pointer that we are going to
// free belongs, we could use some sort of hash map but more simpler is a header
// with the bucket index

// if we want to allocate anything bigger than the larget bucket size,
// we round up to whole pages instead of buckets and return a vmm_alloc result
// we also need to know if THIS was used to allocate and not the bucket approach

#define KMALLOC_MAGIC_USED 0xDEADBEEFu
#define KMALLOC_MAGIC_FREE 0xDEADC0DEu
#define KMALLOC_MAGIC_RAW 0xBAADF00Du

#define KMALLOC_NUM_BUCKETS 8
static const size_t s_bucket_sizes[KMALLOC_NUM_BUCKETS] = {
    16, 32, 64, 128, 256, 512, 1024, 2048};

typedef struct kmalloc_header {
  uint32_t magic;
  uint32_t bucket_index;
  size_t size;
} kmalloc_header_t;

#define KMALLOC_HEADER_SIZE 16

typedef struct kmalloc_bucket {
  size_t chunk_size;

  // points at data
  void *free_list;
} kmalloc_bucket_t;

void allocator_init();

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif
