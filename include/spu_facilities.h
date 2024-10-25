#ifndef STACK_FACILITIES_H
#define STACK_FACILITIES_H

#include <stdint.h>

typedef uint64_t address_t;
typedef double   argument_t;

enum command_t : uint8_t {
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
    CMD_CHAI    = 0x18,
//CHANGE PROCESSOR_FIRST_COMMAND AND PROCESSOR_LAST_COMMAND WHEN CHANGING THIS ENUM!!!
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

static const command_t  processor_first_command   = CMD_PUSH;
static const command_t  processor_last_command    = CMD_CHAI;
//when changing register names array, it is necessary to change formats in compiler.
static const char      *spu_register_names[]      = {"ax", "bx", "cx", "sp",
                                                     "bp", "di", "si", "dx"};
static const address_t  registers_number          = 4;
static const command_t  immediate_constant_mask   = (command_t)0x20;
static const command_t  register_parameter_mask   = (command_t)0x40;
static const command_t  random_access_memory_mask = (command_t)0x80;
static const command_t  operation_code_mask       = (command_t)0x1f;
static const command_t  argument_type_mask        = (command_t)0xe0;
static const address_t  spu_drawing_width         = 96;
static const address_t  spu_drawing_height        = 36;
static const char      *assembler_name            = "CHTO ZA MASHINA ETOT PROCESSOR";
static const uint64_t   assembler_version         = 228;
static const size_t     assembler_name_size       = 64;
static const size_t     random_access_memory_size = 16384;
static const size_t     max_register_name_length  = 3;

#pragma GCC diagnostic pop

struct program_header_t {
    char     assembler_name[assembler_name_size];
    uint64_t assembler_version;
    size_t   code_size;
};

#endif
