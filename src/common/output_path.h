#ifndef GARLIC_OUTPUT_PATH_H
#define GARLIC_OUTPUT_PATH_H

#include "common/types.h"

bool output_path_reserve(string dir, string name);

string output_path_resolve(string dir, string name);

#endif //GARLIC_OUTPUT_PATH_H
