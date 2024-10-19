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

enum asm_error_t {
    ASM_SUCCESS                 = 0 ,
    ASM_EXIT_SUCCESS            = 1 ,
    ASM_NULL_CODE               = 2 ,
    ASM_NO_INPUT_FILES          = 3 ,
    ASM_FLAGS_ERROR             = 4 ,
    ASM_OPENING_FILE_ERROR      = 5 ,
    ASM_MEMORY_ALLOCATING_ERROR = 6 ,
    ASM_READING_ERROR           = 7 ,
    ASM_READING_COMMAND_ERROR   = 8 ,
    ASM_UNEXPECTED_ERROR        = 9 ,
    ASM_INPUT_ERROR             = 10,
    ASM_SYNTAX_ERROR            = 11,
    ASM_REGISTER_ERROR          = 12,
    ASM_UNEXPECTED_PARAMETER    = 13,
    ASM_LABEL_ERROR             = 14,
    ASM_WRITING_FILE_ERROR      = 15,
};

static const char  *default_output_filename = "a.bin";
static const size_t max_label_name_size     = 32;
static const size_t max_command_length      = 32;
static const size_t labels_init_size        = 32;
static const size_t fixups_init_size        = 32;

struct label_t {
    char        label_name[max_label_name_size];
    size_t      label_ip;
    bool        is_defined;
};

struct fixup_t {
    size_t      label_number;
    size_t      code_index;
};

struct code_t {
    const char *input_filename;
    const char *output_filename;
    char       *source_code;
    size_t      source_size;
    size_t      source_code_position;
    size_t      source_current_line;
    label_t    *labels;
    size_t      labels_size;
    size_t      labels_number;
    fixup_t    *fixup;
    size_t      fixup_size;
    size_t      fixup_number;
    uint64_t  *output_code;
    size_t      output_code_size;
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
};

static asm_error_t parse_flags                    (code_t       *code,
                                                   int           argc,
                                                   const char   *argv[]);
static asm_error_t read_source_code               (code_t       *code);
static asm_error_t write_code                     (code_t       *code);
static asm_error_t compile_code                   (code_t       *code);
static asm_error_t destroy_code                   (code_t       *code);

static asm_error_t code_add_label                 (code_t       *code,
                                                   char         *label,
                                                   size_t        instruction_pointer,
                                                   bool          is_defined);
static asm_error_t code_add_command               (code_t       *code,
                                                   char         *command);
static asm_error_t read_next_command              (code_t       *code,
                                                   char         *output);
static asm_error_t allocate_code_structure_memory (code_t       *code);
static asm_error_t code_clean_buffer              (code_t       *code);
static bool        is_label                       (char         *command);
static command_t   get_command_value              (char         *command);
static asm_error_t add_command_arguments          (code_t       *code,
                                                   command_t     command);
static asm_error_t add_arguments_push             (code_t       *code);
static asm_error_t add_arguments_pop              (code_t       *code);
static asm_error_t add_arguments_jmp              (code_t       *code);
static asm_error_t add_arguments_call             (code_t       *code);
static size_t      get_register_number            (char         *register_name);
static asm_error_t get_label_instruction_pointer  (code_t       *code,
                                                   char         *label_name,
                                                   size_t       *output);
static asm_error_t add_fix_up                     (code_t       *code,
                                                   size_t        label_number);
static asm_error_t check_labels_size              (code_t       *code);
static asm_error_t check_fixup_size               (code_t       *code);
static asm_error_t do_fixups                      (code_t       *code);
static asm_error_t try_add_constant_argument      (code_t       *code,
                                                   uint32_t     *argument_type);
static asm_error_t try_add_memory_mask            (code_t       *code,
                                                   uint32_t     *argument_type);
static asm_error_t try_add_register_argument      (code_t       *code,
                                                   uint32_t *argument_type);

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

asm_error_t compile_code(code_t *code) {
    C_ASSERT(code              != NULL, return ASM_NULL_CODE  );
    C_ASSERT(code->source_code != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = allocate_code_structure_memory(code)) != ASM_SUCCESS)
        return error_code;

    while(code->source_code_position < code->source_size) {
        char command[max_command_length] = {};

        if((error_code = read_next_command(code, command)) != ASM_SUCCESS)
            return error_code;

        if(is_label(command)) {
            if((error_code = code_add_label(code,
                                            command,
                                            code->output_code_size,
                                            true)) != ASM_SUCCESS)
                return error_code;

            continue;
        }

        if((error_code = code_add_command(code, command)) != ASM_SUCCESS)
            return error_code;

        if((error_code = code_clean_buffer(code)) != ASM_SUCCESS)
            return error_code;

        code->source_current_line++;
    }

    if((error_code = do_fixups(code)) != ASM_SUCCESS) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while cleaning labels fixups.\r\n");
        return error_code;
    }

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
    _free(code->labels);
    _free(code->fixup);
    memset(code, 0, sizeof(code_t));
    return ASM_SUCCESS;
}

asm_error_t read_next_command(code_t *code, char *output) {
    C_ASSERT(code   != NULL, return ASM_NULL_CODE  );
    C_ASSERT(output != NULL, return ASM_INPUT_ERROR);

    size_t index = 0;
    while(true) {
        if(index == max_command_length) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Syntax error on line %llu, maximum command length is %llu.\r\n",
                         code->source_current_line,
                         max_command_length);
            return ASM_SYNTAX_ERROR;
        }

        char last_symbol = code->source_code[code->source_code_position + index];
        if(last_symbol == '\n' ||
           last_symbol == '\r' ||
           last_symbol == '\0' ||
           last_symbol == ' '  ||
           last_symbol == ';') {
            break;
        }

        if(!isalpha(last_symbol) &&
           !isdigit(last_symbol) &&
           last_symbol != ':') {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unexpected ASCII symbol in %s:%llu, ascii code: %d.\r\n",
                         code->input_filename,
                         code->source_current_line,
                         last_symbol);
            return ASM_SYNTAX_ERROR;
        }

        output[index] = code->source_code[code->source_code_position + index];
        index++;
    }

    code->source_code_position += index;
    while(!isgraph(code->source_code[code->source_code_position]) &&
          code->source_code[code->source_code_position] != '\0')
        code->source_code_position++;

    return ASM_SUCCESS;
}

asm_error_t allocate_code_structure_memory(code_t *code) {
    C_ASSERT(code              != NULL, return ASM_NULL_CODE  );
    C_ASSERT(code->source_code != NULL, return ASM_INPUT_ERROR);

    code->output_code = (uint64_t *)_calloc(code->source_size, sizeof(uint64_t));
    if(code->output_code == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to exit code.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    code->labels_size = labels_init_size;
    code->fixup_size  = fixups_init_size;

    code->fixup = (fixup_t *)_calloc(fixups_init_size, sizeof(fixup_t));
    if(code->fixup == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to fixup's.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    code->labels = (label_t *)_calloc(labels_init_size, sizeof(label_t));
    if(code->labels == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to labels.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    code->fixup_number        = 0;
    code->labels_number       = 0;
    code->source_current_line = 1;
    return ASM_SUCCESS;
}

command_t get_command_value(char *command_name) {
    C_ASSERT(command_name != NULL, return CMD_UNKNOWN);

    size_t commands_number = sizeof(supported_commands) / sizeof(supported_commands[0]);
    for(size_t index = 0; index < commands_number; index++)
        if(strcmp(command_name, supported_commands[index].command_name) == 0)
            return supported_commands[index].command_value;
    return CMD_UNKNOWN;
}

asm_error_t code_add_command(code_t *code, char *command) {
    C_ASSERT(code    != NULL, return ASM_NULL_CODE  );
    C_ASSERT(command != NULL, return ASM_INPUT_ERROR);

    command_t command_value = get_command_value(command);
    if(command_value == CMD_UNKNOWN) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Unknown command '%s' on %s:%llu",
                     command,
                     code->input_filename,
                     code->source_current_line);
        return ASM_SYNTAX_ERROR;
    }

    command_t *code_pointer = (command_t *)(code->output_code + code->output_code_size);
    code_pointer[1] = command_value;
    code->output_code_size++;
    return add_command_arguments(code, command_value);
}

asm_error_t add_command_arguments(code_t *code, command_t command) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    if(command == CMD_PUSH)
        return add_arguments_push(code);

    if(command == CMD_POP)
        return add_arguments_pop(code);

    if(command == CMD_JMP ||
       command == CMD_JA  ||
       command == CMD_JB  ||
       command == CMD_JAE ||
       command == CMD_JBE ||
       command == CMD_JE  ||
       command == CMD_JNE)
        return add_arguments_jmp(code);

    if(command == CMD_CALL)
        return add_arguments_call(code);

    return ASM_SUCCESS;
}

asm_error_t add_arguments_push(code_t *code) {
    uint32_t *argument_type = (uint32_t *)(code->output_code + code->output_code_size - 1);
    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = try_add_memory_mask(code, argument_type)) != ASM_SUCCESS)
        return error_code;

    if((error_code = try_add_constant_argument(code, argument_type)) != ASM_SUCCESS)
        return error_code;

    if((error_code = try_add_register_argument(code, argument_type)) != ASM_SUCCESS)
        return error_code;

    while(code->source_code[code->source_code_position] != '\n' &&
          code->source_code[code->source_code_position] != '\0')
        code->source_code_position++;

    if(code->source_code[code->source_code_position] == '\n')
        code->source_code_position++;

    return ASM_SUCCESS;
}

asm_error_t add_arguments_pop(code_t *code) {
    uint32_t *argument_type = (uint32_t *)(code->output_code + code->output_code_size - 1);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = try_add_memory_mask(code, argument_type)) != ASM_SUCCESS)
        return error_code;

    if((error_code = try_add_constant_argument(code, argument_type)) != ASM_SUCCESS)
        return error_code;

    if((error_code = try_add_register_argument(code, argument_type)) != ASM_SUCCESS)
        return error_code;

    if(!(*argument_type & random_access_memory_mask) &&
       *argument_type & immediate_constant_mask)
        return ASM_UNEXPECTED_PARAMETER;

    while(code->source_code[code->source_code_position] != '\n' &&
          code->source_code[code->source_code_position] != '\0')
        code->source_code_position++;

    if(code->source_code[code->source_code_position] == '\n')
        code->source_code_position++;

    return ASM_SUCCESS;
}

//TODO //TODO //TODO //TODO //TODO //TODO //TODO //FIXME

asm_error_t try_add_constant_argument(code_t *code, uint32_t *argument_type) {
    char *source_pointer = code->source_code + code->source_code_position;

    while(*source_pointer != '\r' &&
          *source_pointer != '\n' &&
          *source_pointer != '\0' &&
          !isdigit(*source_pointer))
        source_pointer++;

    if(!isdigit(*source_pointer))
        return ASM_SUCCESS;

    *argument_type ^= immediate_constant_mask;

    //if [reg + constant] or [constant] we try to read as address in RAM
    if(*argument_type & random_access_memory_mask) {
        size_t parameter = 0;
        if(sscanf(source_pointer, "%llu", &parameter) != 1)
            return ASM_UNEXPECTED_PARAMETER;

        *(size_t *)(code->output_code + code->output_code_size++) = parameter;
        return ASM_SUCCESS;
    }

    //if constant occurs without RAM we read it as double
    else {
        double parameter = 0;
        if(sscanf(source_pointer, "%lg", &parameter) != 1)
            return ASM_UNEXPECTED_PARAMETER;

        *(double *)(code->output_code + code->output_code_size) = parameter;
        code->output_code_size++;
        return ASM_SUCCESS;
    }
}

asm_error_t try_add_register_argument(code_t *code, uint32_t *argument_type) {
    char *source_pointer = code->source_code + code->source_code_position;

    while(!isgraph(*source_pointer))
        source_pointer++;

    if(*source_pointer == '\r' ||
       *source_pointer == '\n' ||
       *source_pointer == '\0')
        return ASM_UNEXPECTED_PARAMETER;

    if(*source_pointer == '[')
        source_pointer++;

    if(isdigit(*source_pointer))
        return ASM_SUCCESS;

    char register_name[3] = {source_pointer[0], source_pointer[1], '\0'};

    size_t register_number = get_register_number(register_name);
    if(register_number < 1 || register_number > registers_number)
        return ASM_REGISTER_ERROR;

    *argument_type ^= register_parameter_mask;
    *(size_t *)(code->output_code + code->output_code_size) = register_number;
    code->output_code_size++;

    return ASM_SUCCESS;
}

asm_error_t try_add_memory_mask(code_t *code, uint32_t *argument_type) {
    char *source_pointer = code->source_code + code->source_code_position;

    while(!isgraph(*source_pointer))
        source_pointer++;

    if(*source_pointer == '\r' ||
       *source_pointer == '\n' ||
       *source_pointer == '\0')
        return ASM_UNEXPECTED_PARAMETER;

    if(*source_pointer != '[')
        return ASM_SUCCESS;

    while(*source_pointer != ']'  &&
          *source_pointer != '\r' &&
          *source_pointer != '\n' &&
          *source_pointer != '\0')
        source_pointer++;

    if(*source_pointer != ']')
        return ASM_UNEXPECTED_PARAMETER;

    *argument_type ^= random_access_memory_mask;
    return ASM_SUCCESS;
}

asm_error_t add_arguments_jmp(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    size_t parameter = 0;
    int read_symbols = 0;
    if(sscanf(code->source_code +
              code->source_code_position,
              "%llu%n",
              &parameter,
              &read_symbols) == 1) {
        *(size_t *)(code->output_code + code->output_code_size) = parameter;
        code->output_code_size++;
        code->source_code_position += read_symbols;
        return ASM_SUCCESS;
    }

    char label[max_label_name_size] = {};
    if(sscanf(code->source_code +
              code->source_code_position,
              "%s%n",
              label,
              &read_symbols) == 1) {
        asm_error_t error_code = get_label_instruction_pointer(code,
                                                               label,
                                                               &parameter);
        if(error_code != ASM_SUCCESS)
            return error_code;

        *(size_t *)(code->output_code + code->output_code_size) = parameter;
        code->output_code_size++;
        code->source_code_position += read_symbols;
        return ASM_SUCCESS;
    }

    color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Unexpected parameter on %s:%llu\r\n",
                 code->input_filename,
                 code->source_current_line);
    return ASM_UNEXPECTED_PARAMETER;
}

asm_error_t add_arguments_call(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    int read_symbols = 0;
    size_t parameter = 0;
    char label[max_label_name_size] = {};
    if(sscanf(code->source_code +
              code->source_code_position,
              "%s%n",
              label,
              &read_symbols) == 1) {
        asm_error_t error_code = get_label_instruction_pointer(code,
                                                               label,
                                                               &parameter);
        if(error_code != ASM_SUCCESS)
            return error_code;

        *(size_t *)(code->output_code + code->output_code_size) = parameter;
        code->output_code_size++;
        code->source_code_position += read_symbols;
        return ASM_SUCCESS;
    }

    color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Unexpected parameter on %s:%llu\r\n",
                 code->input_filename,
                 code->source_current_line);
    return ASM_UNEXPECTED_PARAMETER;
}

size_t get_register_number(char *register_name) {
    C_ASSERT(register_name != NULL, return ASM_INPUT_ERROR);
    for(size_t i = 0; i < registers_number; i++) {
        if(strcmp(spu_register_names[i], register_name) == 0)
            return i + 1;
    }
    return 0;
}

asm_error_t get_label_instruction_pointer(code_t *code,
                                          char   *label_name,
                                          size_t *output) {
    C_ASSERT(code       != NULL, return ASM_NULL_CODE  );
    C_ASSERT(label_name != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(output     != NULL, return ASM_INPUT_ERROR);

    for(size_t index = 0; index < code->labels_number; index++) {
        if(strcmp(code->labels[index].label_name, label_name) == 0) {
            if(code->labels[index].is_defined) {
                *output = code->labels[index].label_ip;
                return ASM_SUCCESS;
            }
            else {
                asm_error_t error_code = add_fix_up(code, index);
                if(error_code != ASM_SUCCESS) {
                    return error_code;
                }
                return ASM_SUCCESS;
            }
        }
    }

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = code_add_label(code, label_name, 0, false)) != ASM_SUCCESS)
        return error_code;

    if((error_code = add_fix_up(code, code->labels_number)) != ASM_SUCCESS)
        return error_code;

    return ASM_SUCCESS;
}

asm_error_t code_add_label(code_t *code,
                           char   *label,
                           size_t  instruction_pointer,
                           bool    is_defined) {
    C_ASSERT(code  != NULL, return ASM_NULL_CODE  );
    C_ASSERT(label != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = check_labels_size(code)) != ASM_SUCCESS)
        return error_code;

    label_t *label_array_pointer = code->labels +
                                   code->labels_number;
    if(strcpy(label_array_pointer->label_name,
              label) != label_array_pointer->label_name)
        return ASM_LABEL_ERROR;

    label_array_pointer->is_defined = is_defined;
    label_array_pointer->label_ip   = instruction_pointer;
    code->labels_number++;
    return ASM_SUCCESS;
}

asm_error_t check_labels_size(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    if(code->labels_number < code->labels_size)
        return ASM_SUCCESS;

    label_t *new_labels_array = (label_t *)_recalloc(code->labels,
                                                     code->labels_size,
                                                     code->labels_size +
                                                     labels_init_size,
                                                     sizeof(label_t));
    if(new_labels_array == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reallocating memory to labels.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    code->labels      =  new_labels_array;
    code->labels_size += labels_init_size;
    return ASM_SUCCESS;
}

asm_error_t add_fix_up(code_t *code, size_t label_number) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = check_fixup_size(code)) != ASM_SUCCESS)
        return error_code;

    code->fixup[code->fixup_number].code_index   = code->output_code_size;
    code->fixup[code->fixup_number].label_number = label_number;
    code->fixup_number++;
    return ASM_SUCCESS;
}

asm_error_t check_fixup_size(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    if(code->fixup_number < code->fixup_size)
        return ASM_SUCCESS;

    fixup_t *new_fixup = (fixup_t *)_recalloc(code->fixup,
                                              code->fixup_size,
                                              code->fixup_size +
                                              fixups_init_size,
                                              sizeof(fixup_t));
    if(new_fixup == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reallocating memory to fixups.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    code->fixup      =  new_fixup;
    code->fixup_size += fixups_init_size;
    return ASM_SUCCESS;
}

asm_error_t code_clean_buffer(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    while(!isgraph(code->source_code[code->source_code_position]) &&
          code->source_code[code->source_code_position] != '\0') {
        code->source_code_position++;

        //move to next line if ';' found
        if(code->source_code[code->source_code_position] == ';') {
            while(code->source_code[code->source_code_position] != '\n' &&
                  code->source_code[code->source_code_position] != '\0')
                code->source_code_position++;
        }
    }

    return ASM_SUCCESS;
}

bool is_label(char *command) {
    C_ASSERT(command != NULL, return false);

    char *colon_pointer = strchr(command, ':');
    if(colon_pointer != NULL &&
       *(colon_pointer + 1) == '\0')
        return true;

    return false;
}

asm_error_t do_fixups(code_t *code) {
    C_ASSERT(code != NULL, return ASM_NULL_CODE);

    for(size_t index = 0; index < code->fixup_number; index++) {
        size_t code_index   = code->fixup[index].code_index;
        size_t label_number = code->fixup[index].label_number;
        size_t label_value  = code->labels[label_number].label_ip;

        *(size_t *)(code->output_code + code_index) = label_value;
    }
    return ASM_SUCCESS;
}
