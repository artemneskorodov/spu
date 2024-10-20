#include <string.h>
#include <ctype.h>
#include <stdint.h>


#include "labels.h"
#include "custom_assert.h"
#include "colors.h"
#include "memory.h"

static const size_t labels_init_size    = 32;
static const size_t fixups_init_size    = 32;

static asm_error_t try_find_label(labels_array_t *labels_array,
                                  char           *label_name,
                                  uint64_t       *code_label_pointer);
static asm_error_t add_fix_up                     (labels_array_t *labels_array,
                                                   size_t          label_number,
                                                   uint64_t       *code_label_pointer);
static asm_error_t check_labels_size              (labels_array_t *labels_array);
static asm_error_t check_fixup_size               (labels_array_t *labels_array);

struct label_t {
    char        label_name[max_label_name_size];
    size_t      label_ip;
    bool        is_defined;
};

struct fixup_t {
    size_t      label_number;
    uint64_t   *code_element;
};

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

asm_error_t try_find_label(labels_array_t *labels_array,
                           char           *label_name,
                           uint64_t       *code_label_pointer) {
    C_ASSERT(labels_array       != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(label_name         != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(code_label_pointer != NULL, return ASM_INPUT_ERROR);
    for(size_t index = 0; index < labels_array->labels_number; index++) {
        if(strcmp(labels_array->labels[index].label_name, label_name) == 0) {
            if(labels_array->labels[index].is_defined) {
                *code_label_pointer = labels_array->labels[index].label_ip;
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

asm_error_t get_label_instruction_pointer(labels_array_t *labels_array,
                                          char           *label_name,
                                          uint64_t       *code_label_pointer) {
    C_ASSERT(labels_array       != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(label_name         != NULL, return ASM_INPUT_ERROR);
    C_ASSERT(code_label_pointer != NULL, return ASM_INPUT_ERROR);

    asm_error_t error_code = ASM_SUCCESS;
    error_code = try_find_label(labels_array,
                                label_name,
                                code_label_pointer);

    if(error_code == ASM_SUCCESS)
        return ASM_SUCCESS;

    if(error_code != ASM_NO_LABEL)
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

asm_error_t code_add_label(labels_array_t *labels_array,
                           char           *label_name,
                           size_t          instruction_pointer,
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

asm_error_t add_fix_up(labels_array_t *labels_array,
                       size_t          label_number,
                       uint64_t       *code_label_pointer) {
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

bool is_label(char *command) {
    C_ASSERT(command != NULL, return false);

    char *colon_pointer = strchr(command, ':');
    if(colon_pointer != NULL &&
       *(colon_pointer + 1) == '\0')
        return true;

    return false;
}

asm_error_t do_fixups(labels_array_t *labels_array) {
    C_ASSERT(labels_array != NULL, return ASM_NULL_CODE);

    for(size_t index = 0; index < labels_array->fixup_number; index++) {
        uint64_t *code_pointer = labels_array->fixup[index].code_element;
        size_t    label_number = labels_array->fixup[index].label_number;
        size_t    label_value  = labels_array->labels[label_number].label_ip;

        *code_pointer = label_value;
    }
    return ASM_SUCCESS;
}
