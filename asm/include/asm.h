#ifndef ASM_H
#define ASM_H

#include <stdio.h>
#include <stdint.h>

#include "spu_facilities.h"
#include "labels.h"
#include "asm_errors.h"

struct code_t {
    const char     *input_filename;
    const char     *output_filename;
    char           *source_code;
    size_t          source_size;
    size_t          source_code_position;
    size_t          source_current_line;
    labels_array_t  labels;
    uint64_t       *output_code;
    size_t          output_code_size;
};

struct command_prototype_t {
    const char *command_name;
    command_t   command_value;
};

static const command_prototype_t supported_commands[] = {
    {.command_name = "push", .command_value = CMD_PUSH},
    {.command_name = "add" , .command_value = CMD_ADD },
    {.command_name = "sub" , .command_value = CMD_SUB },
    {.command_name = "mul" , .command_value = CMD_MUL },
    {.command_name = "div" , .command_value = CMD_DIV },
    {.command_name = "out" , .command_value = CMD_OUT },
    {.command_name = "in"  , .command_value = CMD_IN  },
    {.command_name = "sqrt", .command_value = CMD_SQRT},
    {.command_name = "sin" , .command_value = CMD_SIN },
    {.command_name = "cos" , .command_value = CMD_COS },
    {.command_name = "dump", .command_value = CMD_DUMP},
    {.command_name = "hlt" , .command_value = CMD_HLT },
    {.command_name = "jmp" , .command_value = CMD_JMP },
    {.command_name = "ja"  , .command_value = CMD_JA  },
    {.command_name = "jb"  , .command_value = CMD_JB  },
    {.command_name = "jae" , .command_value = CMD_JAE },
    {.command_name = "jbe" , .command_value = CMD_JBE },
    {.command_name = "je"  , .command_value = CMD_JE  },
    {.command_name = "jne" , .command_value = CMD_JNE },
    {.command_name = "pop" , .command_value = CMD_POP },
    {.command_name = "call", .command_value = CMD_CALL},
    {.command_name = "ret" , .command_value = CMD_RET },
    {.command_name = "draw", .command_value = CMD_DRAW}
};

#endif
