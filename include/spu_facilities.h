#ifndef STACK_FACILITIES_H
#define STACK_FACILITIES_H

#include <stdint.h>

typedef uint32_t argument_type_t;
typedef uint64_t address_t;
typedef double   argument_t;
typedef uint64_t code_element_t;

enum command_t : uint32_t {
    CMD_UNKNOWN = 0x0 ,
    CMD_PUSH    = 0x1 ,
    CMD_ADD     = 0x2 ,
    CMD_SUB     = 0x3 ,
    CMD_MUL     = 0x4 ,
    CMD_DIV     = 0x5 ,
    CMD_OUT     = 0x6 ,
    CMD_IN      = 0x7 ,
    CMD_SQRT    = 0x8 ,
    CMD_SIN     = 0x9 ,
    CMD_COS     = 0xA ,
    CMD_DUMP    = 0xB ,
    CMD_HLT     = 0xC ,
    CMD_JMP     = 0xD ,
    CMD_JA      = 0xE ,
    CMD_JB      = 0xF ,
    CMD_JAE     = 0x10,
    CMD_JBE     = 0x11,
    CMD_JE      = 0x12,
    CMD_JNE     = 0x13,
    CMD_POP     = 0x14,
    CMD_CALL    = 0x15,
    CMD_RET     = 0x16,
    CMD_DRAW    = 0x17,
    CMD_CHAI    = 0x228,
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

static const char           *spu_register_names[]      = {"ax", "bx", "cx", "sp",
                                                          "bp", "di", "si", "dx"};
static const address_t       registers_number          = 4;
static const argument_type_t immediate_constant_mask   = 0b001;
static const argument_type_t register_parameter_mask   = 0b010;
static const argument_type_t random_access_memory_mask = 0b100;
static const address_t       spu_drawing_width         = 96;
static const address_t       spu_drawing_height        = 36;
static const char           *assembler_name            = "CHTO ZA MASHINA ETOT PROCESSOR";
static const uint64_t        assembler_version         = 1;
static const size_t          assembler_name_size       = 64;
static const size_t          random_access_memory_size = 16384;

#pragma GCC diagnostic pop

struct program_header_t {
    char     assembler_name[assembler_name_size];
    uint64_t assembler_version;
    size_t   code_size;
};

#endif
