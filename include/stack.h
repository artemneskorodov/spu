#ifndef STACK_H
#define STACK_H

#include <stdio.h>

#define STACK_HASH_PROTECTION
#define STACK_CANARY_PROTECTION
#define STACK_WRITE_DUMP

enum stack_error_t {
    STACK_SUCCESS                      = 0 ,
    STACK_UNEXPECTED_ERROR             = 1 ,
    STACK_MEMORY_ERROR                 = 2 ,
    STACK_DUMP_ERROR                   = 3 ,
    STACK_NULL                         = 4 ,
    STACK_NULL_DATA                    = 5 ,
    STACK_EMPTY                        = 6 ,
    STACK_INCORRECT_SIZE               = 7 ,
    STACK_INVALID_CAPACITY             = 8 ,
    STACK_INVALID_INPUT                = 9 ,
    STACK_INVALID_OUTPUT               = 10,
    STACK_INVALID_DATA                 = 11,
    STACK_UNEXPECTED_LEFT_CANARY       = 12,
    STACK_UNEXPECTED_RIGHT_CANARY      = 13,
    STACK_UNEXPECTED_DATA_LEFT_CANARY  = 14,
    STACK_UNEXPECTED_DATA_RIGHT_CANARY = 15,
    STACK_UNEXPECTED_STRUCTURE_HASH    = 16,
    STACK_UNEXPECTED_DATA_HASH         = 17,
};

struct stack_t;

#ifdef STACK_WRITE_DUMP
    #define STACK_WRITE_DUMP_ON(...) __VA_ARGS__
    #define DUMP_INIT(__dump_filename, __var, __print)\
        __dump_filename,                              \
        __FILE_NAME__,                                \
        #__var,                                       \
        __PRETTY_FUNCTION__,                          \
        __LINE__,                                     \
        __print,

    #define STACK_DUMP(__stack_pointer, __error)\
        stack_dump(__stack_pointer,             \
                   __FILE_NAME__,               \
                   __PRETTY_FUNCTION__,         \
                   __LINE__,                    \
                   __error)

    stack_error_t stack_dump(stack_t      *stack,
                             const char   *file_name,
                             const char   *function_name,
                             size_t        line,
                             stack_error_t call_reason);
#else
    #define STACK_WRITE_DUMP_ON(...)
    #define DUMP_INIT(...)
#endif

stack_t *stack_init        (STACK_WRITE_DUMP_ON(const char *dump_filename,
                                                const char *initialized_file,
                                                const char *initialized_varname,
                                                const char *initialized_function,
                                                size_t      initialized_line,
                                                int       (*print_func)(FILE *, void *),)
                            size_t capacity,
                            size_t element_size);

stack_error_t stack_push   (stack_t **stack, void *element);
stack_error_t stack_pop    (stack_t **stack, void *output);
stack_error_t stack_destroy(stack_t **stack);

#endif
