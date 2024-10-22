#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "custom_assert.h"
#include "utils.h"
#include "stack.h"
#include "colors.h"
#include "spu_commands.h"
#include "spu_facilities.h"
#include "memory.h"

/**
======================================================================================================
     @brief     Initializing size of stack

======================================================================================================
*/
static const size_t stack_init_size = 16;

static spu_error_t init_spu_code    (spu_t      *spu,
                                     const char *file_name);
static spu_error_t run_spu_code     (spu_t      *spu);
static spu_error_t destroy_spu_code (spu_t      *spu);
static spu_error_t run_command      (spu_t      *spu);
static spu_error_t read_file_header (spu_t      *spu,
                                     FILE       *code_file,
                                     const char *file_name);
static spu_error_t read_file_code   (spu_t      *spu,
                                     FILE       *code_file,
                                     const char *file_name);

/**
======================================================================================================
    @brief      Runs SPU

    @details    Initializes SPU code, runs and destroys it

    @param [in] argc                Number of arguments from command line
    @param [in] argv                Arguments from command line

    @return Exit code

======================================================================================================
*/
int main(int argc, const char *argv[]) {
    spu_t spu = {};
    if(argc != 2) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "SPU expected to have one parameter (the name of binary).\r\n");
        return EXIT_FAILURE;
    }
    if(init_spu_code(&spu, argv[1]) != SPU_SUCCESS)
        return EXIT_FAILURE;

    if(run_spu_code(&spu) != SPU_SUCCESS)
        return EXIT_FAILURE;

    if(destroy_spu_code(&spu) != SPU_SUCCESS)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/**
======================================================================================================
    @brief      Initializes code structure

    @details    Reads header from file name,
                compares processor and assembler names,
                compares assembler version.
                Reads the code from file to code structure

    @param [in] spu                 SPU structure
    @param [in] fil_name            Name of binary code file

    @return Error code

======================================================================================================
*/
spu_error_t init_spu_code(spu_t      *spu,
                          const char *file_name) {
    C_ASSERT(spu       != NULL, return SPU_NULL_POINTER );
    C_ASSERT(file_name != NULL, return SPU_READING_ERROR);

    FILE *code_file = fopen(file_name, "rb");
    if(code_file == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'.\r\n",
                     file_name);
        return SPU_READING_ERROR;
    }

    spu_error_t error_code = SPU_SUCCESS;
    if((error_code = read_file_header(spu, code_file, file_name)) != SPU_SUCCESS)
        return error_code;

    if((error_code = read_file_code(spu, code_file, file_name)) != SPU_SUCCESS)
        return error_code;

    fclose(code_file);

    spu->instruction_pointer = 0;
    spu->stack = stack_init(DUMP_INIT("stack.log",
                                      spu->stack,
                                      file_print_double)
                            stack_init_size,
                            sizeof(argument_t));
    if(spu->stack == NULL) {
        free(spu->code);
        return SPU_STACK_ERROR;
    }

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs code

    @details    Runs commands from code array, while functions does not return exit code

    @param [in] spu                 SPU structure

    @return Error code

    @todo Add checking that instruction_pointer is less then code_size

======================================================================================================
*/
spu_error_t run_spu_code(spu_t *spu) {
    C_ASSERT(spu != NULL, return SPU_NULL_POINTER);

    while(true) {
        spu_error_t error_code = run_command(spu);
        if(error_code != SPU_SUCCESS && error_code != SPU_EXIT_SUCCESS) {
            spu->instruction_pointer--;
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Error while running command '0x%llx' on instruction pointer 0x%llx.\r\n"
                         "Error code '0x%x'\r\n",
                         spu->code[spu->instruction_pointer],
                         spu->instruction_pointer,
                         error_code);
            run_command_dump(spu);
            destroy_spu_code(spu);
            return error_code;
        }
        if(error_code == SPU_EXIT_SUCCESS) {
            destroy_spu_code(spu);
            return SPU_EXIT_SUCCESS;
        }
    }
}

/**
======================================================================================================
    @brief      Destroys SPU structure

    @details    Frees code array, destroys stack and sets all spu structure to zeros

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t destroy_spu_code(spu_t *spu) {
    _free(spu->code);
    stack_destroy(&spu->stack);
    memset(spu, 0, sizeof(spu_t));
    _memory_destroy_log();
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs one command

    @details    Reads command as last element in code array, runs particular command function

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command(spu_t *spu) {
    command_t *code_pointer = (command_t *)(spu->code + spu->instruction_pointer);
    command_t operation_code = code_pointer[1];
    spu->instruction_pointer++;

    switch(operation_code) {
        case CMD_PUSH:
            return run_command_push (spu);
        case CMD_ADD:
            return run_command_add  (spu);
        case CMD_SUB:
            return run_command_sub  (spu);
        case CMD_MUL:
            return run_command_mul  (spu);
        case CMD_DIV:
            return run_command_div  (spu);
        case CMD_OUT:
            return run_command_out  (spu);
        case CMD_IN:
            return run_command_in   (spu);
        case CMD_SQRT:
            return run_command_sqrt (spu);
        case CMD_SIN:
            return run_command_sin  (spu);
        case CMD_COS:
            return run_command_cos  (spu);
        case CMD_DUMP:
            return run_command_dump (spu);
        case CMD_HLT:
            return run_command_hlt  (spu);
        case CMD_JMP:
            return run_command_jmp  (spu);
        case CMD_JA:
            return run_command_ja   (spu);
        case CMD_JB:
            return run_command_jb   (spu);
        case CMD_JAE:
            return run_command_jae  (spu);
        case CMD_JBE:
            return run_command_jbe  (spu);
        case CMD_JE:
            return run_command_je   (spu);
        case CMD_JNE:
            return run_command_jne  (spu);
        case CMD_POP:
            return run_command_pop  (spu);
        case CMD_CALL:
            return run_command_call (spu);
        case CMD_RET:
            return run_command_ret  (spu);
        case CMD_DRAW:
            return run_command_draw (spu);
        case CMD_UNKNOWN:
            return SPU_UNKNOWN_COMMAND;
        default:
            return SPU_UNKNOWN_COMMAND;
    }
}

/**
======================================================================================================
    @brief      Reads and checks file header.

    @details    Compares header assembler name and version. Sets code_size in spu_code.

    @param [in] spu                 SPU structure
    @param [in] code_file           Binary file to run
    @param [in] file_name           Name of binary file

    @return Error code

======================================================================================================
*/
spu_error_t read_file_header (spu_t      *spu,
                              FILE       *code_file,
                              const char *file_name) {
    program_header_t header = {};

    if(fread(&header, 1, sizeof(program_header_t), code_file) != sizeof(program_header_t)) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading code from file '%s'.\r\n",
                     file_name);
        return SPU_READING_ERROR;
    }

    if(strcmp(header.assembler_name, assembler_name) != 0) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Program '%s' was compiled with assembler '%s',\r\n"
                     "This processor supports assembler '%s'.\r\n",
                     file_name,
                     header.assembler_name,
                     assembler_name);
        return SPU_WRONG_ASSEMBLER;
    }

    if(header.assembler_version != assembler_version) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "This program was compiled with assembler version %llu\r\n"
                     "And processor supports only %llu.\r\n",
                     header.assembler_version,
                     assembler_version);
        return SPU_WRONG_VERSION;
    }

    spu->code_size = header.code_size;
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Reads code array.

    @details    Reads code. It is expected that read_file_header(...) called before
                and ile size is set to correct value.

    @param [in] spu                 SPU structure
    @param [in] code_file           Binary file to run
    @param [in] file_name           Name of binary file

    @return Error code

======================================================================================================
*/
spu_error_t read_file_code(spu_t      *spu,
                           FILE       *code_file,
                           const char *file_name) {
    spu->code = (uint64_t *)_calloc(spu->code_size, sizeof(uint64_t));
    if(spu->code == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to code array.\r\n");
        fclose(code_file);
        return SPU_MEMORY_ERROR;
    }

    if(fread(spu->code, sizeof(uint64_t), spu->code_size, code_file) != spu->code_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading code from file '%s'.\r\n",
                     file_name);
        fclose(code_file);
        _free(spu->code);
        return SPU_READING_ERROR;
    }
    return SPU_SUCCESS;
}
