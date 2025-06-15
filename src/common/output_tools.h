#ifndef GARLIC_OUTPUT_TOOLS_H
#define GARLIC_OUTPUT_TOOLS_H

#include <stdio.h>

static void inline backspace(int size) {
    for (int i = 0; i < size; ++i) {
        printf("\b");
    }
}

#endif //GARLIC_OUTPUT_TOOLS_H
