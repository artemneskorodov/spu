#include <string.h>
#include <ctype.h>

#include "compiler.h"
#include "spu_facilities.h"
#include "memory.h"
#include "custom_assert.h"
#include "colors.h"

/**
======================================================================================================
     @brief     It is expected that all commands in source code wont exceed this size.

======================================================================================================
*/
static const size_t max_command_length = 32;

//====================================================================================================
//FUNCTIONS PROTOTYPES
//====================================================================================================
static asm_error_t allocate_code_memory     (code_t     *code);
static asm_error_t parse_line               (code_t     *code);
static asm_error_t read_command             (code_t     *code,
                                             char       *output);
static asm_error_t parse_command            (code_t     *code,
                                             char       *command);
static asm_error_t code_move_next_line      (code_t     *code);
static asm_error_t parse_command_arguments  (code_t     *code,
                                             command_t   operation_code);
static asm_error_t parse_call_jmp_arguments (code_t     *code);
static asm_error_t parse_push_pop_arguments (code_t     *code);
static asm_error_t try_read_RAM_const_reg   (code_t     *code);
static asm_error_t try_read_RAM_const       (code_t     *code);
static asm_error_t try_read_RAM_reg         (code_t     *code);
static asm_error_t try_read_const_reg       (code_t     *code);
static asm_error_t try_read_const           (code_t     *code);
static asm_error_t try_read_reg             (code_t     *code);
static asm_error_t verify_register          (address_t   register_number);
static address_t   get_register_number      (const char *register_name);
static command_t   get_command_value        (const char *command_name);


/**
======================================================================================================
    @brief      Compiles code.

    @details    Runs parsing line in cycle.

    @param [in] code                Code structure.

    @return Error code

======================================================================================================
*/
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

/**
======================================================================================================
    @brief      Parses one line of code.

    @details    Reads one line and determines if it is label or command.
                Runs functions that handles both variants.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
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

/**
======================================================================================================
    @brief      Allocates memory to code structure.

    @details    Allocates output code array and initializes labels structure.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t allocate_code_memory(code_t *code) {
    C_ASSERT(code              != NULL, return ASM_NULL_CODE  );
    C_ASSERT(code->source_code != NULL, return ASM_INPUT_ERROR);

    code->output_code = (code_element_t *)_calloc(code->source_size,
                                                  sizeof(code_element_t));
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

/**
======================================================================================================
    @brief      Reads one command from source code

    @details    Moves source code pointer to next symbol after read word.

    @param [in] code                Code structure.
    @param [in] output              Place to store command.

    @return Error code.

======================================================================================================
*/
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

/**
======================================================================================================
    @brief      Parses one command from source code.

    @details    Writes command number to last element in code array.
                Runs parse_command_arguments(...) to add arguments.

    @param [in] code                Code structure.
    @param [in] command             String with command.

    @return Error code.

======================================================================================================
*/
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

    argument_type_t *code_pointer = (argument_type_t *)(code->output_code +
                                                        code->output_code_size);
    code->output_code_size++;
    *(command_t *)(code_pointer + 1) = operation_code;

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = parse_command_arguments(code, operation_code)) != ASM_SUCCESS)
        return error_code;

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Cleans source code buffer.

    @details    Moves source code position to next line.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t code_move_next_line(code_t *code) {
    while(code->source_code[code->source_code_position] != '\n')
        code->source_code_position++;

    if(code->source_code[code->source_code_position + 1] == '\0') {
        code->source_code_position++;
        return ASM_SUCCESS;
    }

    while(!isprint(code->source_code[code->source_code_position]))
        code->source_code_position++;

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Parses command arguments.

    @details    Runs parsing push, pop, call and jumps arguments.

    @param [in] code                Code structure.
    @param [in] operation_code      Command code.

    @return Error code.

======================================================================================================
*/
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

/**
======================================================================================================
    @brief      Parses jump arguments.

    @details    Parses arguments for call and all variants of jmp.
                Can read labels and integer constants.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t parse_call_jmp_arguments(code_t *code) {
    while(!isgraph(code->source_code[code->source_code_position]))
        code->source_code_position++;

    address_t jump_instruction_pointer = 0;
    char label[max_label_name_size] = {};

    if(sscanf(code->source_code + code->source_code_position,
              "%llu",
              &jump_instruction_pointer) == 1) {
        *(address_t *)(code->output_code +
                       code->output_code_size) = jump_instruction_pointer;
        code->output_code_size++;
        return ASM_SUCCESS;
    }

    if(sscanf(code->source_code + code->source_code_position,
              "%s",
              label) == 1) {
        if(!is_label(label))
            return ASM_LABEL_ERROR;

        code_element_t *code_pointer = code->output_code +
                                       code->output_code_size;
        code->output_code_size++;
        asm_error_t error_code = ASM_SUCCESS;
        if((error_code = get_label_instruction_pointer(&code->labels,
                                                       label,
                                                       code_pointer)) != ASM_SUCCESS)
            return error_code;

        return ASM_SUCCESS;
    }

    color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Unexpected parameter on %s:%llu\r\n",
                 code->input_filename,
                 code->source_current_line);
    return ASM_UNEXPECTED_PARAMETER;
}

/**
======================================================================================================
    @brief      Parses arguments for push and pop.

    @details    Tries to read arguments in one of formats:
                [ax + 1], [ax], [1], ax + 1, ax, 1.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t parse_push_pop_arguments(code_t *code) {
    while(!isgraph(code->source_code[code->source_code_position]))
        code->source_code_position++;

    asm_error_t error_code = ASM_SUCCESS;


    if((error_code = try_read_RAM_const_reg(code)) != ASM_UNABLE_READ_ARGUMENT)
        return error_code;

    if((error_code = try_read_RAM_reg      (code)) != ASM_UNABLE_READ_ARGUMENT)
        return error_code;

    if((error_code = try_read_RAM_const    (code)) != ASM_UNABLE_READ_ARGUMENT)
        return error_code;

    if((error_code = try_read_const_reg    (code)) != ASM_UNABLE_READ_ARGUMENT)
        return error_code;

    if((error_code = try_read_const        (code)) != ASM_UNABLE_READ_ARGUMENT)
        return error_code;

    if((error_code = try_read_reg          (code)) != ASM_UNABLE_READ_ARGUMENT)
        return error_code;

    color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Unexpected parameter on %s:%llu\r\n",
                 code->input_filename,
                 code->source_current_line);
    return ASM_UNEXPECTED_PARAMETER;
}

/**
======================================================================================================
    @brief      Translates register as string to register number.

    @details    Function returns the number of register by its name.

    @param [in] register_name       String with register name.

    @return Error code.

======================================================================================================
*/
address_t get_register_number(const char *register_name) {
    C_ASSERT(register_name != NULL, return ASM_INPUT_ERROR);
    for(address_t i = 0; i < registers_number; i++) {
        if(strcmp(spu_register_names[i], register_name) == 0)
            return i + 1;
    }
    return 0;
}

/**
======================================================================================================
    @brief      Translates command as string to command enumerator.

    @details    Runs through supported_commands array and
                compares written names to command_name.

    @param [in] command_name        String with command.

    @return Error code.

======================================================================================================
*/
command_t get_command_value(const char *command_name) {
    C_ASSERT(command_name != NULL, return CMD_UNKNOWN);

    size_t commands_number = sizeof(supported_commands) / sizeof(supported_commands[0]);
    for(size_t index = 0; index < commands_number; index++) {
        const char *current_command = supported_commands[index].command_name;
        if(strcmp(command_name, current_command) == 0)
            return supported_commands[index].command_value;
    }
    return CMD_UNKNOWN;
}

/**
======================================================================================================
    @brief      Tries to read push or pop parameter with type [ax + 1]

    @details    If reading was success, function writes number of register and constant value to
                code array and turns argument type flags on.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t try_read_RAM_const_reg(code_t *code) {
    argument_type_t *argument_type          = (argument_type_t *)(code->output_code +
                                                                  code->output_code_size - 1);
    char             register_name[3]       = {};
    address_t        constant_integer_value = 0;
    char            *source_pointer         = code->source_code +
                                              code->source_code_position;

    if(sscanf(source_pointer,
              "[%2[abcxspbpdisid] + %llu]",
              register_name,
              &constant_integer_value) != 2 &&
       sscanf(source_pointer,
              "[%llu + %2[abcxspbpdisid]]",
              &constant_integer_value,
              register_name) != 2)
        return ASM_UNABLE_READ_ARGUMENT;

    address_t register_number = get_register_number(register_name);
    if(verify_register(register_number) != ASM_SUCCESS)
        return ASM_REGISTER_ERROR;

    *argument_type |= immediate_constant_mask;
    *argument_type |= register_parameter_mask;
    *argument_type |= random_access_memory_mask;

    *(address_t  *)(code->output_code + code->output_code_size) = constant_integer_value;
    code->output_code_size++;
    *(address_t  *)(code->output_code + code->output_code_size) = register_number;
    code->output_code_size++;

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Tries to read push or pop parameter with type [1]

    @details    If reading was success, function writes constant value to
                code array and turns argument type flags on.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t try_read_RAM_const(code_t *code) {
    argument_type_t *argument_type          = (uint32_t *)(code->output_code +
                                                           code->output_code_size - 1);
    address_t        constant_integer_value = 0;
    char            *source_pointer         = code->source_code +
                                              code->source_code_position;

    if(sscanf(source_pointer,
              "[%llu]",
              &constant_integer_value) != 1)
        return ASM_UNABLE_READ_ARGUMENT;

    *argument_type |= immediate_constant_mask;
    *argument_type |= random_access_memory_mask;

    *(address_t *)(code->output_code + code->output_code_size) = constant_integer_value;
    code->output_code_size++;

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Tries to read push or pop parameter with type [ax]

    @details    If reading was success, function writes number of register to
                code array and turns argument type flags on.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t try_read_RAM_reg(code_t *code) {
    argument_type_t *argument_type    = (argument_type_t *)(code->output_code +
                                                            code->output_code_size - 1);
    char             register_name[3] = {};
    char            *source_pointer   = code->source_code +
                                        code->source_code_position;

    if(sscanf(source_pointer,
              "[%2[abcxspbpdisid]]",
              register_name) != 1)
        return ASM_UNABLE_READ_ARGUMENT;

    address_t register_number = get_register_number(register_name);
    if(verify_register(register_number) != ASM_SUCCESS)
        return ASM_REGISTER_ERROR;

    *argument_type |= register_parameter_mask;
    *argument_type |= random_access_memory_mask;

    *(address_t *)(code->output_code + code->output_code_size) = register_number;
    code->output_code_size++;

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Tries to read push or pop parameter with type ax + 1

    @details    If reading was success, function writes number of register and constant value to
                code array and turns argument type flags on.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t try_read_const_reg(code_t *code) {
    argument_type_t *argument_type         = (argument_type_t *)(code->output_code +
                                                                 code->output_code_size - 1);
    char             register_name[3]      = {};
    argument_t       constant_double_value = 0;
    char            *source_pointer        = code->source_code +
                                             code->source_code_position;

    if(sscanf(source_pointer,
              "%lg + %2[abcxspbpdisid]",
              &constant_double_value,
              register_name) != 2 &&
       sscanf(source_pointer,
              "%2[abcxspbpdisid] + %lg",
              register_name,
              &constant_double_value) != 2)
        return ASM_UNABLE_READ_ARGUMENT;

    address_t register_number = get_register_number(register_name);
    if(verify_register(register_number) != ASM_SUCCESS)
        return ASM_REGISTER_ERROR;

    *argument_type |= register_parameter_mask;
    *argument_type |= immediate_constant_mask;

    *(argument_t *)(code->output_code + code->output_code_size) = constant_double_value;
    code->output_code_size++;
    *(address_t  *)(code->output_code + code->output_code_size) = register_number;
    code->output_code_size++;
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Tries to read push or pop parameter with type 1

    @details    If reading was success, function writes constant value to
                code array and turns argument type flags on.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t try_read_const(code_t *code) {
    argument_type_t *argument_type         = (argument_type_t *)(code->output_code +
                                                                 code->output_code_size - 1);
    argument_t       constant_double_value = 0;
    char            *source_pointer        = code->source_code +
                                             code->source_code_position;

    if(sscanf(source_pointer,
              "%lg",
              &constant_double_value) != 1)
        return ASM_UNABLE_READ_ARGUMENT;

    *argument_type |= immediate_constant_mask;

    *(argument_t *)(code->output_code + code->output_code_size) = constant_double_value;
    code->output_code_size++;
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Tries to read push or pop parameter with type ax

    @details    If reading was success, function writes number of register to
                code array and turns argument type flags on.

    @param [in] code                Code structure.

    @return Error code.

======================================================================================================
*/
asm_error_t try_read_reg(code_t *code) {
    argument_type_t *argument_type    = (argument_type_t *)(code->output_code +
                                                            code->output_code_size - 1);
    char             register_name[3] = {};
    char            *source_pointer   = code->source_code +
                                        code->source_code_position;

    if(sscanf(source_pointer,
              "%2[abcxspbpdisid]",
              register_name) != 1)
        return ASM_UNABLE_READ_ARGUMENT;

    address_t register_number = get_register_number(register_name);

    *argument_type |= register_parameter_mask;

    *(address_t *)(code->output_code + code->output_code_size) = register_number;
    code->output_code_size++;

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Verifies register number.

    @details    Prints register error and returns error value if register number is invalid.

    @param [in] register_number     The number of register argument in code.

    @return Error code.

======================================================================================================
*/
asm_error_t verify_register(address_t register_number) {
    if(register_number < 1 || register_number > registers_number) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Unknown register\r\n");
        return ASM_REGISTER_ERROR;
    }
    return ASM_SUCCESS;
}
