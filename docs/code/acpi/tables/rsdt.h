#pragma once

#include <acpi/header.h>

#define ACPI_TABLE_RSDT_SIG "RSDT"

typedef struct {
    acpi_header hdr;
    uint32_t    table[];
} __attribute__((packed)) rsdt;