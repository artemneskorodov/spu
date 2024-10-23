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

#define DEF_CMD_(__cmd, __num, ...) {.command_name = #__cmd, .command_value = CMD_ ## __cmd},
static const command_prototype_t supported_commands[] = {
    #include "cringe_commands.h"
};
#undef DEF_CMD_

#endif
