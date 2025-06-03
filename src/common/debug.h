#ifndef GARLIC_DEBUG_H
#define GARLIC_DEBUG_H

#ifndef DEBUG
#define DEBUG                       false
#endif

#ifndef DEBUG_PRINT
#define DEBUG_PRINT(FMT, ARGS...) do {                  \
    if ( DEBUG )                                        \
    fprintf(stdout, "" FMT "", ## ARGS);                \
} while (0)
#endif

#ifndef DEBUG_PRINT_BASIC_BLOCK
#define DEBUG_PRINT_BASIC_BLOCK     true
#endif

#ifndef DEBUG_INS_AND_NODE_INFO
#define DEBUG_INS_AND_NODE_INFO     false
#endif

#ifndef DEBUG_WRITE_COLOR
#define DEBUG_WRITE_COLOR           true
#endif

#ifndef DEBUG_STACK
#define DEBUG_STACK                 false
#endif

#ifndef DEBUG_STACK_PRINT
#define DEBUG_STACK_PRINT(FMT, ARGS...) do {            \
    if ( DEBUG_STACK )                                  \
    fprintf(stdout, "" FMT "", ## ARGS);                \
} while (0)
#endif

#ifndef DEBUG_GOTO_OPTIMIZE
#define DEBUG_GOTO_OPTIMIZE false
#endif

#ifndef DEBUG_GOTO_OPTIMIZE_PRINT
#define DEBUG_GOTO_OPTIMIZE_PRINT(FMT, ARGS...) do {    \
    if ( DEBUG_GOTO_OPTIMIZE )                          \
    fprintf(stdout, "" FMT "", ## ARGS);                \
} while (0)
#endif

#ifndef DEBUG_TERNARY
#define DEBUG_TERNARY false
#endif

#ifndef DEBUG_TERNARY_PRINT
#define DEBUG_TERNARY_PRINT(FMT, ARGS...) do {          \
    if (DEBUG_TERNARY)                                  \
    fprintf(stdout, "" FMT "", ## ARGS);                \
} while (0)
#endif

#ifndef DEBUG_EXCEPTION
#define DEBUG_EXCEPTION             false
#endif

#ifndef DEBUG_EXCEPTION_PRINT
#define DEBUG_EXCEPTION_PRINT(FMT, ARGS...) do {        \
    if (DEBUG_EXCEPTION)                                \
    fprintf(stdout, "" FMT "", ## ARGS);                \
} while (0)
#endif

#ifndef DEBUG_ERROR
#define DEBUG_ERROR                 false
#endif

#ifndef DEBUG_ERROR_PRINT
#define DEBUG_ERROR_PRINT(FMT, ARGS...) do {            \
    if (DEBUG_ERROR)                                    \
    fprintf(stdout, "" FMT "", ## ARGS);                \
} while (0)
#endif

#ifndef DEBUG_ANNOTATION
#define DEBUG_ANNOTATION           false
#endif

#ifndef DEBUG_ANNO_PRINT
#define DEBUG_ANNO_PRINT(FMT, ARGS...) do {       \
    if (DEBUG_ANNOTATION)                               \
    fprintf(stdout, "" FMT "", ## ARGS);                \
} while (0)
#endif

#ifndef DEBUG_RENAME_VARIABLES
#define DEBUG_RENAME_VARIABLES     true
#endif

#ifndef SOURCE_FILE_NOTICE
#define SOURCE_FILE_NOTICE         true
#endif
#endif //GARLIC_DEBUG_H

