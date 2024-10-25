#include <math.h>
#include "commands_utils.h"

/**
======================================================================================================
    @brief      Adds first and second.

    @param [in] first               First value to add.
    @param [in] second              Second value to add.

    @return Result of summation

======================================================================================================
*/
argument_t add_values(argument_t first, argument_t second) {
    return second + first;
}

/**
======================================================================================================
    @brief      Subtracts first from second.

    @param [in] first               First value to subtract.
    @param [in] second              Second value to subtract.

    @return Result of subtraction

======================================================================================================
*/
argument_t sub_values(argument_t first, argument_t second) {
    return second - first;
}

/**
======================================================================================================
    @brief      Multiplies first and second.

    @param [in] first               First value to multiply.
    @param [in] second              Second value to multiply.

    @return Product of first and second.

======================================================================================================
*/
argument_t mul_values(argument_t first, argument_t second) {
    return second * first;
}

/**
======================================================================================================
    @brief      Divides second value by first.

    @param [in] first               First value to divide.
    @param [in] second              Second value to divide.

    @return Result of division

======================================================================================================
*/
argument_t div_values(argument_t first, argument_t second) {
    return second / first;
}

/**
======================================================================================================
    @brief      Checks if second value is less than first.

    @param [in] first               First value to compare.
    @param [in] second              Second value to compare.

    @return True if second is less than first

======================================================================================================
*/
bool is_below(argument_t first, argument_t second) {
    if(second < first)
        return true;

    return false;
}

/**
======================================================================================================
    @brief      Checks if second value is less than or equal to first.

    @param [in] first               First value to compare.
    @param [in] second              Second value to compare.

    @return True if second is less than or equal to first

======================================================================================================
*/
bool is_below_or_equal(argument_t first, argument_t second) {
    if(second < first || is_equal(first, second))
        return true;

    return false;
}

/**
======================================================================================================
    @brief      Checks if second value is bigger than first.

    @param [in] first               First value to compare.
    @param [in] second              Second value to compare.

    @return True if second is bigger than first

======================================================================================================
*/
bool is_above(argument_t first, argument_t second) {
    if(second > first)
        return true;

    return false;
}

/**
======================================================================================================
    @brief      Checks if second value is bigger than or equal to first.

    @param [in] first               First value to compare.
    @param [in] second              Second value to compare.

    @return True if second is bigger than or equal to first

======================================================================================================
*/
bool is_above_or_equal(argument_t first, argument_t second) {
    if(second > first || is_equal(first, second))
        return true;

    return false;
}

/**
======================================================================================================
    @brief      Checks if second value is equal to first.

    @param [in] first               First value to compare.
    @param [in] second              Second value to compare.

    @return True if second is equal to first

======================================================================================================
*/
bool is_equal(argument_t first, argument_t second) {
    const argument_t epsilon = 10e-6;

    if(fabs(first - second) < epsilon)
        return true;

    return false;
}

/**
======================================================================================================
    @brief      Checks if second value is not equal first.

    @param [in] first               First value to compare.
    @param [in] second              Second value to compare.

    @return True if second is not equal first

======================================================================================================
*/
bool is_not_equal(argument_t first, argument_t second) {
    return !is_equal(first, second);
}
