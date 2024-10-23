#ifndef STACK_FACILITIES_H
#define STACK_FACILITIES_H

#include <stdint.h>

#define DEF_CMD_(__cmd, __num, ...) CMD_ ## __cmd = (__num),

enum command_t : uint32_t {
    CMD_unknown = 0x0,
    #include "cringe_commands.h"
};

#undef DEF_CMD_

static const char    *spu_register_names[]      = {"ax", "bx", "cx", "sp",
                                                   "bp", "di", "si", "dx"};
static const size_t   registers_number          = 4;
static const uint32_t immediate_constant_mask   = 0b001;
static const uint32_t register_parameter_mask   = 0b010;
static const uint32_t random_access_memory_mask = 0b100;
static const char    *assembler_name            = "CHTO ZA MASHINA ETOT PROCESSOR";
static const uint32_t assembler_version         = 1;
static const size_t   assembler_name_size       = 64;
static const size_t   spu_drawing_width         = 96;
static const size_t   spu_drawing_height        = 36;

struct program_header_t {
    char     assembler_name[assembler_name_size];
    uint64_t assembler_version;
    size_t   code_size;
};

#endif
