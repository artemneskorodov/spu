#include <string.h>
#include <ctype.h>

#include "compiler.h"
#include "spu_facilities.h"
#include "memory.h"
#include "custom_assert.h"
#include "colors.h"

static const size_t max_command_length = 32;

static asm_error_t allocate_code_memory(code_t *code);
static asm_error_t parse_line(code_t *code);
static asm_error_t read_command(code_t *code,
                                char   *output);
static asm_error_t parse_command(code_t *code, char *command);
static asm_error_t code_move_next_line(code_t *code);
static asm_error_t parse_command_arguments(code_t *code, command_t operation_code);
static asm_error_t parse_call_jmp_arguments(code_t *code);
static asm_error_t parse_push_pop_arguments(code_t *code);
static size_t get_register_number(const char *register_name);
static command_t get_command_value(const char *command_name);

asm_error_t compile_code(code_t *code) {
    C_ASSERT(code              != NULL, return ASM_NULL_CODE  );
    C_ASSERT(code->source_code != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = allocate_code_memory(code)) != ASM_SUCCESS)
        return error_code;

    while(code->source_code_position < code->source_size) {
        if((error_code = parse_line(code)) != ASM_SUCCESS)
            return error_code;
    }

    if((error_code = do_fixups(&code->labels)) != ASM_SUCCESS)
        return error_code;

    return ASM_SUCCESS;
}

asm_error_t parse_line(code_t *code) {
    char command[max_command_length] = {};

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = read_command(code, command)) != ASM_SUCCESS)
        return error_code;

    if(is_label(command))
        return code_add_label(&code->labels,
                              command,
                              code->output_code_size,
                              true);

    if((error_code = parse_command(code, command)) != ASM_SUCCESS)
        return error_code;

    if((error_code = code_move_next_line(code)) != ASM_SUCCESS)
        return error_code;

    code->source_current_line++;
    return ASM_SUCCESS;
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

asm_error_t read_command(code_t *code,
                         char   *output) {
    int read_symbols = 0;
    if(sscanf(code->source_code + code->source_code_position,
              "%s%n",
              output,
              &read_symbols) != 1) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reading command %s:%llu.\r\n",
                     code->input_filename,
                     code->source_current_line);
        return ASM_READING_ERROR;
    }
    code->source_code_position += (size_t)read_symbols;
    return ASM_SUCCESS;
}

asm_error_t parse_command(code_t *code, char *command) {
    command_t operation_code = get_command_value(command);
    if(operation_code == CMD_UNKNOWN) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Unknown command '%s' %s:%llu.\r\n",
                     command,
                     code->input_filename,
                     code->source_current_line);
        return ASM_SYNTAX_ERROR;
    }

    uint32_t *code_pointer = (uint32_t *)(code->output_code + code->output_code_size);
    code->output_code_size++;
    code_pointer[1] = operation_code;

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = parse_command_arguments(code, operation_code)) != ASM_SUCCESS)
        return error_code;

    return ASM_SUCCESS;
}

asm_error_t code_move_next_line(code_t *code) {
    while(code->source_code[code->source_code_position] != '\n')
        code->source_code_position++;

    while(!isprint(code->source_code[code->source_code_position]))
        code->source_code_position++;

    return ASM_SUCCESS;
}

asm_error_t parse_command_arguments(code_t *code, command_t operation_code) {
    if(operation_code == CMD_PUSH ||
       operation_code == CMD_POP)
        return parse_push_pop_arguments(code);

    if(operation_code == CMD_CALL ||
       operation_code == CMD_JMP  ||
       operation_code == CMD_JA   ||
       operation_code == CMD_JB   ||
       operation_code == CMD_JAE  ||
       operation_code == CMD_JBE  ||
       operation_code == CMD_JE   ||
       operation_code == CMD_JNE)
        return parse_call_jmp_arguments(code);

    return ASM_SUCCESS;
}

asm_error_t parse_call_jmp_arguments(code_t *code) {
    while(!isgraph(code->source_code[code->source_code_position]))
        code->source_code_position++;

    size_t jump_instruction_pointer = 0;
    char label[max_label_name_size] = {};

    if(sscanf(code->source_code + code->source_code_position,
              "%llu",
              &jump_instruction_pointer) == 1) {
        *(size_t *)(code->output_code + code->output_code_size) = jump_instruction_pointer;
        code->output_code_size++;
        return ASM_SUCCESS;
    }

    if(sscanf(code->source_code + code->source_code_position,
              "%s",
              label) == 1) {
        if(!is_label(label))
            return ASM_LABEL_ERROR;

        asm_error_t error_code = ASM_SUCCESS;
        uint64_t *code_pointer = code->output_code + code->output_code_size;
        code->output_code_size++;
        if((error_code = get_label_instruction_pointer(&code->labels,
                                                       label,
                                                       code_pointer)) != ASM_SUCCESS)
            return error_code;

        return ASM_SUCCESS;
    }
}

asm_error_t parse_push_pop_arguments(code_t *code) {
    while(!isgraph(code->source_code[code->source_code_position]))
        code->source_code_position++;

    uint32_t *argument_type = (uint32_t *)(code->output_code + code->output_code_size - 1);

    char     register_name[3]       = {};
    uint64_t constant_integer_value = 0;
    double   constant_double_value  = 0;

    asm_error_t error_code = ASM_SUCCESS;

    //FIXME new function
    if(sscanf(code->source_code + code->source_code_position,
              "[%2[abcxspbpdisid] + %llu]",
              register_name,
              &constant_integer_value) == 2 ||
       sscanf(code->source_code + code->source_code_position,
              "[%llu + %2[abcxspbpdisid]]",
              &constant_integer_value,
              register_name) == 2) {
        size_t register_number = get_register_number(register_name);
        if(register_number < 1 || register_number > registers_number) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown register '%s' on %s:%llu\r\n",
                         register_name,
                         code->input_filename,
                         code->source_current_line);
            return ASM_REGISTER_ERROR;
        }

        *argument_type ^= immediate_constant_mask;
        *argument_type ^= register_parameter_mask;
        *argument_type ^= random_access_memory_mask;

        *(uint64_t *)(code->output_code + code->output_code_size) = constant_integer_value;
        code->output_code_size++;
        *(size_t   *)(code->output_code + code->output_code_size) = register_number;
        code->output_code_size++;

        return ASM_SUCCESS;
    }

    //FIXME new function
    if(sscanf(code->source_code + code->source_code_position,
              "[%llu]",
              &constant_integer_value) == 1) {
        *argument_type ^= immediate_constant_mask;
        *argument_type ^= random_access_memory_mask;

        *(uint64_t *)(code->output_code + code->output_code_size) = constant_integer_value;
        code->output_code_size++;

        return ASM_SUCCESS;
    }

    //FIXME new function
    if(sscanf(code->source_code + code->source_code_position,
              "[%2[abcxspbpdisid]]",
              register_name) == 1) {
        size_t register_number = get_register_number(register_name);
        if(register_number < 1 || register_number > registers_number) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown register '%s' on %s:%llu\r\n",
                         register_name,
                         code->input_filename,
                         code->source_current_line);
            return ASM_REGISTER_ERROR;
        }

        *argument_type ^= register_parameter_mask;
        *argument_type ^= random_access_memory_mask;

        *(size_t *)(code->output_code + code->output_code_size) = register_number;
        code->output_code_size++;

        return ASM_SUCCESS;
    }

    //FIXME new function
    if(sscanf(code->source_code + code->source_code_position,
              "%lg + %2[abcxspbpdisid]",
              &constant_double_value,
              register_name) == 2 ||
       sscanf(code->source_code + code->source_code_position,
              "%2[abcxspbpdisid] + %lg",
              register_name,
              &constant_double_value) == 2) {
        size_t register_number = get_register_number(register_name);
        if(register_number < 1 || register_number > registers_number) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown register '%s' on %s:%llu\r\n",
                         register_name,
                         code->input_filename,
                         code->source_current_line);
            return ASM_REGISTER_ERROR;
        }

        *argument_type ^= register_parameter_mask;
        *argument_type ^= immediate_constant_mask;

        *(uint64_t *)(code->output_code + code->output_code_size) = constant_integer_value;
        code->output_code_size++;
        *(size_t   *)(code->output_code + code->output_code_size) = register_number;
        code->output_code_size++;
        return ASM_SUCCESS;
    }

    //FIXME new function
    if(sscanf(code->source_code + code->source_code_position,
              "%2[abcxspbpdisid]",
              register_name) == 1) {
        size_t register_number = get_register_number(register_name);
        if(register_number < 1 || register_number > registers_number) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "Unknown register '%s' on %s:%llu\r\n",
                         register_name,
                         code->input_filename,
                         code->source_current_line);
            return ASM_REGISTER_ERROR;
        }

        *argument_type ^= register_parameter_mask;

        *(size_t   *)(code->output_code + code->output_code_size) = register_number;
        code->output_code_size++;

        return ASM_SUCCESS;
    }

    //FIXME new function
    if(sscanf(code->source_code + code->source_code_position,
              "%lg",
              &constant_double_value) == 1) {
        *argument_type ^= immediate_constant_mask;

        *(double *)(code->output_code + code->output_code_size) = constant_double_value;
        code->output_code_size++;
        return ASM_SUCCESS;
    }

    color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Unexpected parameter on %s:%llu\r\n",
                 code->input_filename,
                 code->source_current_line);
    return ASM_UNEXPECTED_PARAMETER;
}

size_t get_register_number(const char *register_name) {
    C_ASSERT(register_name != NULL, return ASM_INPUT_ERROR);
    for(size_t i = 0; i < registers_number; i++) {
        if(strcmp(spu_register_names[i], register_name) == 0)
            return i + 1;
    }
    return 0;
}

command_t get_command_value(const char *command_name) {
    C_ASSERT(command_name != NULL, return CMD_UNKNOWN);

    size_t commands_number = sizeof(supported_commands) / sizeof(supported_commands[0]);
    for(size_t index = 0; index < commands_number; index++)
        if(strcmp(command_name, supported_commands[index].command_name) == 0)
            return supported_commands[index].command_value;
    return CMD_UNKNOWN;
}
