#include <string.h>
#include <ctype.h>
#include <stdint.h>


#include "labels.h"
#include "custom_assert.h"
#include "colors.h"
#include "memory.h"
#include "spu_facilities.h"

/**
======================================================================================================
    @brief Initializing size of labels array.

======================================================================================================
*/
static const size_t labels_init_size    = 32;

/**
======================================================================================================
    @brief Initializing size of fixups array.

======================================================================================================
*/
static const size_t fixups_init_size    = 32;

//====================================================================================================
//FUNCTIONS PROTOTYPES
//====================================================================================================
static asm_error_t try_find_label    (labels_array_t *labels_array,
                                      char           *label_name,
                                      command_t      *code_label_pointer);
static asm_error_t add_fix_up        (labels_array_t *labels_array,
                                      size_t          label_number,
                                      command_t      *code_label_pointer);
static asm_error_t check_labels_size (labels_array_t *labels_array);
static asm_error_t check_fixup_size  (labels_array_t *labels_array);
static asm_error_t paste_label_ip    (label_t        *label,
                                      command_t      *code_pointer);

/**
======================================================================================================
    @brief Label structure

======================================================================================================
*/
struct label_t {
    char            label_name[max_label_name_size];
    address_t       label_ip;
    bool            is_defined;
};

/**
======================================================================================================
    @brief Fixup structure

======================================================================================================
*/
struct fixup_t {
    size_t          label_number;
    command_t      *code_element;
};

/**
======================================================================================================
    @brief      Initializes labels structure.

    @details    Labels structure consist of two arrays: labels and fixups.
                This function allocates memory to this arrays and sets default values in labels structure.

    @param [in] labels_array        Labels structure pointer.

    @return Error code

======================================================================================================
*/
asm_error_t code_labels_init(labels_array_t *labels_array) {
    labels_array->fixup = (fixup_t *)_calloc(fixups_init_size, sizeof(fixup_t));
    if(labels_array->fixup == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to fixup's.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    labels_array->labels = (label_t *)_calloc(labels_init_size, sizeof(label_t));
    if(labels_array->labels == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while allocating memory to labels.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    labels_array->labels_size   = labels_init_size;
    labels_array->fixup_size    = fixups_init_size;
    labels_array->fixup_number  = 0;
    labels_array->labels_number = 0;
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Searches if label already exists in labels array.

    @details    Function compares written label names with label_name.
                If written label is found, function tries to read its instruction pointer,
                If label is undefined yet, function adds code element to fixups array.
                If it is the first time label occurs, function return ASM_NO_LABEL.

    @param [in] labels_array        Labels structure pointer.
    @param [in] label_name          String with label name.
    @param [in] code_label_pointer  Place in code where label represents instruction pointer value.

    @return Error code (ASM_NO_LABEL if label does not exist)

======================================================================================================
*/
asm_error_t try_find_label(labels_array_t *labels_array,
                           char           *label_name,
                           command_t      *code_label_pointer) {
    C_ASSERT(labels_array       != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(label_name         != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(code_label_pointer != NULL, return ASM_INPUT_ERROR);
    for(size_t index = 0; index < labels_array->labels_number; index++) {
        label_t *label = labels_array->labels + index;
        if(strcmp(label->label_name, label_name) == 0) {
            if(label->is_defined) {
                asm_error_t error_code = paste_label_ip(label, code_label_pointer);
                if(error_code != ASM_SUCCESS)
                    return error_code;

                return ASM_SUCCESS;
            }
            else {
                asm_error_t error_code = add_fix_up(labels_array, index, code_label_pointer);
                if(error_code != ASM_SUCCESS)
                    return error_code;

                return ASM_SUCCESS;
            }
        }
    }
    return ASM_NO_LABEL;
}

/**
======================================================================================================
    @brief      Writes label instruction pointer.

    @details    Firstly function tries to find existing label.
                If it is found, it is expected that function try_find_label(...) wrote it.
                If there is no existing label with searching name, function adds label to
                labels array and if needed adds it to fixups array.

    @param [in] labels_array        Labels structure pointer.
    @param [in] label_name          String with label name.
    @param [in] code_label_pointer  Place in code where label represents instruction pointer value.

    @return Error code

======================================================================================================
*/
asm_error_t get_label_instruction_pointer(labels_array_t *labels_array,
                                          char           *label_name,
                                          command_t      *code_label_pointer) {
    C_ASSERT(labels_array       != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(label_name         != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(code_label_pointer != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = try_find_label(labels_array,
                                    label_name,
                                    code_label_pointer)) != ASM_NO_LABEL)
        return error_code;

    if((error_code = code_add_label(labels_array,
                                    label_name,
                                    0,
                                    false)) != ASM_SUCCESS)
        return error_code;

    if((error_code = add_fix_up(labels_array,
                                labels_array->labels_number - 1,
                                code_label_pointer)) != ASM_SUCCESS)
        return error_code;

    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Adds label to labels array.

    @details    Function tries to find existing label with the same name.
                If there is not such label, it writes a new one.
                It is expected that is_defined is set to true only when instruction pointer represents
                the final label value.

    @param [in] labels_array        Labels structure pointer.
    @param [in] label_name          String with label name.
    @param [in] instruction_pointer Instruction pointer value represented by label.
    @param [in] is_defined          Determines if label is already defined.

    @return Error code

======================================================================================================
*/
asm_error_t code_add_label(labels_array_t *labels_array,
                           char           *label_name,
                           address_t       instruction_pointer,
                           bool            is_defined) {
    C_ASSERT(labels_array != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(label_name   != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = check_labels_size(labels_array)) != ASM_SUCCESS)
        return error_code;

    label_t *new_label = labels_array->labels +
                         labels_array->labels_number;

    for(size_t index = 0; index < labels_array->labels_number; index++)
        if(strcmp(labels_array->labels[index].label_name, label_name) == 0)
            new_label = labels_array->labels + index;

    if(strcpy(new_label->label_name,
              label_name) != new_label->label_name)
        return ASM_LABEL_ERROR;

    new_label->is_defined = is_defined;
    new_label->label_ip   = instruction_pointer;
    labels_array->labels_number++;
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Checks if size of label is sufficient.

    @details    If number of labels is bigger then or equal to size of array, it reallocated label array.

    @param [in] labels_array        Labels structure pointer.

    @return Error code

======================================================================================================
*/
asm_error_t check_labels_size(labels_array_t *labels_array) {
    C_ASSERT(labels_array != NULL, return ASM_INPUT_ERROR);

    if(labels_array->labels_number < labels_array->labels_size)
        return ASM_SUCCESS;

    label_t *new_labels = (label_t *)_recalloc(labels_array->labels,
                                               labels_array->labels_size,
                                               labels_array->labels_size +
                                               labels_init_size,
                                               sizeof(label_t));
    if(new_labels == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reallocating memory to labels.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    labels_array->labels      =  new_labels;
    labels_array->labels_size += labels_init_size;
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Adds fixup to labels structure.

    @details    Adds one element to fixups array, checking if size is sufficient.

    @param [in] labels_array        Labels structure pointer.
    @param [in] label_number        Index of label in labels array.
    @param [in] code_label_pointer  Place in code where label represents instruction pointer value.

    @return Error code

======================================================================================================
*/
asm_error_t add_fix_up(labels_array_t *labels_array,
                       size_t          label_number,
                       command_t      *code_label_pointer) {
    C_ASSERT(labels_array       != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(code_label_pointer != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    if((error_code = check_fixup_size(labels_array)) != ASM_SUCCESS)
        return error_code;

    labels_array->fixup[labels_array->fixup_number].code_element = code_label_pointer;
    labels_array->fixup[labels_array->fixup_number].label_number = label_number;
    labels_array->fixup_number++;
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Checks if size of fixups array is sufficient.

    @details    If number of fixups is bigger then or equal to size, it reallocates fixups array.

    @param [in] labels_array        Labels structure pointer.

    @return Error code

======================================================================================================
*/
asm_error_t check_fixup_size(labels_array_t *labels_array) {
    C_ASSERT(labels_array != NULL, return ASM_INPUT_ERROR);

    if(labels_array->fixup_number < labels_array->fixup_size)
        return ASM_SUCCESS;

    fixup_t *new_fixup = (fixup_t *)_recalloc(labels_array->fixup,
                                              labels_array->fixup_size,
                                              labels_array->fixup_size +
                                              fixups_init_size,
                                              sizeof(fixup_t));
    if(new_fixup == NULL) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while reallocating memory to fixups.\r\n");
        return ASM_MEMORY_ALLOCATING_ERROR;
    }

    labels_array->fixup      =  new_fixup;
    labels_array->fixup_size += fixups_init_size;
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Checks if string is label.

    @details    Returns true if there is ':' in string.

    @param [in] command             The string, with read command.

    @return Error code

======================================================================================================
*/
bool is_label(char *command) {
    C_ASSERT(command != NULL, return false);

    char *colon_pointer = strchr(command, ':');
    if(colon_pointer != NULL)
        return true;

    return false;
}

/**
======================================================================================================
    @brief      Fills missing code elements.

    @details    Runs throw array of fixups and writes label instruction pointer value,
                using pointer which is expected to be after compilation in labels array.

    @param [in] labels_array        Labels structure pointer.

    @return Error code

======================================================================================================
*/
asm_error_t do_fixups(labels_array_t *labels_array) {
    C_ASSERT(labels_array != NULL, return ASM_NULL_CODE);

    for(size_t index = 0; index < labels_array->fixup_number; index++) {
        command_t *code_pointer = labels_array->fixup[index].code_element;

        size_t     label_number = labels_array->fixup[index].label_number;
        label_t *  label        = labels_array->labels + label_number;

        asm_error_t error_code = paste_label_ip(label, code_pointer);
        if(error_code != ASM_SUCCESS)
            return error_code;
    }
    return ASM_SUCCESS;
}

/**
======================================================================================================
    @brief      Sets value of label in code.

    @details    Pastes label instruction pointer to the code_pointer cell.
                Does not move instruction pointer.
                Sets sizeof(address_t) bytes.

    @param [in] label               Label to fill in.
    @param [in] code_pointer        Cell of code to paste label in.

    @return Error code

======================================================================================================
*/
asm_error_t paste_label_ip(label_t   *label,
                           command_t *code_pointer) {
    if(memcpy(code_pointer, &label->label_ip, sizeof(address_t)) != code_pointer) {
        color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                     "Error while adding argument to code.\r\n"
                     "Label: %s\r\n",
                     label->label_name);
        return ASM_MEMSET_ERROR;
    }
    return ASM_SUCCESS;
}
