#ifndef GARLIC_METADATA_H
#define GARLIC_METADATA_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "parser/class/class_structure.h"
#include "parser/class/class_tools.h"
#include "common/str_tools.h"
#include "hashmap_tools.h"

void parse_class_finish(jclass_file* jclass);

jclass_file* parse_class_file(const char* path);

jclass_file* parse_class_content_from_jar_entry(jd_jar_entry *e);

jclass_file* parse_class_content(string, string, size_t);

void parse_constant_pool_section(jclass_file*);

void parse_interfaces_section(jclass_file*);

void parse_fields_section(jclass_file*);

jattr* parse_attributes_section(jclass_file*, uint16_t);

void parse_methods_section(jclass_file*);

void print_code_section(jclass_file*, jattr_code*);

void print_java_class_file_info(jclass_file*);

void init_java_class_content(jclass_file *jc, const char *path);

jsource_file* init_java_source_file(jclass_file *jc);

#endif //GARLIC_METADATA_H
