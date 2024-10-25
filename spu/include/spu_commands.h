#ifndef SPU_COMMANDS_H
#define SPU_COMMANDS_H

#include <stdio.h>
#include <stdint.h>

#include "stack.h"
#include "spu_facilities.h"

enum spu_error_t {
    SPU_SUCCESS         = 0 ,
    SPU_EXIT_SUCCESS    = 1 ,
    SPU_STACK_ERROR     = 2 ,
    SPU_CODE_SIZE_ERROR = 3 ,
    SPU_NULL_POINTER    = 4 ,
    SPU_READING_ERROR   = 5 ,
    SPU_MEMORY_ERROR    = 6 ,
    SPU_UNKNOWN_COMMAND = 7 ,
    SPU_INPUT_ERROR     = 8 ,
    SPU_REGISTER_ERROR  = 9 ,
    SPU_WRONG_VERSION   = 10,
    SPU_WRONG_ASSEMBLER = 11,
    SPU_MEMSET_ERROR    = 12,
    SPU_DUMP_ERROR      = 13,
};

struct spu_t {
    stack_t     *stack;
    command_t   *code;
    address_t    code_size;
    address_t    instruction_pointer;
    argument_t   registers[registers_number];
    argument_t  *random_access_memory;
    argument_t   push_register;
};

spu_error_t run_command_chai     (spu_t    *spu);
spu_error_t run_command_push     (spu_t    *spu);
spu_error_t run_command_add      (spu_t    *spu);
spu_error_t run_command_sub      (spu_t    *spu);
spu_error_t run_command_mul      (spu_t    *spu);
spu_error_t run_command_div      (spu_t    *spu);
spu_error_t run_command_out      (spu_t    *spu);
spu_error_t run_command_in       (spu_t    *spu);
spu_error_t run_command_sqrt     (spu_t    *spu);
spu_error_t run_command_sin      (spu_t    *spu);
spu_error_t run_command_cos      (spu_t    *spu);
spu_error_t run_command_dump     (spu_t    *spu);
spu_error_t run_command_hlt      (spu_t    *spu);
spu_error_t run_command_jmp      (spu_t    *spu);
spu_error_t run_command_ja       (spu_t    *spu);
spu_error_t run_command_jb       (spu_t    *spu);
spu_error_t run_command_jae      (spu_t    *spu);
spu_error_t run_command_jbe      (spu_t    *spu);
spu_error_t run_command_je       (spu_t    *spu);
spu_error_t run_command_jne      (spu_t    *spu);
spu_error_t run_command_pop      (spu_t    *spu);
spu_error_t run_command_call     (spu_t    *spu);
spu_error_t run_command_ret      (spu_t    *spu);
spu_error_t run_command_draw     (spu_t    *spu);
bool        is_command_supported (command_t operation_code);

struct command_handler_t {
    command_t     operation_code;
    spu_error_t (*handler)(spu_t *spu);
};

static const command_handler_t command_handlers[] = {
    {             /*one element is set to zeros*/           },
    {.operation_code = CMD_PUSH, .handler = run_command_push},
    {.operation_code = CMD_ADD , .handler = run_command_add },
    {.operation_code = CMD_SUB , .handler = run_command_sub },
    {.operation_code = CMD_MUL , .handler = run_command_mul },
    {.operation_code = CMD_DIV , .handler = run_command_div },
    {.operation_code = CMD_OUT , .handler = run_command_out },
    {.operation_code = CMD_IN  , .handler = run_command_in  },
    {.operation_code = CMD_SQRT, .handler = run_command_sqrt},
    {.operation_code = CMD_SIN , .handler = run_command_sin },
    {.operation_code = CMD_COS , .handler = run_command_cos },
    {.operation_code = CMD_DUMP, .handler = run_command_dump},
    {.operation_code = CMD_HLT , .handler = run_command_hlt },
    {.operation_code = CMD_JMP , .handler = run_command_jmp },
    {.operation_code = CMD_JA  , .handler = run_command_ja  },
    {.operation_code = CMD_JB  , .handler = run_command_jb  },
    {.operation_code = CMD_JAE , .handler = run_command_jae },
    {.operation_code = CMD_JBE , .handler = run_command_jbe },
    {.operation_code = CMD_JE  , .handler = run_command_je  },
    {.operation_code = CMD_JNE , .handler = run_command_jne },
    {.operation_code = CMD_POP , .handler = run_command_pop },
    {.operation_code = CMD_CALL, .handler = run_command_call},
    {.operation_code = CMD_RET , .handler = run_command_ret },
    {.operation_code = CMD_DRAW, .handler = run_command_draw},
    {.operation_code = CMD_CHAI, .handler = run_command_chai}};

#endif
