#pragma once

#include <acpi/header.h>

#define ACPI_TABLE_XSDT_SIG "XSDT"

typedef struct {
    acpi_header hdr;
    uint64_t    table[];
} __attribute__((packed)) xsdt;