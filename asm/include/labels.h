#ifndef LABELS_H
#define LABELS_H

#include <stdio.h>
#include <stdint.h>

#include "asm_errors.h"
#include "spu_facilities.h"

static const size_t max_label_name_size = 32;

struct fixup_t;
struct label_t;

struct labels_array_t {
    label_t *labels;
    size_t   labels_number;
    size_t   labels_size;
    fixup_t *fixup;
    size_t   fixup_number;
    size_t   fixup_size;
};

bool        is_label                       (char           *command);
asm_error_t code_labels_init               (labels_array_t *labels_array);
asm_error_t get_label_instruction_pointer  (labels_array_t *labels_array,
                                            char           *label_name,
                                            code_element_t *code_label_pointer);
asm_error_t do_fixups                      (labels_array_t *labels_array);
asm_error_t code_add_label                 (labels_array_t *labels_array,
                                            char           *label_name,
                                            address_t       instruction_pointer,
                                            bool            is_defined);

#endif
