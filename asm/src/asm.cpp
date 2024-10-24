#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#include "spu_facilities.h"
#include "custom_assert.h"
#include "colors.h"
#include "memory.h"
#include "utils.h"
#include "asm.h"
#include "compiler.h"

/**
======================================================================================================
     @brief     Default name of output binary file.

======================================================================================================
*/
static const char *default_output_filename = "a.bin";

//====================================================================================================
//FUNCTIONS PROTOTYPES
//====================================================================================================
static asm_error_t parse_flags      (code_t     *code,
                                     int         argc,
                                     const char *argv[]);
static asm_error_t read_source_code (code_t     *code);
static asm_error_t write_code       (code_t     *code);
static asm_error_t destroy_code     (code_t     *code);
static asm_error_t write_header     (code_t     *code,
                                     FILE       *output_file);

/**
======================================================================================================
    @brief      Runs assembler.

    @details    Parses flags, reads source file, compiles code and writes it to
                the default output file, or in file with name from console input
                while using '-o' flag.

    @param [in] argc                Number of arguments typed in by user.
    @param [in] argv                Arguments from console.

    @return Exit code.

======================================================================================================
*/
int main(int argc, const char *argv[]) {
    code_t code = {};
    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = parse_flags     (&code, argc, argv)) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }
    if((error_code = read_source_code(&code)            ) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }
    if((error_code = compile_code    (&code)            ) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }
    if((error_code = write_code      (&code)            ) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }

    destroy_code(&code);
    return EXIT_SUCCESS;
}

/**
======================================================================================================
    @brief      Parses flags from console.

    @details    There are only two variants:
                1. asm 'source'
                2. asm 'source' -o 'output'
                Default output file name is 'a.bin'

    @param [in] code                Code structure.
    @param [in] argc                Number of arguments typed in by user.
    @param [in] argv                Arguments from console.

    @return Error code.

======================================================================================================
*/
asm_error_t parse_flags(code_t     *code,
                        int         argc,
                        const char *argv[]) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    if(argc <= 1) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "No input files.\r\n");
        return ASM_NO_INPUT_FILES;
    }
    if(argc == 2) {
        code->input_filename  = argv[1];
        code->output_filename = default_output_filename;
        return ASM_SUCCESS;
    }
    if(argc == 4) {
        if(strcmp(argv[2], "-o") != 0) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown flag '%s'.\r\n",
                         argv[2]);
            return ASM_FLAGS_ERROR;
        }

        code->input_filename  = argv[1];
        code->output_filename = argv[3];
        return ASM_SUCCESS;
    }

    color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Unexpected amount of flags.\r\n");
    return ASM_FLAGS_ERROR;
}

/**
======================================================================================================
    @brief      Reads source code text.

    @details    Allocates memory to source code string,
                Reads text from file with name, which is determined by parse_flags(...).

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t read_source_code(code_t *code) {
    C_ASSERT(code                 != NULL, return ASM_NULL_CODE     );
    C_ASSERT(code->input_filename != NULL, return ASM_NO_INPUT_FILES);

    FILE *source_code_file = fopen(code->input_filename, "rb");
    if(source_code_file == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening file '%s'.\r\n",
                     code->input_filename);
        return ASM_OPENING_FILE_ERROR;
    }

    code->source_size = file_size(source_code_file);

    code->source_code = (char *)_calloc(code->source_size, sizeof(char));
    if(code->source_code == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to source code of file '%s'.\r\n",
                     code->input_filename);
        fclose(source_code_file);
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    if(fread(code->source_code,
             sizeof(char),
             code->source_size,
             source_code_file) != code->source_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading source code from '%s'.\r\n",
                     code->input_filename);
        fclose(source_code_file);
        return ASM_READING_ERROR;
    }

    fclose(source_code_file);
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Writes compiled code to output file.

    @details    Function writes assembler header and compiled code to file with name,
                determined by parse_flags(...).

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t write_code(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    FILE *output_file = fopen(code->output_filename, "wb");

    if(output_file == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening output file '%s'.\r\n",
                     code->output_filename);
        return ASM_OPENING_FILE_ERROR;
    }

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = write_header(code, output_file)) != ASM_SUCCESS)
        return error_code;

    if(fwrite(code->output_code,
              sizeof(code_element_t),
              code->output_code_size,
              output_file) != code->output_code_size) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while writing compiled code to file '%s'.\r\n",
                     code->output_filename);
        return ASM_WRITING_FILE_ERROR;
    }

    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Successfully wrote binary code to file '%s'.\r\n",
                 code->output_filename);
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Adds assembler header to binary code.

    @details    Header contains of assembler name, version and number of elements in binary code.
                It is written to the start of file.

    @param [in] code                Code structure.
    @param [in] output_file         File where function will write header.

    @return Error code.

======================================================================================================
*/
asm_error_t write_header(code_t *code,
                         FILE   *output_file) {
    program_header_t header = {
        .assembler_version  = assembler_version,
        .code_size          = code->output_code_size};
    strcpy(header.assembler_name, assembler_name);

    if(fwrite(&header,
              sizeof(program_header_t),
              1,
              output_file) != 1) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while writing compiled code to file '%s'.\r\n",
                     code->output_filename);
        return ASM_WRITING_FILE_ERROR;
    }

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Destroys code structure.

    @details    Frees source_code, output_code, labels and fixups.
                Closes memory dump file.
                Sets code structure memory to zeros.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t destroy_code(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    _free(code->source_code  );
    _free(code->output_code  );
    _free(code->labels.labels);
    _free(code->labels.fixup );
    _memory_destroy_log();
    memset(code, 0, sizeof(code_t));
    return ASM_SUCCESS;
}
