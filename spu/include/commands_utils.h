#ifndef COMMANDS_UTILS_H
#define COMMANDS_UTILS_H

#include "spu_facilities.h"
#include "spu_commands.h"

bool       is_below          (argument_t first,
                              argument_t second);
bool       is_below_or_equal (argument_t first,
                              argument_t second);
bool       is_above          (argument_t first,
                              argument_t second);
bool       is_above_or_equal (argument_t first,
                              argument_t second);
bool       is_equal          (argument_t first,
                              argument_t second);
bool       is_not_equal      (argument_t first,
                              argument_t second);
argument_t add_values        (argument_t first,
                              argument_t second);
argument_t sub_values        (argument_t first,
                              argument_t second);
argument_t mul_values        (argument_t first,
                              argument_t second);
argument_t div_values        (argument_t first,
                              argument_t second);

#endif
