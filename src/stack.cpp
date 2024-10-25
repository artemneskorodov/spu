#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "stack.h"
#include "memory.h"
#include "colors.h"
#include "custom_assert.h"

//==============================================================================
//PROTECTION MODES ON
//==============================================================================
#define STACK_HASH_PROTECTION
#define STACK_CANARY_PROTECTION
#define STACK_WRITE_DUMP

//==============================================================================
//OPERATIONS WITH STACK
//==============================================================================
enum stack_operation_t {
    STACK_OPERATION_PUSH,
    STACK_OPERATION_POP ,
};

//==============================================================================
//MACRO TO DESTROY STACK AND RETURN ERROR
//==============================================================================
#define STACK_RETURN_ERROR(__stack_pointer, __return_value) {\
    stack_destroy(&(__stack_pointer));                       \
    return (__return_value);                                 \
}

//==============================================================================
//MACRO TO CHECK IF STACK SIZE IS SUFFICIENT AND EXPAND IT IF NEEDED
//==============================================================================
#define STACK_CHECK_SIZE(__stack_pointer, __operation) {            \
    stack_error_t __error_code = stack_check_size((__stack_pointer),\
                                                  (__operation));   \
    if((__error_code) != STACK_SUCCESS)                             \
        STACK_RETURN_ERROR(*(__stack_pointer), (__error_code));     \
}

//==============================================================================
//CHECK IF STACK IS VALID, WRITE DUMP AND RETURN ERROR IF NOT
//==============================================================================
#define STACK_VERIFY(__stack_pointer) {                        \
    stack_error_t __error_code = stack_verify(__stack_pointer);\
    if(__error_code != STACK_SUCCESS) {                        \
        STACK_DUMP((__stack_pointer), (__error_code));         \
        stack_destroy(&(__stack_pointer));                     \
        return (__error_code);                                 \
    }                                                          \
}

//==============================================================================
//FUNCTIONS PROTOTYPES
//==============================================================================
static stack_error_t stack_check_size(stack_t **        stack,
                                      stack_operation_t operation);
static stack_error_t stack_verify    (stack_t *stack);

//==============================================================================
//STACK WRITE DUMP MODE
//==============================================================================
#ifdef STACK_WRITE_DUMP
    static const char *TEXT_STACK_SUCCESS                      = "STACK_SUCCESS"                     ;
    static const char *TEXT_STACK_UNEXPECTED_ERROR             = "STACK_UNEXPECTED_ERROR"            ;
    static const char *TEXT_STACK_MEMORY_ERROR                 = "STACK_MEMORY_ERROR"                ;
    static const char *TEXT_STACK_DUMP_ERROR                   = "STACK_DUMP_ERROR"                  ;
    static const char *TEXT_STACK_NULL                         = "STACK_NULL"                        ;
    static const char *TEXT_STACK_NULL_DATA                    = "STACK_NULL_DATA"                   ;
    static const char *TEXT_STACK_EMPTY                        = "STACK_EMPTY"                       ;
    static const char *TEXT_STACK_INCORRECT_SIZE               = "STACK_INCORRECT_SIZE"              ;
    static const char *TEXT_STACK_INVALID_CAPACITY             = "STACK_INVALID_CAPACITY"            ;
    static const char *TEXT_STACK_INVALID_INPUT                = "STACK_INVALID_INPUT"               ;
    static const char *TEXT_STACK_INVALID_OUTPUT               = "STACK_INVALID_OUTPUT"              ;
    static const char *TEXT_STACK_INVALID_DATA                 = "STACK_INVALID_DATA"                ;
    static const char *TEXT_STACK_UNEXPECTED_LEFT_CANARY       = "STACK_UNEXPECTED_LEFT_CANARY"      ;
    static const char *TEXT_STACK_UNEXPECTED_RIGHT_CANARY      = "STACK_UNEXPECTED_RIGHT_CANARY"     ;
    static const char *TEXT_STACK_UNEXPECTED_DATA_LEFT_CANARY  = "STACK_UNEXPECTED_DATA_LEFT_CANARY" ;
    static const char *TEXT_STACK_UNEXPECTED_DATA_RIGHT_CANARY = "STACK_UNEXPECTED_DATA_RIGHT_CANARY";
    static const char *TEXT_STACK_UNEXPECTED_STRUCTURE_HASH    = "STACK_UNEXPECTED_STRUCTURE_HASH"   ;
    static const char *TEXT_STACK_UNEXPECTED_DATA_HASH         = "STACK_UNEXPECTED_DATA_HASH"        ;

    static const char *  get_error_text           (stack_error_t error);
    static stack_error_t stack_write_members      (stack_t *stack);
    static stack_error_t write_stack_members_flags(stack_t *stack);
#else
    #define STACK_DUMP(...)
#endif

//==============================================================================
//PROTECTION OF STACK WITH HASH MODE
//==============================================================================
#ifdef STACK_HASH_PROTECTION
    typedef uint64_t hash_t;

    #define STACK_UPDATE_HASH(__stack_pointer) {                        \
        stack_error_t __error_code = stack_update_hash(__stack_pointer);\
        if(__error_code != STACK_SUCCESS)                               \
            STACK_RETURN_ERROR(__stack_pointer, __error_code);          \
    }

    static stack_error_t stack_update_hash     (stack_t *stack);
    static stack_error_t stack_calculate_hashes(stack_t *stack,
                                                hash_t * structure_hash,
                                                hash_t * data_hash);
    static hash_t        hash_function         (const void *start,
                                                const void *end);
    static stack_error_t stack_verify_hashes   (stack_t *stack);
#else
    #define STACK_UPDATE_HASH(__stack_pointer)
#endif

//==============================================================================
//PROTECTION OF STACK WITH CANARIES MODE
//==============================================================================
#ifdef STACK_CANARY_PROTECTION
    typedef uint64_t canary_t;

    const canary_t CANARY_HEX_SPEAK = 0xC0FFEEC0FFEE;

    #define STACK_UPDATE_CANARY(__stack_pointer) {                        \
        stack_error_t __error_code = stack_update_canary(__stack_pointer);\
        if(__error_code != STACK_SUCCESS)                                 \
            STACK_RETURN_ERROR(__stack_pointer, __error_code);            \
    }

    static stack_error_t stack_update_canary       (stack_t *stack);
    static size_t        calculate_alignment_offset(size_t capacity,
                                                    size_t element_size);
    static stack_error_t stack_verify_canaries     (stack_t *stack);
#else
    #define STACK_UPDATE_CANARY(__stack_pointer)
#endif

//==============================================================================
//THE DEFINITION OF STACK STRUCTURE
//==============================================================================
struct stack_t {
    #ifdef STACK_CANARY_PROTECTION
        canary_t  structure_left_canary;
        canary_t *data_left_canary;
        canary_t *data_right_canary;
        size_t    alignment_offset;
    #endif

    #ifdef STACK_HASH_PROTECTION
        hash_t structure_hash;
        hash_t data_hash;
    #endif

    #ifdef STACK_WRITE_DUMP
        FILE *      dump_file;
        const char *dump_filename;
        const char *initialized_file;
        const char *initialized_varname;
        const char *initialized_function;
        size_t      initialized_line;
        int       (*print_func)(FILE *, void *);
    #endif

    size_t size;
    size_t capacity;
    size_t init_capacity;
    size_t element_size;
    char * data;

    #ifdef STACK_CANARY_PROTECTION
        canary_t structure_right_canary;
    #endif
};

//==============================================================================
//GLOBAL FUNCTION
//==============================================================================

//------------------------------------------------------------------------------
//INITIALIZES STACK
//------------------------------------------------------------------------------
stack_t *stack_init(STACK_WRITE_DUMP_ON(const char *dump_filename,
                                        const char *initialized_file,
                                        const char *initialized_varname,
                                        const char *initialized_function,
                                        size_t      initialized_line,
                                        int       (*print_func)(FILE *, void *),)
                    size_t capacity,
                    size_t element_size) {
    C_ASSERT(element_size != 0, return NULL);
    #ifdef STACK_WRITE_DUMP
        C_ASSERT(dump_filename        != NULL, return NULL);
        C_ASSERT(initialized_file     != NULL, return NULL);
        C_ASSERT(initialized_varname  != NULL, return NULL);
        C_ASSERT(initialized_function != NULL, return NULL);
        C_ASSERT(print_func           != NULL, return NULL);
    #endif

    size_t allocation_size = sizeof(stack_t) + capacity * element_size;

    #ifdef STACK_CANARY_PROTECTION
        size_t alignment_offset = calculate_alignment_offset(capacity, element_size);
        allocation_size += alignment_offset + 2 * sizeof(canary_t);
    #endif

    stack_t *stack = (stack_t *)_calloc(allocation_size, 1);
    if(stack == NULL)
        return NULL;

    stack->capacity      = capacity    ;
    stack->element_size  = element_size;
    stack->init_capacity = capacity    ;
    stack->data = (char *)stack + sizeof(stack_t);

    #ifdef STACK_CANARY_PROTECTION
        stack->data              = stack->data + sizeof(canary_t);
        stack->alignment_offset  = alignment_offset;
    #endif

    #ifdef STACK_WRITE_DUMP
        stack->dump_filename        = dump_filename       ;
        stack->initialized_file     = initialized_file    ;
        stack->initialized_varname  = initialized_varname ;
        stack->initialized_function = initialized_function;
        stack->initialized_line     = initialized_line    ;
        stack->print_func           = print_func          ;

        stack->dump_file = fopen(stack->dump_filename, "wb");
        if(stack->dump_file == NULL) {
            stack_destroy(&stack);
            return NULL;
        }
    #endif

    #ifdef STACK_HASH_PROTECTION
        if(stack_update_hash(stack) != STACK_SUCCESS) {
            stack_destroy(&stack);
            return NULL;
        }
    #endif

    #ifdef STACK_CANARY_PROTECTION
        if(stack_update_canary(stack) != STACK_SUCCESS) {
            stack_destroy(&stack);
            return NULL;
        }
    #endif

    if(stack_verify(stack) != STACK_SUCCESS) {
        stack_destroy(&stack);
        return NULL;
    }
    return stack;
}

//------------------------------------------------------------------------------
//PUSHES ELEMENT IN STACK
//------------------------------------------------------------------------------
stack_error_t stack_push(stack_t **stack, void *element) {
    C_ASSERT(stack   != NULL, return STACK_NULL         );
    C_ASSERT(element != NULL, return STACK_INVALID_INPUT);

    STACK_VERIFY(*stack);
    STACK_CHECK_SIZE(stack, STACK_OPERATION_PUSH);

    char *stack_storage = (*stack)->data +
                          (*stack)->element_size *
                          (*stack)->size;
    if(memcpy(stack_storage,
              element,
              (*stack)->element_size) != stack_storage)
        STACK_RETURN_ERROR(*stack, STACK_MEMORY_ERROR);

    (*stack)->size++;

    STACK_UPDATE_HASH  (*stack);
    STACK_UPDATE_CANARY(*stack);
    STACK_VERIFY       (*stack);
    return STACK_SUCCESS;
}

//------------------------------------------------------------------------------
//POPS ELEMENT FROM STACK, WRITES ELEMENT TO OUTPUT
//------------------------------------------------------------------------------
stack_error_t stack_pop(stack_t **stack, void *output) {
    C_ASSERT(stack  != NULL, return STACK_NULL          );
    C_ASSERT(output != NULL, return STACK_INVALID_OUTPUT);

    STACK_VERIFY(*stack);
    STACK_CHECK_SIZE(stack, STACK_OPERATION_POP);

    size_t allocated_size = sizeof(stack_t) + (*stack)->capacity * (*stack)->element_size;

    #ifdef STACK_CANARY_PROTECTION
        allocated_size += 2 * sizeof(canary_t) + (*stack)->alignment_offset;
    #endif

    if(output > (*stack) &&
       output < (*stack) + allocated_size)
        return STACK_INVALID_OUTPUT;

    if((*stack)->size == 0)
        return STACK_EMPTY;

    (*stack)->size--;
    char *stack_storage = (*stack)->data +
                          (*stack)->size *
                          (*stack)->element_size;
    if(memcpy(output,
              stack_storage,
              (*stack)->element_size) != output)
        STACK_RETURN_ERROR(*stack, STACK_MEMORY_ERROR);

    if(memset(stack_storage,
              0,
              (*stack)->element_size) != stack_storage)
        STACK_RETURN_ERROR(*stack, STACK_MEMORY_ERROR);

    STACK_UPDATE_HASH  (*stack);
    STACK_UPDATE_CANARY(*stack);
    STACK_VERIFY       (*stack);
    return STACK_SUCCESS;
}

//------------------------------------------------------------------------------
//DESTROYS STACK
//------------------------------------------------------------------------------
stack_error_t stack_destroy(stack_t **stack) {
    C_ASSERT(stack != NULL, return STACK_NULL);

    if(*stack == NULL)
        return STACK_SUCCESS;

    STACK_WRITE_DUMP_ON(fclose((*stack)->dump_file));
    _free(*stack);
    _memory_destroy_log();

    *stack = NULL;
    return STACK_SUCCESS;
}

//==============================================================================
//STATIC FUNCTIONS
//==============================================================================

//------------------------------------------------------------------------------
//CHECKS IF SIZE OF STACK IS SUFFICIENT
//------------------------------------------------------------------------------
stack_error_t stack_check_size(stack_t **        stack,
                               stack_operation_t operation) {
    if(stack == NULL)
        return STACK_NULL;

    STACK_VERIFY(*stack);

    size_t new_capacity = 0;

    switch(operation) {
        case STACK_OPERATION_PUSH: {
            if((*stack)->size < (*stack)->capacity)
                return STACK_SUCCESS;
            new_capacity = (*stack)->capacity * 2;
            break;
        }
        case STACK_OPERATION_POP:  {
            if((*stack)->size % 4      != 0 ||
               (*stack)->size * 4      >= (*stack)->capacity ||
               (*stack)->init_capacity == (*stack)->capacity)
                return STACK_SUCCESS;
            new_capacity = (*stack)->capacity / 4;
            break;
        }
        default:                   {
            return STACK_UNEXPECTED_ERROR;
        }
    }

    #ifdef STACK_CANARY_PROTECTION
        size_t offset = calculate_alignment_offset(new_capacity,
                                                   (*stack)->element_size);
    #endif

    size_t old_size = sizeof(stack_t) +
                      (*stack)->capacity *
                      (*stack)->element_size;
    size_t new_size = sizeof(stack_t) +
                      new_capacity *
                      (*stack)->element_size;


    #ifdef STACK_CANARY_PROTECTION
        old_size += 2 * sizeof(canary_t) + (*stack)->alignment_offset;
        new_size += 2 * sizeof(canary_t) + offset;
    #endif

    stack_t *new_stack = (stack_t *)_recalloc(*stack, old_size, new_size, 1);
    if(new_stack == NULL)
        return STACK_MEMORY_ERROR;

    #ifdef STACK_CANARY_PROTECTION
        if(operation == STACK_OPERATION_PUSH) {
            canary_t *old_canary = (canary_t *)((char *)(new_stack + 1) +
                                                sizeof(canary_t) +
                                                new_stack->capacity *
                                                new_stack->element_size);
            *old_canary = 0;
        }
    #endif

    *stack = new_stack;
    new_stack->capacity = new_capacity           ;
    new_stack->data     = (char *)(new_stack + 1);

    #ifdef STACK_CANARY_PROTECTION
        new_stack->data             += sizeof(canary_t);
        new_stack->alignment_offset =  offset          ;
    #endif

    STACK_UPDATE_HASH  (*stack);
    STACK_UPDATE_CANARY(*stack);
    STACK_VERIFY       (*stack);
    return STACK_SUCCESS;
}

//------------------------------------------------------------------------------
//CHECKS IF STACK IS VALID
//------------------------------------------------------------------------------
stack_error_t stack_verify(stack_t *stack) {
    if(stack == NULL)
        return STACK_NULL;

    if(stack->data == NULL)
        return STACK_NULL_DATA;

    if(stack->size > stack->capacity)
        return STACK_INCORRECT_SIZE;

    #ifdef STACK_CANARY_PROTECTION
        if(stack->data != (char *)(stack + 1) + sizeof(canary_t))
            return STACK_INVALID_DATA;

        stack_error_t canary_state = stack_verify_canaries(stack);
        if(canary_state != STACK_SUCCESS)
            return canary_state;
    #else
        if(stack->data != stack + 1)
            return STACK_INVALID_DATA;
    #endif

    #ifdef STACK_HASH_PROTECTION
        stack_error_t hash_state = stack_verify_hashes(stack);
        if(hash_state != STACK_SUCCESS)
            return hash_state;
    #endif

    #ifdef STACK_WRITE_DUMP
        if(stack->dump_file == NULL)
            return STACK_DUMP_ERROR;
    #endif

    return STACK_SUCCESS;
}

//==============================================================================
//STACK WRITE DUMP MODE FUNCTIONS DEFINITION
//==============================================================================
#ifdef STACK_WRITE_DUMP
    //------------------------------------------------------------------------------
    //WRITES STACK INFORMATION IN DUMP FILE
    //------------------------------------------------------------------------------
    stack_error_t stack_dump(stack_t *stack,
                             const char *file_name,
                             const char *function_name,
                             size_t line,
                             stack_error_t call_reason) {
        if(stack->dump_file == NULL) {
            color_printf(RED_TEXT, BOLD_TEXT, DEFAULT_BACKGROUND,
                         "MEMORY DUMP FILE ERROR\r\n"
                         "called from: %s:%llu\r\n",
                         file_name,
                         line);
            return STACK_DUMP_ERROR;
        }

        if(fprintf(stack->dump_file,
                   "stack_t[0x%p] initialized in %s:%llu as "
                   "'stack_t %s' in function '%s'\r\n"
                   "dump called from %s:%llu '%s'\r\n"
                   "ERROR = ",
                   stack,
                   stack->initialized_file,
                   stack->initialized_line,
                   stack->initialized_varname,
                   stack->initialized_function,
                   file_name,
                   line,
                   function_name) < 0)
            return STACK_DUMP_ERROR;

        const char *error_definition = get_error_text(call_reason);
        if(error_definition == NULL)
            error_definition = "'unknown error'";

        if(fprintf(stack->dump_file,
                   "'%s'\r\n",
                   error_definition) < 0)
            return STACK_DUMP_ERROR;

        if(stack == NULL)
            return STACK_NULL;

        #ifdef STACK_CANARY_PROTECTION
            if(fprintf(stack->dump_file,
                       "{\r\n"
                       "\t\t---CANARIES---\r\n"
                       "\tcanary_left       = 0x%llx;\r\n"
                       "\tdata_canary_left [0x%p] = 0x%llx;\r\n"
                       "\tdata_canary_right[0x%p] = 0x%llx;\r\n"
                       "\tcanary_right      = 0x%llx;\r\n",
                       stack->structure_left_canary,
                       stack->data_left_canary,
                       *(stack->data_left_canary),
                       stack->data_right_canary,
                       *(stack->data_right_canary),
                       stack->structure_right_canary) < 0)
                return STACK_DUMP_ERROR;
        #endif

        #ifdef STACK_HASH_PROTECTION
            if(fprintf(stack->dump_file,
                       "\t\t---HASHES---\r\n"
                       "\tstructure_hash    = 0x%llx;\r\n"
                       "\tdata_hash         = 0x%llx;\r\n",
                       stack->structure_hash,
                       stack->data_hash) < 0)
                return STACK_DUMP_ERROR;
        #endif

        if(fprintf(stack->dump_file,
                   "\t\t---DEFAULT_INFO---\r\n"
                   "\tsize              =   %llu;\r\n"
                   "\tcapacity          =   %llu;\r\n"
                   "\telement_size      =   %llu;\r\n"
                   "\t\t---MEMBERS---\r\n"
                   "\tdata[0x%p]:\r\n",
                   stack->size,
                   stack->capacity,
                   stack->element_size,
                   stack->data) < 0)
            return STACK_DUMP_ERROR;

        stack_error_t members_writing_state = stack_write_members(stack);
        if(members_writing_state != STACK_SUCCESS)
            return members_writing_state;

        if(fprintf(stack->dump_file,
                   "}\r\n\r\n") < 0)
            return STACK_DUMP_ERROR;

        fflush(stack->dump_file);
        return STACK_SUCCESS;
    }

    //------------------------------------------------------------------------------
    //WRITES STACK MEMBERS
    //------------------------------------------------------------------------------
    stack_error_t stack_write_members(stack_t *stack) {
        if(stack->data == NULL          ) {
            if(fprintf(stack->dump_file,
                       "\t\t--- (POISON)\r\n") < 0)
                return STACK_DUMP_ERROR;

            return STACK_SUCCESS;
        }
        if(stack->size > stack->capacity) {
            if(fprintf(stack->dump_file,
                       "\t\tincorrect size\r\n") < 0)
                return STACK_DUMP_ERROR;

            return STACK_SUCCESS;
        }

        stack_error_t printing_error = write_stack_members_flags(stack);
        if(printing_error != STACK_SUCCESS)
            return printing_error;

        return STACK_SUCCESS;
    }

    //------------------------------------------------------------------------------
    //WRITES STACK MEMBERS WITH * BEFORE INDEX AND (POISON) AFTER ELEMENT IF IT IS
    //------------------------------------------------------------------------------
    stack_error_t write_stack_members_flags(stack_t *stack) {
        const char * const POISON_ELEMENT_FLAG = " (POISON)";
        const char * const NORMAL_ELEMENT_FLAG = ""         ;
        const char * const POISON_INDEX_FLAG   = "*"        ;
        const char * const NORMAL_INDEX_FLAG   = " "        ;

        for(size_t element = 0; element < stack->capacity; element++) {
            const char *index_flag = NULL;
            const char *element_flag = NULL;

            if(element < stack->size) {
                index_flag = NORMAL_INDEX_FLAG;
                element_flag = NORMAL_ELEMENT_FLAG;
            }
            else{
                index_flag = POISON_INDEX_FLAG;
                element_flag = POISON_ELEMENT_FLAG;
            }

            if(fprintf(stack->dump_file,
                       "\t   %s[%llu] = ",
                       index_flag,
                       element) < 0)
                return STACK_DUMP_ERROR;

            if(stack->print_func(stack->dump_file,
                                 (char *)stack->data +
                                 element *
                                 stack->element_size) < 0)
                return STACK_DUMP_ERROR;

            if(fprintf(stack->dump_file,
                       "%s;\r\n",
                       element_flag) < 0)
                return STACK_DUMP_ERROR;
        }
        return STACK_SUCCESS;
    }

    //------------------------------------------------------------------------------
    //RETURNS STRING WITH TEXT DEFINITION OF ERROR
    //------------------------------------------------------------------------------
    const char *get_error_text(stack_error_t error) {
        switch(error) {
            case STACK_SUCCESS:
                return TEXT_STACK_SUCCESS;
            case STACK_UNEXPECTED_ERROR:
                return TEXT_STACK_UNEXPECTED_ERROR;
            case STACK_MEMORY_ERROR:
                return TEXT_STACK_MEMORY_ERROR;
            case STACK_DUMP_ERROR:
                return TEXT_STACK_DUMP_ERROR;
            case STACK_NULL:
                return TEXT_STACK_NULL;
            case STACK_NULL_DATA:
                return TEXT_STACK_NULL_DATA;
            case STACK_EMPTY:
                return TEXT_STACK_EMPTY;
            case STACK_INCORRECT_SIZE:
                return TEXT_STACK_INCORRECT_SIZE;
            case STACK_INVALID_CAPACITY:
                return TEXT_STACK_INVALID_CAPACITY;
            case STACK_INVALID_INPUT:
                return TEXT_STACK_INVALID_INPUT;
            case STACK_INVALID_OUTPUT:
                return TEXT_STACK_INVALID_OUTPUT;
            case STACK_INVALID_DATA:
                return TEXT_STACK_INVALID_DATA;
            case STACK_UNEXPECTED_LEFT_CANARY:
                return TEXT_STACK_UNEXPECTED_LEFT_CANARY;
            case STACK_UNEXPECTED_RIGHT_CANARY:
                return TEXT_STACK_UNEXPECTED_RIGHT_CANARY;
            case STACK_UNEXPECTED_DATA_LEFT_CANARY:
                return TEXT_STACK_UNEXPECTED_DATA_LEFT_CANARY;
            case STACK_UNEXPECTED_DATA_RIGHT_CANARY:
                return TEXT_STACK_UNEXPECTED_DATA_RIGHT_CANARY;
            case STACK_UNEXPECTED_STRUCTURE_HASH:
                return TEXT_STACK_UNEXPECTED_STRUCTURE_HASH;
            case STACK_UNEXPECTED_DATA_HASH:
                return TEXT_STACK_UNEXPECTED_DATA_HASH;
            default:
                return NULL;
        }
    }
#endif

//==============================================================================
//STACK CANARY PROTECTION MODE FUNCTIONS DEFINITION
//==============================================================================
#ifdef STACK_CANARY_PROTECTION
    //------------------------------------------------------------------------------
    //I AM WRITING THIS STACK WITH RESPECT TO THE CODE CULTURE
    //THIS CODE COUNTS CANARIES AND WRITES THEM INTO STACK STRUCTURE
    //I HANDLE ERRORS AND WRITE DUMPS BY DEFAULT
    //AND AT END I WILL GIVE AN AWFUL SEGFAULT
    //
    //DEBUG
    //AS A HELL'S HUG
    //DEBUG
    //AS A HELL'S HUG
    //------------------------------------------------------------------------------
    stack_error_t stack_update_canary(stack_t *stack) {
        stack->data_left_canary  = (canary_t *)((char *)stack +
                                                sizeof(stack_t));
        stack->data_right_canary = (canary_t *)((char *)stack +
                                                sizeof(stack_t) +
                                                sizeof(canary_t) +
                                                stack->capacity *
                                                stack->element_size +
                                                stack->alignment_offset);

        *(stack->data_left_canary ) = (canary_t)stack->data ^ CANARY_HEX_SPEAK;
        *(stack->data_right_canary) = (canary_t)stack->data ^ CANARY_HEX_SPEAK;

        stack->structure_left_canary  = (canary_t)stack ^ CANARY_HEX_SPEAK;
        stack->structure_right_canary = (canary_t)stack ^ CANARY_HEX_SPEAK;
        return STACK_SUCCESS;
    }

    //------------------------------------------------------------------------------
    //RETURNS OFFSET WHICH IS NEEDED TO ALIGN RIGHT DATA CANARY
    //------------------------------------------------------------------------------
    size_t calculate_alignment_offset(size_t capacity,
                                      size_t element_size) {
        return (sizeof(canary_t) -
                capacity *
                element_size %
                sizeof(canary_t)) % sizeof(canary_t);
    }

    //------------------------------------------------------------------------------
    //CHECKS IF CURRENT CANARIES ARE SAME AS WRITTEN IN STACK STRUCTURE
    //------------------------------------------------------------------------------
    stack_error_t stack_verify_canaries(stack_t *stack) {
        if(stack->structure_left_canary  != ((canary_t)stack ^
                                             CANARY_HEX_SPEAK))
            return STACK_UNEXPECTED_LEFT_CANARY;

        if(stack->structure_right_canary != ((canary_t)stack ^
                                             CANARY_HEX_SPEAK))
            return STACK_UNEXPECTED_RIGHT_CANARY;

        if(*(stack->data_left_canary ) != ((canary_t)stack->data ^
                                           CANARY_HEX_SPEAK))
            return STACK_UNEXPECTED_DATA_LEFT_CANARY;

        if(*(stack->data_right_canary) != ((canary_t)stack->data ^
                                           CANARY_HEX_SPEAK))
            return STACK_UNEXPECTED_DATA_RIGHT_CANARY;

        return STACK_SUCCESS;
    }
#endif

//==============================================================================
//STACK HASH PROTECTION MODE FUNCTIONS DEFINITION
//==============================================================================
#ifdef STACK_HASH_PROTECTION
    //------------------------------------------------------------------------------
    //FUNCTION UPDATES STACK HASHES
    //------------------------------------------------------------------------------
    stack_error_t stack_update_hash(stack_t *stack) {
        stack_error_t error_code = stack_calculate_hashes(stack,
                                                          &stack->structure_hash,
                                                          &stack->data_hash);
        if(error_code != STACK_SUCCESS)
            return error_code;

        return STACK_SUCCESS;
    }

    //------------------------------------------------------------------------------
    //FUNCTION WRITES HASHES OF STRUCTURE AND DATA
    //------------------------------------------------------------------------------
    stack_error_t stack_calculate_hashes(stack_t *stack,
                                         hash_t * structure_hash,
                                         hash_t * data_hash) {
        if(stack == NULL)
            return STACK_NULL;

        *structure_hash = hash_function(&stack->size,
                                        &stack->data + 1);

        *data_hash      = hash_function(stack->data,
                                        stack->data +
                                        stack->capacity *
                                        stack->element_size);
        return STACK_SUCCESS;
    }

    //------------------------------------------------------------------------------
    //HASH FUNCTION djb2, COUNTS HASH FROM START TO END
    //------------------------------------------------------------------------------
    hash_t hash_function(const void *start,
                         const void *end) {
        hash_t hash = 5381;
        const char *bytes_start = (const char *)start;
        for(const char *elem = bytes_start; elem < end; elem++)
            hash = (hash << 5) + hash + *elem;
        return hash;
    }

    //------------------------------------------------------------------------------
    //CHECKS IF CURRENT HASH IS SAME AS WRITTEN IN STACK STRUCTURE
    //------------------------------------------------------------------------------
    stack_error_t stack_verify_hashes(stack_t *stack) {
        hash_t structure_hash = 0,
               data_hash      = 0;

        stack_error_t error_code = stack_calculate_hashes(stack,
                                                          &structure_hash,
                                                          &data_hash);
        if(error_code != STACK_SUCCESS)
            return error_code;

        if(stack->structure_hash != structure_hash)
            return STACK_UNEXPECTED_STRUCTURE_HASH;

        if(stack->data_hash      != data_hash)
            return STACK_UNEXPECTED_DATA_HASH;

        return STACK_SUCCESS;
    }
#endif
