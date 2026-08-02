#ifndef ACPI_HEADER_H_
#define ACPI_HEADER_H_

#include <utils/printk.h>

typedef struct rsdp {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_phys;
} __attribute__((packed)) rsdp_t;

typedef struct xsdp {
  rsdp_t rsdp;
  uint32_t length;
  uint64_t xsdt_phys;
  uint8_t ext_checksum;
  uint8_t reserved[3];
} __attribute__((packed)) xsdp_t;

typedef struct acpi_header {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed)) acpi_header_t;

typedef struct {
  size_t table_count;
  acpi_header_t **tables;
} acpi_tables_t;

acpi_tables_t *acpi_get_tables();

#endif
