#ifndef GARLIC_JAR_H
#define GARLIC_JAR_H
#include "parser/class/metadata.h"
#include "common/str_tools.h"
#include <unistd.h>
#include <libgen.h>

void jar_status(jd_jar *jar);

void jar_file_analyse(string path, string save_path, int thread_cnt);

jsource_file* jar_entry_analyse(jd_jar *jar,
                                jd_jar_entry *entry,
                                jsource_file *parent);

jsource_file* jar_entry_anonymous_analyse(jd_jar *jar,
                                          jd_jar_entry *entry,
                                          jsource_file *parent);

#endif //GARLIC_JAR_H
