#ifndef LIMINE_REQUESTS_H_
#define LIMINE_REQUESTS_H_

#include <limine/limine.h>

extern volatile uint64_t limine_base_revision[];
extern volatile uint64_t limine_requests_start_marker[];
extern volatile struct limine_framebuffer_request framebuffer_request;
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_executable_address_request kernel_address_request;
extern volatile uint64_t limin_requests_end_marker[];

#endif
