#ifndef GARLIC_OUTPUT_ERROR_H
#define GARLIC_OUTPUT_ERROR_H

#include <stdio.h>

#include "common/types.h"

void output_close(FILE *stream, string path);

void output_open_failed(string path);

bool output_had_errors(void);

bool output_report(void);

#endif //GARLIC_OUTPUT_ERROR_H
