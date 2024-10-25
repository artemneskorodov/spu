#include "dump.h"
#include "colors.h"

//====================================================================================================
//FUNCTIONS PROTOTYPES
//====================================================================================================
static const size_t dump_elements_in_line = 15;

static spu_error_t  print_code_line     (spu_t    *spu,
                                         size_t    line,
                                         size_t    width);
static spu_error_t  print_top_border    (size_t    width);
static spu_error_t  print_code_indexes  (size_t    width,
                                         size_t    ip_elem,
                                         size_t    line);
static spu_error_t  print_bottom_border (size_t    width,
                                         size_t    ip_elem);
static spu_error_t  print_code_elements (spu_t    *spu,
                                         size_t    width,
                                         size_t    ip_elem,
                                         size_t    lines);

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
                 "|_____________________________________|\r\n\r\n");

    size_t lines_number = spu->code_size / dump_elements_in_line;
    size_t last_line_elements = spu->code_size % dump_elements_in_line;

    for(size_t line = 0; line < lines_number; line++)
        if(print_code_line(spu, line, dump_elements_in_line) != SPU_SUCCESS)
            return SPU_DUMP_ERROR;

    if(print_code_line(spu, lines_number, last_line_elements) != SPU_SUCCESS)
        return SPU_DUMP_ERROR;

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

    @todo

======================================================================================================
*/
spu_error_t write_registers_dump(spu_t *spu) {
    color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|              Registers:             |\r\n"
                 "|_____________________________________|\r\n");

    for(address_t reg = 0; reg < registers_number; reg++) {
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "    %s    ",
                     spu_register_names[reg]);
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "        0x%016llx",
                     *(address_t *)(spu->registers + reg));
        color_printf(CYAN_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|\r\n"
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

    @todo

=======================================================================================================
*/
spu_error_t write_ram_dump(spu_t *spu) {
    color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                 " _____________________________________ \r\n"
                 "|         Random Access Memory        |\r\n"
                 "|_____________________________________|\r\n");

    for(address_t item = 0; item < random_access_memory_size; item++) {
        color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "  % 16llu",
                     item);
        color_printf(YELLOW_TEXT,  BOLD_TEXT, DEFAULT_BACKGROUND, "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "0x%016llx",
                     spu->random_access_memory[item]);
        color_printf(YELLOW_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "|\r\n"
                     "|__________________|__________________|\r\n");
    }

    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Dumps code array

    @details    Prints one line of code array,
                Length of line is determined in dump_elements_in_line.

    @param [in] spu                 SPU structure
    @param [in] line                Number of currently printing line.

    @return Error code

======================================================================================================
*/
spu_error_t print_code_line(spu_t *spu, size_t line, size_t width) {
    size_t instruction_pointer_element = dump_elements_in_line;
    if(spu->instruction_pointer >= line * dump_elements_in_line &&
       spu->instruction_pointer < (line + 1) * dump_elements_in_line)
        instruction_pointer_element = spu->instruction_pointer - line * dump_elements_in_line;

    if(print_top_border(width) != SPU_SUCCESS)
        return SPU_DUMP_ERROR;

    if(print_code_indexes(width, instruction_pointer_element, line) != SPU_SUCCESS)
        return SPU_DUMP_ERROR;

    if(print_bottom_border(width, instruction_pointer_element) != SPU_SUCCESS)
        return SPU_DUMP_ERROR;

    if(print_code_elements(spu, width, instruction_pointer_element, line) != SPU_SUCCESS)
        return SPU_DUMP_ERROR;

    if(print_bottom_border(width, instruction_pointer_element) != SPU_SUCCESS)
        return SPU_DUMP_ERROR;

    fputs("\r\n\r\n", stdout);
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Prints top border of line of code.

    @details    Prints top border of cell in line for every element in table.

    @param [in] width               Number of elements in line.

    @return Error code

======================================================================================================
*/
spu_error_t print_top_border(size_t width) {
    for(size_t elem = 0; elem < width; elem++)
        color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, " ________");

    fputs("\r\n", stdout);
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Prints indexes for line of code.

    @details    Prints indexes of current line.
                Colorizes background if if element is under instruction pointer.
                It is expected that ip_elem is set to value that can not occur in line if line
                does not have element under instruction pointer.

    @param [in] width               Number of elements in line.
    @param [in] ip_elem             Index of element under instruction pointer.
    @param [in] line                Number of currently printing line of code table.

    @return Error code

======================================================================================================
*/
spu_error_t print_code_indexes(size_t width, size_t ip_elem, size_t line) {
    for(size_t elem = 0; elem < width; elem++) {
        background_t background = DEFAULT_BACKGROUND;
        if(elem == ip_elem)
            background = GREEN_BACKGROUND;

        color_printf(BLUE_TEXT,    BOLD_TEXT, DEFAULT_BACKGROUND, "|");
        color_printf(MAGENTA_TEXT, BOLD_TEXT, background,         "% 8llu",
                     elem + line * dump_elements_in_line);
    }

    color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|\r\n");
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Prints bottom border for line in code table.

    @details    Colorizes background if if element is under instruction pointer.
                It is expected that ip_elem is set to value that can not occur in line if line
                does not have element under instruction pointer.

    @param [in] width               Number of elements in line.
    @param [in] ip_elem             Index of element under instruction pointer.

    @return Error code

======================================================================================================
*/
spu_error_t print_bottom_border(size_t width, size_t ip_elem) {
    for(size_t elem = 0; elem < width; elem++) {
        background_t background = DEFAULT_BACKGROUND;
        if(elem == ip_elem)
            background = GREEN_BACKGROUND;

        color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|");
        color_printf(BLUE_TEXT, BOLD_TEXT, background,         "________");
    }
    color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|\r\n");
    return SPU_SUCCESS;
}

/**
======================================================================================================
    @brief      Prints elements in code array.

    @details    Colorizes background if if element is under instruction pointer.
                It is expected that ip_elem is set to value that can not occur in line if line
                does not have element under instruction pointer.

    @param [in] spu                 SPU structure.
    @param [in] width               Number of elements in line.
    @param [in] ip_elem             Index of element under instruction pointer.
    @param [in] line                Number of currently printing line of code table.

    @return Error code

======================================================================================================
*/
spu_error_t print_code_elements(spu_t *spu, size_t width, size_t ip_elem, size_t line) {
    for(size_t elem = 0; elem < width; elem++) {
        background_t background = DEFAULT_BACKGROUND;
        if(elem == ip_elem)
            background = GREEN_BACKGROUND;

        color_printf(BLUE_TEXT   , BOLD_TEXT, DEFAULT_BACKGROUND, "|");
        color_printf(DEFAULT_TEXT, BOLD_TEXT, background,         "  0x%02x  ",
                     spu->code[elem + line * dump_elements_in_line]);
    }
    color_printf(BLUE_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND, "|\r\n");
    return SPU_SUCCESS;
}
