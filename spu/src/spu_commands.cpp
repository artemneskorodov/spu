#include <math.h>
#include <stdio.h>
#include <windows.h>

#include "spu_facilities.h"
#include "spu_commands.h"
#include "colors.h"
#include "utils.h"

static argument_t  *get_args_push_pop    (spu_t *spu);
static spu_error_t  write_code_dump      (spu_t *spu);
static spu_error_t  write_registers_dump (spu_t *spu);
static spu_error_t  write_ram_dump       (spu_t *spu);

/**
======================================================================================================
    @brief      Runs command PUSH

    @details    Uses get_args_push_pop to get command arguments.
                It is expected that argument flags are set correctly and
                all arguments occur in right order:
                constant value, register value.

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
    argument_t first_item  = 0,
               second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = first_item + second_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = second_item - first_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = first_item * second_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = second_item / first_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
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
    argument_t item = 0;
    if(stack_pop (&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    item = sqrt(item);
    if(stack_push(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
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
    argument_t item = 0;
    if(stack_pop (&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    item = sin(item);
    if(stack_push(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
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
    argument_t item = 0;
    if(stack_pop (&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    item = cos(item);
    if(stack_push(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
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
    @brief      Dumps code array

    @details    Prints code array as table
                All indexes are printed as decimal numbers and
                all values in code are printed as hex numbers

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t write_code_dump(spu_t *spu) {
    color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|                Code:                |\r\n"
                 "|_____________________________________|\r\n");

    for(address_t item = 0; item < spu->code_size; item++) {
        background_t background = DEFAULT_BACKGROUND;
        if(item == spu->instruction_pointer)
            background = YELLOW_BACKGROUND;

        color_printf(BLUE_TEXT,    BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, background,
                     "  % 16llu",
                     item);
        color_printf(BLUE_TEXT,    BOLD_TEXT, background,
                     "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, background,
                     "0x%016llx",
                     spu->code[item]);
        color_printf(BLUE_TEXT,    BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|\r\n|");
        color_printf(BLUE_TEXT,    BOLD_TEXT, background,
                     "__________________|__________________");
        color_printf(BLUE_TEXT,    BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        if(item == spu->instruction_pointer)
            color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         " <--------     (instruction_pointer = 0x%llx)",
                         spu->instruction_pointer);
        printf("\r\n");
    }

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Dumps registers

    @details    Prints registers array as table.
                Uses intel register names.
                Values that are stored in registers are printed as hex numbers.

    @param [in] spu                 SPU structure

    @return Error code

======================================================================================================
*/
spu_error_t write_registers_dump(spu_t *spu) {
    color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|              Registers:             |\r\n"
                 "|_____________________________________|\r\n");

    for(address_t reg = 0; reg < registers_number; reg++) {
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "    %s    ",
                     spu_register_names[reg]);
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "        0x%016llx",
                     *(code_element_t *)(spu->registers + reg));
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|\r\n"
                     "|__________|__________________________|\r\n");
    }

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Dumps RAM

    @details    Prints RAM array as table.
                All indexes are printed as decimal numbers and
                all values, stored in RAM, printed as hex numbers.

    @param [in] spu                 SPU structure

    @return Error code

=======================================================================================================
*/
spu_error_t write_ram_dump(spu_t *spu) {
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|         Random Access Memory        |\r\n"
                 "|_____________________________________|\r\n");

    for(address_t item = 0; item < random_access_memory_size; item++) {
        color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "  % 16llu",
                     item);
        color_printf(YELLOW_TEXT,  BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "0x%016llx",
                     spu->random_access_memory[item]);
        color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|\r\n"
                     "|__________________|__________________|\r\n");
    }

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
    spu->instruction_pointer = (address_t)spu->code[spu->instruction_pointer];
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
    argument_t first_item  = 0,
               second_item = 0;

    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    if(first_item < second_item)
        return run_command_jmp(spu);

    spu->instruction_pointer++;
    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;

    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    if(first_item > second_item)
        return run_command_jmp(spu);

    spu->instruction_pointer++;
    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;

    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    if(first_item <= second_item)
        return run_command_jmp(spu);

    spu->instruction_pointer++;
    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;

    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    if(first_item >= second_item)
        return run_command_jmp(spu);

    spu->instruction_pointer++;
    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;

    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    if(is_equal_double(first_item, second_item))
        return run_command_jmp(spu);

    spu->instruction_pointer++;
    return SPU_SUCCESS;
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
    argument_t first_item  = 0,
               second_item = 0;

    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    if(!is_equal_double(first_item, second_item))
        return run_command_jmp(spu);

    spu->instruction_pointer++;
    return SPU_SUCCESS;
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
    size_t return_pointer = spu->instruction_pointer + 1;
    if(stack_push(&spu->stack, &return_pointer) != STACK_SUCCESS) {
        return SPU_STACK_ERROR;
    }

    spu->instruction_pointer = (size_t)spu->code[spu->instruction_pointer];
    return SPU_SUCCESS;
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

//         ___________________________________________________________________
//        | dx       |    push(val(dx))          |    pop(&dx)                | return address of dx
//        |__________|___________________________|____________________________|
//        | dx + 5   |    push(val(dx) + 5)      |    none                    | return address of push register and add values there
//        |__________|___________________________|____________________________|
//        | [dx + 5] |    push(ram[val(dx) + 5]) |    pop(&ram[val(dx) + 5])  | return address of ram
//        |__________|___________________________|____________________________|
//        | [dx]     |    push(ram[val(dx)])     |    pop(&ram[val(dx)])      | return address of ram
//        |__________|___________________________|____________________________|
//        | 5        |    push(5)                |    none                    | return address of push register and add values there
//        |__________|___________________________|____________________________|

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
    command_t      *code_pointer   = (command_t *)(spu->code +
                                                   spu->instruction_pointer - 1);
    command_t       operation_code = code_pointer[1];
    argument_type_t argument_type  = (argument_type_t)code_pointer[0];

    if(argument_type & random_access_memory_mask) {
        address_t ram_address = 0;

        if(argument_type & immediate_constant_mask) {
            ram_address += *(address_t *)(spu->code +
                                          spu->instruction_pointer);
            spu->instruction_pointer++;
        }

        if(argument_type & register_parameter_mask) {
            address_t register_number = *(address_t *)(spu->code +
                                                       spu->instruction_pointer);
            spu->instruction_pointer++;
            ram_address += (address_t)spu->registers[register_number - 1];
        }

        return spu->random_access_memory + ram_address;
    }

    else{
        if(operation_code == CMD_POP) {
            address_t register_number = *(address_t *)(spu->code +
                                                       spu->instruction_pointer);
            spu->instruction_pointer++;
            return spu->registers + register_number - 1;
        }

        else {
            spu->push_register = 0;

            if(argument_type & immediate_constant_mask) {
                spu->push_register += *(argument_t *)(spu->code +
                                                      spu->instruction_pointer);
                spu->instruction_pointer++;
            }

            if(argument_type & register_parameter_mask) {
                address_t register_number = *(address_t *)(spu->code +
                                                           spu->instruction_pointer);
                spu->instruction_pointer++;
                spu->push_register += spu->registers[register_number - 1];
            }

            return &spu->push_register;
        }
    }
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
spu_error_t run_command_draw (spu_t *spu) {
    char buffer[spu_drawing_height * (spu_drawing_width + 1) + 1] = {};
    size_t buffer_index = 0;
    for(size_t h = 0; h < spu_drawing_height; h++) {
        for(size_t w = 0; w < spu_drawing_width; w++) {
            code_element_t memory_element = *(code_element_t *)(spu->random_access_memory +
                                                                h * spu_drawing_width + w);
            if(memory_element == 0)
                buffer[buffer_index++] = '.';

            else
                buffer[buffer_index++] = '*';
        }
        buffer[buffer_index++] = '\n';
    }

    Sleep(30);
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
