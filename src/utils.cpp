#include <math.h>

#include "utils.h"

size_t file_size(FILE *file) {
    fseek(file, 0, 2);
    size_t size = ftell(file);
    fseek(file, 0, 0);
    return size;
}

int file_print_double(FILE *output, void *item) {
    return fprintf(output, "%lg", *(double *)item);
}

bool is_equal_double(double first, double second) {
    const double epsilon = 10e-9;
    if(fabs(first - second) < epsilon)
        return true;

    return false;
}
