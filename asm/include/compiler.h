#ifndef COMPILER_H
#define COMPILER_H

#include <stdint.h>

#include "asm.h"
#include "asm_errors.h"

asm_error_t compile_code(code_t *code);

#endif

