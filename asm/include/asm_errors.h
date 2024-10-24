#ifndef ASM_ERRORS_H
#define ASM_ERRORS_H

enum asm_error_t {
    ASM_SUCCESS                 = 0 ,
    ASM_EXIT_SUCCESS            = 1 ,
    ASM_NULL_CODE               = 2 ,
    ASM_NO_INPUT_FILES          = 3 ,
    ASM_FLAGS_ERROR             = 4 ,
    ASM_OPENING_FILE_ERROR      = 5 ,
    ASM_MEMORY_ALLOCATING_ERROR = 6 ,
    ASM_READING_ERROR           = 7 ,
    ASM_READING_COMMAND_ERROR   = 8 ,
    ASM_UNEXPECTED_ERROR        = 9 ,
    ASM_INPUT_ERROR             = 10,
    ASM_SYNTAX_ERROR            = 11,
    ASM_REGISTER_ERROR          = 12,
    ASM_UNEXPECTED_PARAMETER    = 13,
    ASM_LABEL_ERROR             = 14,
    ASM_WRITING_FILE_ERROR      = 15,
    ASM_NO_LABEL                = 16,
    ASM_UNABLE_READ_ARGUMENT    = 17,
    ASM_MEMSET_ERROR            = 18,
};

#endif
