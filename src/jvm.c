#include "parser/class/metadata.h"
#include "jvm/jvm_decompile.h"
#include "common/str_tools.h"
#include "decompiler/klass.h"
#include "jar/jar.h"
#include "file_tools.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

typedef struct jd_opt {
    char *path;
    char *out;
    unsigned char *magic;
    int option;
    int thread_num;
} jd_opt;

static unsigned char* magic_of_file(char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[garlic] open file: %s failed\n", filepath);
        return NULL;
    }
    
    unsigned char *magic = calloc(1, 4);
    if (fread(magic, 1, 4, fp) != 4) {
        if (feof(fp))
            fprintf(stderr, "[garlic] file: %s less than 4 bytes\n", filepath);
        else
            fprintf(stderr, "[garlic] read file: %s error\n", filepath);
        return NULL;
    }
    fclose(fp);

    if (magic[0] == 0xCA && magic[1] == 0xFE && magic[2] == 0xBA && magic[3] == 0xBE) {
        DEBUG_PRINT("valid java class file\n");
    }
    else if (magic[0] == 0x50 && magic[1] == 0x4B && magic[2] == 0x03 && magic[3] == 0x04) {
        DEBUG_PRINT("valid jar file\n");
    }
    else if (magic[0] == 0x64 && magic[1] == 0x65 && magic[2] == 0x78 && magic[3] == 0x0A) {
        DEBUG_PRINT("valid dex file\n");
    }
    else {
        fprintf(stderr, "[garlic] file: %s is not a valid Java class/JAR/DEX file\n", filepath);
        free(magic);
        return NULL;
    }

    return magic;
}

static bool is_jvm_class(jd_opt *opt)
{
    return (opt->magic[0] == 0xCA && opt->magic[1] == 0xFE &&
            opt->magic[2] == 0xBA && opt->magic[3] == 0xBE);
}

static bool is_jar_file(jd_opt *opt)
{
    return (opt->magic[0] == 0x50 && opt->magic[1] == 0x4B &&
            opt->magic[2] == 0x03 && opt->magic[3] == 0x04);
}

static bool is_dex_file(jd_opt *opt)
{
    return (opt->magic[0] == 0x64 && opt->magic[1] == 0x65 &&
            opt->magic[2] == 0x78 && opt->magic[3] == 0x0A);
}

static void prepare_opt_output(jd_opt *opt) {
    if (!opt) {
        return;
    }
    
    char *out = opt->out;
    
    if (out == NULL) {
        if (!opt->path) {
            return;
        }
        
        char *last_slash = strrchr(opt->path, '/');
        char *jar_name;
        
        if (last_slash != NULL) {
            // Extract filename after the last slash
            jar_name = malloc(strlen(last_slash + 1) + 1);
            strcpy(jar_name, last_slash + 1);
        } else {
            // No slash found, use the entire path as filename
            jar_name = malloc(strlen(opt->path) + 1);
            strcpy(jar_name, opt->path);
        }
        
        str_replace_char(jar_name, '.', '_');

        // Create a copy of the path for dirname (dirname may modify the string)
        char *path_copy = malloc(strlen(opt->path) + 1);
        strcpy(path_copy, opt->path);
        char *jar_dir = dirname(path_copy);
        
        if (!jar_dir || !jar_name) {
            free(jar_name);
            free(path_copy);
            return;
        }
        
        size_t out_len = strlen(jar_dir) + strlen(jar_name) + 2;
        out = malloc(out_len);
        if (!out) {
            free(jar_name);
            free(path_copy);
            return;
        }
        
        snprintf(out, out_len, "%s/%s", jar_dir, jar_name);
        
        free(jar_name);
        free(path_copy);
        opt->out = out;
    }
    
    mkdir_p(out);
}

static void prepare_opt_threads(jd_opt *opt) {
    if (opt->thread_num == 0) {
        opt->thread_num = 4;
    }
    else if (opt->thread_num < 2) {
        opt->thread_num = 1;
    }
    else if (opt->thread_num > 16) {
        opt->thread_num = 16; // Limit to a maximum of 16 threads
    }
}

static void opt_usage(const char *progname) {
    fprintf(stderr, "Usage: %s file [-p] [-o outpath] [-t num]\n", progname);
    fprintf(stderr, "    -p: like javap or dexdump, print class info\n");
    fprintf(stderr, "    -o: output path for jar/dex/war files\n");
    fprintf(stderr, "    -t: number of threads to use (default is 4)\n");
}

static jd_opt* parse_opt(int argc, char **argv) {
    // fprintf(stderr, "DEBUG: parse_opt started, argc=%d\n", argc);
    // fflush(stderr);
    
    int oc;
    optind = 2;
    opterr = 0; // Disable getopt error messages

    // fprintf(stderr, "DEBUG: checking argv[1]\n");
    // fflush(stderr);
    
    char *path = argv[1];
    if (path == NULL || (path != NULL && (STR_EQL(path, "-h") || STR_EQL(path, "--help")))) {
        opt_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }
    
    // fprintf(stderr, "DEBUG: calling magic_of_file with path: %s\n", path);
    // fflush(stderr);
    
    unsigned char *magic = magic_of_file(path);
    if (magic == NULL)
        exit(EXIT_FAILURE);

    // fprintf(stderr, "DEBUG: allocating jd_opt\n");
    // fflush(stderr);
    
    jd_opt *opt = malloc(sizeof(jd_opt));
    opt->magic = magic;
    opt->path = path;
    opt->out = NULL;  // Initialize to NULL
    opt->option = 0;  // Initialize to 0
    opt->thread_num = 0;  // Initialize to 0

    while ((oc = getopt(argc, argv, "po:t:h")) != -1) {
        // fprintf(stderr, "DEBUG: processing option %c\n", oc);
        // fflush(stderr);
        
        switch (oc) {
            case 'p': { // like javap
                opt->option = 1;
                break;
            }
            case 'o': {
                opt->out = malloc(strlen(optarg) + 1);
                strcpy(opt->out, optarg);
                opt->out[strlen(optarg)] = '\0';  // Use strlen(optarg) instead of strlen(opt->out)
                break;
            }
            case 't': {
                opt->thread_num = atoi(optarg);
                break;
            }
            case '?': {
                if (optopt == 'o') {
                    fprintf(stderr, "[garlic] Option -%c requires a output path.\n", optopt);
                    fprintf(stderr, "    example: %s %s -o [output path]\n", argv[0], path);
                    fprintf(stderr, "    if there is no -o option, "
                                    "the default output directory for "
                                    "jar/dex/war is the same "
                                    "level directory as the file\n"
                                    "    class's will be output to stdout\n");
                }
                else if (optopt == 't' && !is_jvm_class(opt)) {
                    fprintf(stderr, "[garlic] Option -%c requires a number of threads count.\n", optopt);
                    fprintf(stderr, "    example: %s %s -t [thread count]\n", argv[0], path);
                    fprintf(stderr, "    if there is no -t option, "
                                    "the default number of threads depends "
                                    "on the number of CPUs.\n    if the "
                                    "number of threads is set to less than 2, "
                                    "multithreading mode will be turned off\n");
                }
                break;
            }
            default:
                opt_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    // fprintf(stderr, "DEBUG: parse_opt completed successfully\n");
    // fflush(stderr);
    
    return opt;
}

static void free_opt(jd_opt *opt) {
    if (opt->magic != NULL) {
        free(opt->magic);
    }
    if (opt->out != NULL) {
        free(opt->out);
    }
    free(opt);
}

static void run_for_jvm_class(jd_opt *opt) {
    mem_init_pool();
    jclass_file *jc = parse_class_file(opt->path);
    if (opt->option == 1) {
        print_java_class_file_info(jc);
    }
    else {
        jvm_analyse_class_file(jc->jfile);
    }
    mem_free_pool();
}

static void run_for_jvm_jar(jd_opt *opt) {
    // fprintf(stderr, "DEBUG: run_for_jvm_jar started\n");
    // fflush(stderr);
    
    // fprintf(stderr, "DEBUG: calling prepare_opt_output\n");
    // fflush(stderr);
    
    prepare_opt_output(opt);
    
    // fprintf(stderr, "DEBUG: calling prepare_opt_threads\n");
    // fflush(stderr);
    
    prepare_opt_threads(opt);
    
    // fprintf(stderr, "DEBUG: printing analysis info\n");
    // fflush(stderr);
    
    printf("[Garlic] JAR file analysis\n");
    printf("File     : %s\n", opt->path);
    printf("Save to  : %s\n", opt->out);
    printf("Thread   : %d\n", opt->thread_num);
    
    // fprintf(stderr, "DEBUG: calling jar_file_analyse\n");
    // fflush(stderr);
    
    jar_file_analyse(opt->path, opt->out, opt->thread_num);
    printf("\n[Done]\n");
}

int main(int argc, char **argv)
{
    // fprintf(stderr, "DEBUG: main started, argc=%d\n", argc);
    
    jd_opt *opt = parse_opt(argc, argv);
    // fprintf(stderr, "DEBUG: parse_opt completed\n");

    // fprintf(stderr, "DEBUG: checking if is_jvm_class\n");
    fflush(stderr);
    
    if (is_jvm_class(opt)) {
        // fprintf(stderr, "DEBUG: running for jvm class\n");
        run_for_jvm_class(opt);
        free_opt(opt);
    }
    else {
        // fprintf(stderr, "DEBUG: checking if is_jar_file\n");
        fflush(stderr);
        
        if (is_jar_file(opt)) {
            // fprintf(stderr, "DEBUG: running for jar file\n");
            fflush(stderr);
            
            run_for_jvm_jar(opt);
            free_opt(opt);
        }
        else if (is_dex_file(opt)) {
            fprintf(stderr, "[garlic] DEX file is not supported for open source version yet.\n");
            fprintf(stderr, "         Please contact the author on github\n");

            free_opt(opt);
            exit(EXIT_FAILURE);
        }
        else {
            fprintf(stderr, "[garlic] Unsupported file type: %s\n", opt->path);
            free_opt(opt);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}