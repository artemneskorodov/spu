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

spu_error_t run_command_chai (spu_t *spu);
spu_error_t run_command_push (spu_t *spu);
spu_error_t run_command_add  (spu_t *spu);
spu_error_t run_command_sub  (spu_t *spu);
spu_error_t run_command_mul  (spu_t *spu);
spu_error_t run_command_div  (spu_t *spu);
spu_error_t run_command_out  (spu_t *spu);
spu_error_t run_command_in   (spu_t *spu);
spu_error_t run_command_sqrt (spu_t *spu);
spu_error_t run_command_sin  (spu_t *spu);
spu_error_t run_command_cos  (spu_t *spu);
spu_error_t run_command_dump (spu_t *spu);
spu_error_t run_command_hlt  (spu_t *spu);
spu_error_t run_command_jmp  (spu_t *spu);
spu_error_t run_command_ja   (spu_t *spu);
spu_error_t run_command_jb   (spu_t *spu);
spu_error_t run_command_jae  (spu_t *spu);
spu_error_t run_command_jbe  (spu_t *spu);
spu_error_t run_command_je   (spu_t *spu);
spu_error_t run_command_jne  (spu_t *spu);
spu_error_t run_command_pop  (spu_t *spu);
spu_error_t run_command_call (spu_t *spu);
spu_error_t run_command_ret  (spu_t *spu);
spu_error_t run_command_draw (spu_t *spu);

#endif
