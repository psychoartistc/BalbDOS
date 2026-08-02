#include <allocator/slab.h>
#include <drivers/acpi/header.h>
#include <drivers/api.h>
#include <limine/requests.h>
#include <utils/memory.h>

DRIVER_NAME("ACPI table driver");
DRIVER_DESCRIPTION("System driver that parses ACPI tables");
DRIVER_VERSION("1.0.0");
DRIVER_LICENSE(DRVLICENSE_DEFAULT);

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

  printk("[ OK ] Parsed ACPI tables: [");

  for (size_t i = 0; i < s_tables.table_count; i++) {
    if (i == s_tables.table_count - 1)
      printk("%.4s", s_tables.tables[i]->signature);
    else
      printk("%.4s, ", s_tables.tables[i]->signature);
  }
  printk("]\n\n");
}

static void handle_xsdp(xsdp_t *xsdp) {
  acpi_header_t *xsdt = (acpi_header_t *)p2v(xsdp->xsdt_phys);
  int entries = (xsdt->length - sizeof(acpi_header_t)) / 8;
  if (entries == 0)
    return;

  s_tables.table_count = entries;
  s_tables.tables =
      (acpi_header_t **)kzalloc(sizeof(acpi_header_t *) * entries);
  if (!s_tables.tables) {
    klog_error("Out of memory when allocating ACPI tables");
    return;
  }

  uint64_t *tablePtrs = (uint64_t *)((uintptr_t)xsdt + sizeof(acpi_header_t));
  for (int i = 0; i < entries; i++) {
    acpi_header_t *table = (acpi_header_t *)p2v(tablePtrs[i]);
    s_tables.tables[i] = acpi_clone_table(table);
  }
}

static void handle_rsdp(rsdp_t *rsdp) {
  acpi_header_t *xsdt = (acpi_header_t *)p2v(rsdp->rsdt_phys);

  kassert(acpi_validate_checksum(xsdt));

  int entries = (xsdt->length - sizeof(acpi_header_t)) / 4;
  if (entries == 0)
    return;

  s_tables.table_count = entries;
  s_tables.tables =
      (acpi_header_t **)kzalloc(sizeof(acpi_header_t *) * entries);
  if (!s_tables.tables) {
    klog_error("Out of memory when allocating ACPI tables");
    return;
  }

  uint32_t *tablePtrs = (uint32_t *)((uintptr_t)xsdt + sizeof(acpi_header_t));

  for (int i = 0; i < entries; i++) {
    acpi_header_t *table = (acpi_header_t *)p2v(tablePtrs[i]);
    s_tables.tables[i] = acpi_clone_table(table);
  }
  /*
    printk("\n*** ACPI header info ***\n");
    printk("* Signature: %.4s\n", xsdt->signature);
    printk("* Length: %d\n", xsdt->length);
    printk("* Revision: %d\n", xsdt->revision);
    printk("* Checksum: %d\n", xsdt->checksum);
    printk("* OEM ID: %.6s\n", xsdt->oem_id);
    printk("* OEM table ID: %.8s\n", xsdt->oem_table_id);
    printk("* OEM revision: %d\n", xsdt->oem_revision);
    printk("* Creator id: %d\n", xsdt->creator_id);
    printk("* Creator revision: %d\n", xsdt->creator_revision);
    printk("* Calculated total entries: %d\n\n", entries); */
}

static int acpi_drv_init() {
  rsdp_t *rsdp = (rsdp_t *)rsdp_request.response->address;
  if (rsdp->revision >= 2) {
    xsdp_t *xsdp = (xsdp_t *)rsdp;

    handle_xsdp(xsdp);
  } else
    handle_rsdp(rsdp);

  output_tables();
  pmm_reclaim_acpi_pages();

  return 0;
}

static void acpi_drv_exit() {
  if (s_tables.tables) {
    for (size_t i = 0; i < s_tables.table_count; i++)
      if (s_tables.tables[i])
        kfree(s_tables.tables[i]);
    kfree(s_tables.tables);
  }
}

acpi_tables_t *acpi_get_tables() { return &s_tables; }

driver_exit(acpi_drv_exit);
driver_init(acpi_drv_init);
