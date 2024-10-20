#include <ctype.h>
#include <string.h>

#include "compiler.h"
#include "asm.h"
#include "colors.h"
#include "spu_facilities.h"
#include "custom_assert.h"
#include "memory.h"
#include "labels.h"

static const size_t max_command_length = 32;

static asm_error_t code_add_command               (code_t       *code,
                                                   char         *command);
static asm_error_t read_next_command              (code_t       *code,
                                                   char         *output);
static asm_error_t code_clean_buffer              (code_t       *code);
static command_t   get_command_value              (char         *command);
static asm_error_t add_command_arguments          (code_t       *code,
                                                   command_t     command);
static asm_error_t add_arguments_push             (code_t       *code);
static asm_error_t add_arguments_pop              (code_t       *code);
static asm_error_t add_arguments_jmp              (code_t       *code);
static asm_error_t add_arguments_call             (code_t       *code);
static size_t      get_register_number            (char         *register_name);
static asm_error_t try_add_constant_argument      (code_t       *code,
                                                   uint32_t     *argument_type);
static asm_error_t try_add_memory_mask            (code_t       *code,
                                                   uint32_t     *argument_type);
static asm_error_t try_add_register_argument      (code_t       *code,
                                                   uint32_t     *argument_type);
static asm_error_t allocate_code_memory           (code_t       *code);

asm_error_t compile_code(code_t *code) {
    C_ASSERT(code              != NULL, return ASM_NULL_CODE  );
    C_ASSERT(code->source_code != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = allocate_code_memory(code)) != ASM_SUCCESS)
        return error_code;

    while(code->source_code_position < code->source_size) {
        char command[max_command_length] = {};

        if((error_code = read_next_command(code, command)) != ASM_SUCCESS)
            return error_code;

        if(is_label(command)) {
            if((error_code = code_add_label(&code->labels,
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

    if((error_code = do_fixups(&code->labels)) != ASM_SUCCESS) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while cleaning labels fixups.\r\n");
        return error_code;
    }

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
        uint64_t *code_label_pointer = code->output_code + code->output_code_size;
        code->output_code_size++;
        asm_error_t error_code = get_label_instruction_pointer(&code->labels,
                                                               label,
                                                               code_label_pointer);
        if(error_code != ASM_SUCCESS)
            return error_code;

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
        uint64_t *code_label_pointer = code->output_code + code->output_code_size;
        code->output_code_size++;
        asm_error_t error_code = get_label_instruction_pointer(&code->labels,
                                                               label,
                                                               code_label_pointer);
        if(error_code != ASM_SUCCESS)
            return error_code;

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

asm_error_t allocate_code_memory(code_t *code) {
    C_ASSERT(code              != NULL, return ASM_NULL_CODE  );
    C_ASSERT(code->source_code != NULL, return ASM_INPUT_ERROR);

    code->output_code = (uint64_t *)_calloc(code->source_size, sizeof(uint64_t));
    if(code->output_code == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to exit code.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    asm_error_t error_code = code_labels_init(&code->labels);
    if((error_code != ASM_SUCCESS))
        return error_code;

    code->source_current_line = 1;
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

