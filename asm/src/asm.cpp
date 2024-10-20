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

static const char  *default_output_filename = "a.bin";

static asm_error_t parse_flags                    (code_t       *code,
                                                   int           argc,
                                                   const char   *argv[]);
static asm_error_t read_source_code               (code_t       *code);
static asm_error_t write_code                     (code_t       *code);
static asm_error_t destroy_code                   (code_t       *code);

int main(int argc, const char *argv[]) {
    code_t code = {};
    if(parse_flags(&code, argc, argv) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }

    if(read_source_code(&code) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }

    if(compile_code(&code) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }

    if(write_code(&code) != ASM_SUCCESS) {
        destroy_code(&code);
        return EXIT_FAILURE;
    }

    destroy_code(&code);
    return EXIT_SUCCESS;
}

asm_error_t parse_flags(code_t *code, int argc, const char *argv[]) {
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
        if(strcmp(argv[2], "-o") != 0)
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown flag '%s'.\r\n",
                         argv[2]);

        code->input_filename  = argv[1];
        code->output_filename = argv[3];
        return ASM_SUCCESS;
    }

    color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Unexpected amount of flags.\r\n");
    return ASM_FLAGS_ERROR;
}

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

asm_error_t write_code(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    FILE *output_file = fopen(code->output_filename, "wb");
    if(output_file == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while opening output file '%s'.\r\n",
                     code->output_filename);
        return ASM_OPENING_FILE_ERROR;
    }

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

    if(fwrite(code->output_code,
              sizeof(uint64_t),
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

asm_error_t destroy_code(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    _free(code->source_code);
    _free(code->output_code);
    _free(code->labels.labels);
    _free(code->labels.fixup);
    _memory_destroy_log();
    memset(code, 0, sizeof(code_t));
    return ASM_SUCCESS;
}
