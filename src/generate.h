#pragma once

enum {
    GEN_SU = 1 << 0,
    GEN_VU = 1 << 1,
};

uint32_t generate_code(void* code, int inst_count, int* cycle_estimate, int flags);
