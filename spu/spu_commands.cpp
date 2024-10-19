#include <math.h>
#include <stdio.h>

#include "spu_facilities.h"
#include "spu_commands.h"
#include "colors.h"
#include "utils.h"

static uint64_t   *get_args_push_pop   (spu_t *spu);
static spu_error_t write_code_dump     (spu_t *spu);
static spu_error_t write_registers_dump(spu_t *spu);
static spu_error_t write_ram_dump      (spu_t *spu);

spu_error_t run_command_push(spu_t *spu) {
    if(stack_push(&spu->stack, get_args_push_pop(spu)) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_add(spu_t *spu) {
    argument_t first_item = 0, second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = first_item + second_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_sub(spu_t *spu) {
    argument_t first_item = 0, second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = second_item - first_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_mul(spu_t *spu) {
    argument_t first_item = 0, second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = first_item * second_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_div(spu_t *spu) {
    argument_t first_item = 0, second_item = 0;
    if(stack_pop(&spu->stack, &first_item ) != STACK_SUCCESS ||
       stack_pop(&spu->stack, &second_item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    argument_t result = second_item / first_item;
    if(stack_push(&spu->stack, &result) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

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

spu_error_t run_command_sqrt(spu_t *spu) {
    argument_t item = 0;
    if(stack_pop(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    item = sqrt(item);
    if(stack_push(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_sin(spu_t *spu) {
    argument_t item = 0;
    if(stack_pop(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    item = sin(item);
    if(stack_push(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_cos(spu_t *spu) {
    argument_t item = 0;
    if(stack_pop(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    item = cos(item);
    if(stack_push(&spu->stack, &item) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_dump(spu_t *spu) {
    if(STACK_DUMP(spu->stack, STACK_SUCCESS) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "=========================================="
                 "==========================================\r\n");

    spu_error_t error_code = SPU_SUCCESS;
    if((error_code = write_code_dump(spu)) != SPU_SUCCESS)
        return error_code;

    if((error_code = write_registers_dump(spu)) != SPU_SUCCESS)
        return error_code;

    if((error_code = write_ram_dump(spu)) != SPU_SUCCESS)
        return error_code;

    color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 "=========================================="
                 "==========================================\r\n");
    return SPU_SUCCESS;
}

spu_error_t write_code_dump(spu_t *spu) {
    color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|                Code:                |\r\n"
                 "|_____________________________________|\r\n");

    for(size_t item = 0; item < spu->code_size; item++) {
        background_t background = DEFAULT_BACKGROUND;
        if(item == spu->instruction_pointer)
            background = YELLOW_BACKGROUND;

        color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, background,
                     "  % 16llu",
                     item);
        color_printf(BLUE_TEXT, BOLD_TEXT, background,
                     "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, background,
                     "0x%016llx",
                     spu->code[item]);
        color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|\r\n|");
        color_printf(BLUE_TEXT, BOLD_TEXT, background,
                     "__________________|__________________");
        color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        if(item == spu->instruction_pointer)
            color_printf(GREEN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         " <--------     (instruction_pointer = 0x%llx)\r\n",
                         spu->instruction_pointer);
        else
            printf("\r\n");
    }

    return SPU_SUCCESS;
}

spu_error_t write_registers_dump(spu_t *spu) {
    color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|              Registers:             |\r\n"
                 "|_____________________________________|\r\n");

    for(size_t reg = 0; reg < registers_number; reg++) {
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "    %s    ",
                     spu_register_names[reg]);
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "        0x%016llx",
                     *(uint64_t *)(spu->registers + reg));
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|\r\n"
                     "|__________|__________________________|\r\n");
    }

    return SPU_SUCCESS;
}

spu_error_t write_ram_dump(spu_t *spu) {
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|         Random Access Memory        |\r\n"
                 "|_____________________________________|\r\n");

    for(size_t item = 0; item < random_access_memory_size; item++) {
        color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "  % 16llu",
                     item);
        color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
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

spu_error_t run_command_hlt(spu_t */*spu*/) {
    return SPU_EXIT_SUCCESS;
}

spu_error_t run_command_jmp(spu_t *spu) {
    spu->instruction_pointer = (size_t)spu->code[spu->instruction_pointer];
    return SPU_SUCCESS;
}

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

spu_error_t run_command_pop(spu_t *spu) {
    if(stack_pop(&spu->stack, get_args_push_pop(spu)) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

spu_error_t run_command_call(spu_t *spu) {
    size_t return_pointer = spu->instruction_pointer + 1;
    if(stack_push(&spu->stack, &return_pointer) != STACK_SUCCESS) {
        return SPU_STACK_ERROR;
    }

    spu->instruction_pointer = (size_t)spu->code[spu->instruction_pointer];
    return SPU_SUCCESS;
}

spu_error_t run_command_ret (spu_t *spu) {
    if(stack_pop(&spu->stack, &spu->instruction_pointer) != STACK_SUCCESS)
        return SPU_STACK_ERROR;

    return SPU_SUCCESS;
}

//TODO     ___________________________________________________________________
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

uint64_t *get_args_push_pop(spu_t *spu) {
    uint32_t *code_pointer   = (uint32_t *)(spu->code + spu->instruction_pointer - 1);
    command_t operation_code = (command_t)code_pointer[1];
    uint32_t  argument_type  = code_pointer[0];

    if(argument_type & random_access_memory_mask) {
        size_t ram_address = 0;

        if(argument_type & immediate_constant_mask) {
            ram_address += *(size_t *)(spu->code + spu->instruction_pointer);
            spu->instruction_pointer++;
        }

        if(argument_type & register_parameter_mask) {
            size_t register_number = *(size_t *)(spu->code + spu->instruction_pointer);
            spu->instruction_pointer++;
            ram_address += (size_t)spu->registers[register_number - 1];
        }

        return (uint64_t *)(spu->random_access_memory + ram_address);
    }

    else{
        if(operation_code == CMD_POP) {
            size_t register_number = *(size_t *)(spu->code + spu->instruction_pointer);
            spu->instruction_pointer++;
            return (uint64_t *)(spu->registers + register_number - 1);
        }

        else {
            spu->push_register = 0;

            if(argument_type & immediate_constant_mask) {
                spu->push_register += *(argument_t *)(spu->code + spu->instruction_pointer);
                spu->instruction_pointer++;
            }

            if(argument_type & register_parameter_mask) {
                size_t register_number = *(size_t *)(spu->code + spu->instruction_pointer);
                spu->instruction_pointer++;
                spu->push_register += spu->registers[register_number - 1];
            }

            return (uint64_t *)&spu->push_register;
        }
    }
}
