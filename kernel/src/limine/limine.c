#include <limine/requests.h>
#include <utils/kassert.h>

__attribute__((
    used,
    section(".limine_requests"))) volatile uint64_t limine_base_revision[] =
    LIMINE_BASE_REVISION(6);

__attribute__((used, section(".limine_requests_start"))) volatile uint64_t
    limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;
__attribute__((
    used,
    section(".limine_requests"))) volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

__attribute__((
    used, section(".limine_requests"))) volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST_ID, .revision = 0};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_hhdm_request
    hhdm_request = {.id = LIMINE_HHDM_REQUEST_ID, .revision = 0};

__attribute__((
    used,
    section(
        ".limine_requests"))) volatile struct limine_executable_address_request
    kernel_address_request = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
                              .revision = 0};

__attribute__((used,
               section(".limine_requests"))) volatile struct limine_rsdp_request
    rsdp_request = {.id = LIMINE_RSDP_REQUEST_ID, .revision = 0};

__attribute__((used, section(".limine_requests_end"))) volatile uint64_t
    limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

void request_asserts() {
  kassert(LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision));
  kassert(framebuffer_request.response != NULL &&
          framebuffer_request.response->framebuffer_count >= 1);
  kassert(memmap_request.response != NULL &&
          memmap_request.response->entry_count >= 1);
  kassert(hhdm_request.response != NULL);
  kassert(kernel_address_request.response != NULL &&
          kernel_address_request.response->physical_base != 0 &&
          kernel_address_request.response->virtual_base != 0);
  kassert(rsdp_request.response != NULL &&
          rsdp_request.response->address != NULL);
}
