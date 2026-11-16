#pragma once

#include <stdbool.h>

typedef struct {
    bool    part_type_set;
    int     part_type; // 0 = MBR; 1 = GPT;
    char    *dev;
    bool    yes;       // skip the confirmation prompt
} config;
