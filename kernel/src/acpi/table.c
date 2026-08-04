#include <acpi/table.h>
#include <allocator/slab.h>
#include <limine/requests.h>
#include <utils/kassert.h>
#include <utils/memory.h>

static acpi_tables_t s_tables;

static void output_rsdp(rsdp_t *rsdp, int fromXsdt) {
  if (!fromXsdt)
    printk("\n*** RSDP info ***\n");
  else
    printk("\n*** XSDP info ***\n");
  printk("* Signature: %.8s\n", rsdp->signature);
  printk("* Checksum: %d\n", rsdp->checksum);
  printk("* OEM ID: %.6s\n", rsdp->oem_id);
  printk("* Revision: %d\n", rsdp->revision);
  printk("* RSDT physical address: 0x%p\n", rsdp->rsdt_phys);
  if (!fromXsdt)
    printk("\n");
}

static acpi_header_t *acpi_clone_table(acpi_header_t *table) {
  acpi_header_t *copy = (acpi_header_t *)kzalloc(table->length);
  if (!copy) {
    klog_error("Out of memory when cloning ACPI table");
    return NULL;
  }
  memcpy(copy, table, table->length);
  return copy;
}

static int acpi_validate_checksum(acpi_header_t *header) {
  uint8_t sum = 0;
  uint8_t *bytes = (uint8_t *)header;

  for (uint32_t i = 0; i < header->length; i++) {
    sum += bytes[i];
  }

  return sum == 0;
}

static void output_tables() {
  if (!s_tables.tables || s_tables.table_count == 0)
    return;

  printk("[ ");
  fb_puts("OK", 0x008000, fb_get_background_color());
  printk(" ]");
  printk(" Parsed ACPI tables: [");

  for (size_t i = 0; i < s_tables.table_count; i++) {
    if (i == s_tables.table_count - 1)
      printk("%.4s", s_tables.tables[i]->signature);
    else
      printk("%.4s, ", s_tables.tables[i]->signature);
  }
  printk("]\n");
}

static void handle_sdp(rsdp_t *rsdp, int is_xsdp) {
  acpi_header_t *hdr = NULL;
  if (is_xsdp) {
    xsdp_t *xsdp = (xsdp_t *)rsdp;

    hdr = (acpi_header_t *)p2v(xsdp->xsdt_phys);
  } else {
    hdr = (acpi_header_t *)p2v(rsdp->rsdt_phys);
  }

  kassert(acpi_validate_checksum(hdr));

  int entryDivisor = (is_xsdp) ? 8 : 4;
  int entries = (hdr->length - sizeof(acpi_header_t)) / entryDivisor;
  if (entries == 0)
    return;

  s_tables.table_count = entries;
  s_tables.tables =
      (acpi_header_t **)kzalloc(sizeof(acpi_header_t *) * entries);
  if (!s_tables.tables) {
    klog_error("Out of memory when allocating ACPI tables");
    return;
  }

  uint64_t *tablePtrs = (uint64_t *)((uintptr_t)hdr + sizeof(acpi_header_t));
  for (int i = 0; i < entries; i++) {
    acpi_header_t *table = (acpi_header_t *)p2v(tablePtrs[i]);
    s_tables.tables[i] = acpi_clone_table(table);
  }
}

void acpi_init() {
  rsdp_t *rsdp = (rsdp_t *)rsdp_request.response->address;
  handle_sdp(rsdp, rsdp->revision >= 2);
  output_tables();
  pmm_reclaim_acpi_pages();
}

void acpi_cleanup() {
  if (s_tables.tables) {
    for (size_t i = 0; i < s_tables.table_count; i++)
      if (s_tables.tables[i])
        kfree(s_tables.tables[i]);
    kfree(s_tables.tables);
  }
}

acpi_tables_t *acpi_get_tables() { return &s_tables; }
