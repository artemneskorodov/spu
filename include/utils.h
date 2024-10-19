#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

size_t file_size(FILE *file);
int file_print_double(FILE *output, void *item);
bool is_equal_double(double first, double second);

#endif
