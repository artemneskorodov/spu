#include <math.h>
#include <stdio.h>
#include <windows.h>

#include "spu_facilities.h"
#include "spu_commands.h"
#include "colors.h"
#include "utils.h"
#include "dump.h"
#include "commands_utils.h"

//====================================================================================================
//FUNCTIONS PROTOTYPES
//====================================================================================================
static argument_t  *get_args_push_pop   (spu_t       *spu);
static spu_error_t  pop_two_elements    (spu_t       *spu,
                                         argument_t  *first,
                                         argument_t  *second);
static argument_t  *get_pop_argument    (spu_t       *spu);
static argument_t  *get_memory_address  (spu_t       *spu,
                                         command_t    argument_type);
static argument_t  *get_push_argument   (spu_t       *spu,
                                         command_t    argument_type);
static spu_error_t  jump_with_condition (spu_t       *spu,
                                         bool       (*comparator) (argument_t first,
                                                                   argument_t second));
static spu_error_t  calculate_for_two   (spu_t       *spu,
                                         argument_t (*function)   (argument_t first,
                                                                   argument_t second));
static spu_error_t  calculate_for_one   (spu_t       *spu,
                                         argument_t (*function)   (argument_t item));
static spu_error_t  copy_argument       (spu_t       *spu,
                                         void        *output);

/**
======================================================================================================
    @brief      Runs command PUSH

    @details    Uses get_args_push_pop to get command arguments.
                It is expected that argument flags are set correctly and
                all arguments occur in right order:
                constant value and than register value.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_push(spu_t *spu) {
    if(stack_push(&spu->stack, get_args_push_pop(spu)) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs command ADD

    @details    Pops two elements from stack, adds them and pushes the result back.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_add(spu_t *spu) {
    return calculate_for_two(spu, add_values);
}

/**
======================================================================================================
    @brief      Runs command SUB

    @details    Pops two elements from stack and pushes back the result of subtraction
                the first element from the second.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_sub(spu_t *spu) {
    return calculate_for_two(spu, sub_values);
}

/**
======================================================================================================
    @brief      Runs command MUL

    @details    Pops two elements from stack and pushes back the result of multiplying them.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_mul(spu_t *spu) {
    return calculate_for_two(spu, mul_values);
}

/**
======================================================================================================
    @brief      Runs command DIV

    @details    Pops two elements from stack and pushes back the result of dividing
                the second by the first.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_div(spu_t *spu) {
    return calculate_for_two(spu, div_values);
}

/**
======================================================================================================
    @brief      Runs command OUT

    @details    Pops one element from stack and prints it as double (%lg format).

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_out(spu_t *spu) {
    argument_t item = 0;
    if(stack_pop(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Output: ");

    color_printf(DEFAULT_TEXT, NORMAL_TEXT, DEFAULT_BACKGROUND,
                 "%lg\r\n", item);

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs command PUSH

    @details    Scans one element as double (%lg format) from user console and pushes it in stack.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_in(spu_t *spu) {
    argument_t item = 0;

    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "Input: ");
    if(scanf("%lg", &item) != 1)
        return SPU_INPUT_ERROR;

    if(stack_push(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs command SQRT

    @details    Pops one element from stack and pushes back its square root.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_sqrt(spu_t *spu) {
    return calculate_for_one(spu, sqrt);
}

/**
======================================================================================================
    @brief      Runs command SIN

    @details    Pops one element from stack and pushes back its sine.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_sin(spu_t *spu) {
    return calculate_for_one(spu, sin);
}

/**
======================================================================================================
    @brief      Runs command COS

    @details    Pops one element from stack and pushes back its cosine.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_cos(spu_t *spu) {
    return calculate_for_one(spu, cos);
}

/**
======================================================================================================
    @brief      Runs command DUMP

    @details    Dumps spu structure
                Runs other functions to dump code array, registers and RAM, runs stack dump,
                which is written to "stack.log"

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_dump(spu_t *spu) {
    if(STACK_DUMP(spu->stack, STACK_SUCCESS) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "=========================================="
                 "==========================================\r\n");

    spu_error_t error_code = SPU_SUCCESS;
    if((error_code = write_code_dump     (spu)) != SPU_SUCCESS)
        return error_code;

    if((error_code = write_registers_dump(spu)) != SPU_SUCCESS)
        return error_code;

    if((error_code = write_ram_dump      (spu)) != SPU_SUCCESS)
        return error_code;

    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "=========================================="
                 "==========================================\r\n");
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs command HLT

    @details    The return value represents that program is finished.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_hlt(spu_t */*spu*/) {
    return SPU_EXIT_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs command JMP

    @details    Changes instruction pointer to the value,
                which is located after command in code array.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_jmp(spu_t *spu) {
    spu_error_t error_code              = SPU_SUCCESS;
    address_t   new_instruction_pointer = 0;

    if((error_code = copy_argument(spu, &new_instruction_pointer)) != SPU_SUCCESS)
        return error_code;

    spu->instruction_pointer = new_instruction_pointer;
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs command JA

    @details    Pops two elements from stack.
                Runs jmp command if the first is less then the second

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_ja(spu_t *spu) {
    return jump_with_condition(spu, is_above);
}

/**
======================================================================================================
    @brief      Runs command JB

    @details    Pops two elements from stack.
                Runs jmp command if the first is bigger then the second

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_jb(spu_t *spu) {
    return jump_with_condition(spu, is_below);
}

/**
======================================================================================================
    @brief      Runs command JAE

    @details    Pops two elements from stack.
                Runs jmp command if the first is less then or equal to the second

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_jae(spu_t *spu) {
    return jump_with_condition(spu, is_above_or_equal);
}

/**
======================================================================================================
    @brief      Runs command JBE

    @details    Pops two elements from stack.
                Runs jmp command if the first is less then or equal to the second

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_jbe(spu_t *spu) {
    return jump_with_condition(spu, is_below_or_equal);
}

/**
======================================================================================================
    @brief      Runs command JE

    @details    Pops two elements from stack.
                Runs jmp command if the first is equal to the second

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_je(spu_t *spu) {
    return jump_with_condition(spu, is_equal);
}

/**
======================================================================================================
    @brief      Runs command JNE

    @details    Pops two elements from stack.
                Runs jmp command if the first is not equal to the second

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_jne(spu_t *spu) {
    return jump_with_condition(spu, is_not_equal);
}

/**
======================================================================================================
    @brief      Runs command POP

    @details    Uses get_args_push_pop to gain the pointer,
                where to put the result of stack pop.
                Pops one element from stack.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_pop(spu_t *spu) {
    if(stack_pop(&spu->stack, get_args_push_pop(spu)) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Runs command CALL

    @details    Pushes current value of instruction pointer to stack,
                Runs jmp command

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_call(spu_t *spu) {
    address_t return_pointer = spu->instruction_pointer + sizeof(address_t);

    if(stack_push(&spu->stack, &return_pointer) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return run_command_jmp(spu);
}

/**
======================================================================================================
    @brief      Runs command JA

    @details    Pops one element from stack, it is assumed that
                when the ret command occurs the last element in stack is instruction pointer,
                which is pushed there by command call.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_ret (spu_t *spu) {
    if(stack_pop(&spu->stack, &spu->instruction_pointer) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Reads arguments to push and pop

    @details    If the argument is RAM address function returns pointer to particular element in RAM.
                Else if the command is pop it returns the pointer to register.
                Else the argument type represents the value and function puts it in push_register and
                returns its address.

    @param [in] spu                 SPU structure

    @return Pointer to element where to pop, or from where to push.

======================================================================================================
*/
argument_t *get_args_push_pop(spu_t *spu) {
    command_t code_element   = spu->code[spu->instruction_pointer - 1];
    command_t operation_code = (command_t)(code_element & operation_code_mask);
    command_t argument_type  = (command_t)(code_element & argument_type_mask );

    if(argument_type & random_access_memory_mask)
        return get_memory_address(spu, argument_type);

    if(operation_code == CMD_POP)
        return get_pop_argument(spu);

    return get_push_argument(spu, argument_type);
}

/**
======================================================================================================
    @brief      Reads arguments to pop

    @details    It is expected that it is already checked that pop argument
                is not a RAM address.
                Function reads only register parameter.

    @param [in] spu                 SPU structure

    @return Pointer to element where to pop, or from where to push.

======================================================================================================
*/
argument_t *get_pop_argument(spu_t *spu) {
    spu_error_t error_code      = SPU_SUCCESS;
    address_t   register_number = 0;

    if((error_code = copy_argument(spu, &register_number)) != SPU_SUCCESS)
        return NULL;

    return spu->registers + register_number - 1;
}

/**
======================================================================================================
    @brief      Reads arguments to push

    @details    It is expected that it is already checked that push argument
                is not a RAM address.
                If constant occurs, function reads it as argument_t value (double).
                Registers are treated as their values.
                Function puts values in push register of SPU and returns its pointer.

    @param [in] spu                 SPU structure
    @param [in] argument_type       Integers with flags set on types of arguments.

    @return Pointer to element where to pop, or from where to push.

======================================================================================================
*/
argument_t *get_push_argument(spu_t     *spu,
                              command_t  argument_type) {
    spu->push_register = 0;
    spu_error_t error_code = SPU_SUCCESS;

    if(argument_type & immediate_constant_mask) {
        argument_t constant_value = 0;
        if((error_code = copy_argument(spu, &constant_value)) != SPU_SUCCESS)
            return NULL;

        spu->push_register += constant_value;
    }

    if(argument_type & register_parameter_mask) {
        address_t register_number = 0;
        if((error_code = copy_argument(spu, &register_number)) != SPU_SUCCESS)
            return NULL;

        spu->push_register += spu->registers[register_number - 1];
    }

    return &spu->push_register;
}

/**
======================================================================================================
    @brief      Reads arguments to push and pop in memory

    @details    It is expected that it is checked that argument type mask of RAM is on.
                Function treat constants as address_t (uint64_t).
                Values in registers are casted to address_t.
                Function returns pointer to RAM cell.

    @param [in] spu                 SPU structure
    @param [in] argument_type       Integers with flags set on types of arguments.

    @return Pointer to element where to pop, or from where to push.

======================================================================================================
*/
argument_t *get_memory_address(spu_t     *spu,
                               command_t  argument_type) {
    address_t   ram_address = 0;
    spu_error_t error_code  = SPU_SUCCESS;

    if(argument_type & immediate_constant_mask) {
        address_t constant_value = 0;
        if((error_code = copy_argument(spu, &constant_value)) != SPU_SUCCESS)
            return NULL;

        ram_address += constant_value;
    }

    if(argument_type & register_parameter_mask) {
        address_t register_number = 0;
        if((error_code = copy_argument(spu, &register_number)) != SPU_SUCCESS)
            return NULL;

        ram_address += (address_t)spu->registers[register_number - 1];
    }

    return spu->random_access_memory + ram_address;
}

/**
======================================================================================================
    @brief      Runs command DRAW

    @details    Scans the first spu_drawing_height * spu_drawing_width elements of RAM.
                If the element is 0 function prints '.' and if not, it prints '*'.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_draw(spu_t *spu) {
    char buffer[spu_drawing_height * (spu_drawing_width + 1) + 1] = {};
    size_t buffer_index = 0;
    for(size_t h = 0; h < spu_drawing_height; h++) {
        for(size_t w = 0; w < spu_drawing_width; w++) {
            uint64_t memory_element = *(uint64_t *)(spu->random_access_memory +
                                                    h * spu_drawing_width + w);
            if(memory_element == 0)
                buffer[buffer_index++] = '.';

            else
                buffer[buffer_index++] = '*';
        }
        buffer[buffer_index++] = '\n';
    }

    Sleep(20);
    system("cls");
    fputs(buffer, stderr);
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Main command.

    @details    Prints epileptic tea.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t run_command_chai(spu_t */*spu*/) {
    const char *first_frame  = "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@......@@@..@@@@..@@@@@@.@@@@@@......@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@...@@@@@@@..@@@@..@@@@@...@@@@@@@..@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@..@@@@@@@@........@@@@..@..@@@@@@..@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@...@@@@@@@..@@@@..@@@.......@@@@@..@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@......@@@..@@@@..@@@..@@@..@@@......@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@###############@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@######        +      ######@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@##   +     +         +    +  ##@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@#      +     +    + + +         #@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@##      +      +        +    ##@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@.######    +      +   ######......@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@.......###############.......@@@@...@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@...........................@@@@@@@..@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@...........................@@@@@@...@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@.........................@@@@@...@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@..............................@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@...................@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@.............@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n"
                               "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n";

    const char *second_frame = ".......................................................................................\r\n"
                               "......................@@@@@@...@@....@@......@......@@@@@@.............................\r\n"
                               ".....................@@@.......@@....@@.....@@@.......@@...............................\r\n"
                               ".....................@@........@@@@@@@@....@@.@@......@@...............................\r\n"
                               ".....................@@@.......@@....@@...@@@@@@@.....@@...............................\r\n"
                               "......................@@@@@@...@@....@@...@@...@@...@@@@@@.............................\r\n"
                               ".................................###############.......................................\r\n"
                               "...........................######    +    +     ######.................................\r\n"
                               ".........................##     +    +    +  +    +   ##...............................\r\n"
                               "........................#        +  +   +  +     +    + #..............................\r\n"
                               ".........................## +        +   +  +         ##...............................\r\n"
                               "..........................@######  +            ######@@@@@@...........................\r\n"
                               "..........................@@@@@@@###############@@@@@@@....@@@.........................\r\n"
                               "...........................@@@@@@@@@@@@@@@@@@@@@@@@@@@.......@@........................\r\n"
                               "...........................@@@@@@@@@@@@@@@@@@@@@@@@@@@......@@@........................\r\n"
                               "............................@@@@@@@@@@@@@@@@@@@@@@@@@.....@@@..........................\r\n"
                               ".............................@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@............................\r\n"
                               "..............................@@@@@@@@@@@@@@@@@@@......................................\r\n"
                               ".................................@@@@@@@@@@@@@.........................................\r\n"
                               ".......................................................................................\r\n"
                               ".......................................................................................\r\n"
                               ".......................................................................................\r\n"
                               ".......................................................................................\r\n";

    const size_t chai_cycles = 1024;

    for(size_t frame = 0; frame < chai_cycles; frame++) {
        system("cls");
        if(frame % 2 == 0)
            fputs(first_frame, stderr);
        else
            fputs(second_frame, stderr);
        Sleep(50);
    }

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Pops two elements from SPU stack and writes them to first and second.

    @details    First is the number of pop, so it will contain the last element in stack.

    @param [in] spu                 SPU structure
    @param [in] first               Pointer to storage for first pop.
    @param [in] second              Pointer to storage for second pop.

    @return Error code

======================================================================================================
*/
spu_error_t pop_two_elements(spu_t      *spu,
                             argument_t *first,
                             argument_t *second) {
    if(stack_pop(&spu->stack, first ) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    if(stack_pop(&spu->stack, second) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Pushes function result to stack.

    @details    Pops two elements from stack and passes them in function given by caller.
                First argument in called function will be the result of first pop.
                Function result is pushed in SPU stack.

    @param [in] spu                 SPU structure
    @param [in] function            Function which will be the result of command.

    @return Error code

======================================================================================================
*/
spu_error_t calculate_for_two(spu_t       *spu,
                              argument_t (*function)(argument_t first,
                                                     argument_t second)) {
    argument_t first_item  = 0,
               second_item = 0;

    if(pop_two_elements(spu, &first_item, &second_item) != SPU_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = function(first_item, second_item);
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Jumps with condition.

    @details    Pops two elements from stack and passes them in comparator.
                The first argument in comparator will be the result of first pop.
                If comparator returns true, function calls run_command_jmp.
                Else it moves instruction pointer to next command.

    @param [in] spu                 SPU structure
    @param [in] comparator          Function which compare to elements.

    @return Error code

======================================================================================================
*/
spu_error_t jump_with_condition(spu_t  *spu,
                                bool  (*comparator)(argument_t first,
                                                    argument_t second)) {
    argument_t first_item  = 0,
               second_item = 0;

    if(pop_two_elements(spu, &first_item, &second_item) != SPU_SUCCESS)
        return SPU_STACK_ERROR;

    if(comparator(first_item, second_item))
        return run_command_jmp(spu);

    spu->instruction_pointer += sizeof(address_t);
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Pushes function result to stack.

    @details    Pops one element from stack and passes it to function given by caller.
                Function result is pushed to SPU stack.

    @param [in] spu                 SPU structure
    @param [in] function            Function which will be the result of command.

    @return Error code

======================================================================================================
*/
spu_error_t calculate_for_one(spu_t       *spu,
                              argument_t (*function)(argument_t item)) {
    argument_t item = 0;
    if(stack_pop(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = function(item);
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Reads one argument from code.

    @details    Copies 8 bytes from code array to output.
                Moves instruction pointer to next cell of code.

    @param [in] spu                 SPU structure
    @param [in] output              Storage to argument.

    @return Error code

======================================================================================================
*/
spu_error_t copy_argument(spu_t *spu,
                          void  *output) {
    if(memcpy(output, spu->code + spu->instruction_pointer, sizeof(uint64_t)) != output)
        return SPU_MEMSET_ERROR;

    spu->instruction_pointer += sizeof(uint64_t);
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Checks if command is valid.

    @details    Commands are treated as valid if their numerical value
                lies between first and last command.

    @param [in] operation_code      Command

    @return Error code

======================================================================================================
*/
bool is_command_supported (command_t operation_code) {
    if(operation_code >= processor_first_command &&
       operation_code <= processor_last_command)
        return true;

    return false;
}
