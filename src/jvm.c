#include "parser/class/metadata.h"
#include "jvm/jvm_decompile.h"
#include "common/str_tools.h"
#include "decompiler/klass.h"
#include "jar/jar.h"
#include "file_tools.h"
#include <unistd.h>

#define JAVA_CLASS_MAGIC 0xCAFEBABE
#define JAR_FILE_MAGIC   0x504B0304
#define DEX_FILE_MAGIC   0x6465780A

typedef enum {
    JD_FILE_TYPE_UNKNOWN = 0,
    JD_FILE_TYPE_JAVA_CLASS,
    JD_FILE_TYPE_JAR,
    JD_FILE_TYPE_DEX
} jd_file_type_t;

typedef struct jd_opt {
    char *path;
    char *out;
    jd_file_type_t ft;
    int option;
    int thread_num;
} jd_opt;

static jd_file_type_t magic_of_file(char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[garlic] Open file: %s failed\n", filepath);
        return JD_FILE_TYPE_UNKNOWN;
    }
    uint32_t magic = 0;
    size_t bytes_read = fread(&magic, 1, sizeof(magic), fp);
    if (bytes_read != sizeof(magic)) {
        fprintf(stderr, "[garlic] File %s read error.\n", filepath);
        return JD_FILE_TYPE_UNKNOWN;
    }
    fclose(fp);

    uint32_t be_magic = ((magic & 0xFF) << 24) |
                        (((magic >> 8) & 0xFF) << 16) |
                        (((magic >> 16) & 0xFF) << 8) |
                        ((magic >> 24) & 0xFF);

    switch (be_magic) {
        case JAVA_CLASS_MAGIC:
            return JD_FILE_TYPE_JAVA_CLASS;
        case JAR_FILE_MAGIC:
            return JD_FILE_TYPE_JAR;
        case DEX_FILE_MAGIC:
            return JD_FILE_TYPE_DEX;
        default:
            fprintf(stderr, "[garlic] file: %s is not a "
                            "valid Java class/JAR/DEX file\n", filepath);
            return JD_FILE_TYPE_UNKNOWN;
    }
}

static inline bool is_jvm_class(jd_opt *opt)
{
    return opt->ft == JD_FILE_TYPE_JAVA_CLASS;
}

static bool is_jar_file(jd_opt *opt)
{
    return opt->ft == JD_FILE_TYPE_JAR;
}

static bool is_dex_file(jd_opt *opt)
{
    return opt->ft == JD_FILE_TYPE_DEX;
}

static void prepare_opt_output(jd_opt *opt) {
    char *out = opt->out;
    if (out == NULL) {
        char *copy_path = strdup(opt->path);
        char *jar_name = strdup(basename(opt->path));
        char *jar_dir = dirname(copy_path);
        str_replace_char(jar_name, '.', '_');

        out = malloc(strlen(jar_dir) + strlen(jar_name) + 2);
        sprintf(out, "%s/%s", jar_dir, jar_name);
        free(jar_name);
        free(copy_path);
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
    int oc;
    optind = 2;
    opterr = 0; // Disable getopt error messages

    char *path = argv[1];
    if (path == NULL || ((STR_EQL(path, "-h") || STR_EQL(path, "--help")))) {
        opt_usage(argv[0]);
        exit(EXIT_SUCCESS);
    }
    jd_file_type_t ft = magic_of_file(path);
    if (ft == JD_FILE_TYPE_UNKNOWN)
        exit(EXIT_FAILURE);

    jd_opt *opt = malloc(sizeof(jd_opt));
    memset(opt, 0, sizeof(jd_opt));
    opt->path = path;
    opt->ft = ft;

    while ((oc = getopt(argc, argv, "po:t:h")) != -1) {
        switch (oc) {
            case 'p': { // like javap
                opt->option = 1;
                break;
            }
            case 'o': {
                opt->out = optarg;
                opt->out = malloc(strlen(optarg) + 1);
                strcpy(opt->out, optarg);
                opt->out[strlen(opt->out)] = '\0';
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
    return opt;
}

static void free_opt(jd_opt *opt) {
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
    prepare_opt_output(opt);
    prepare_opt_threads(opt);
    printf("[Garlic] JAR file analysis\n");
    printf("File     : %s\n", opt->path);
    printf("Save to  : %s\n", opt->out);
    printf("Thread   : %d\n", opt->thread_num);
    jar_file_analyse(opt->path, opt->out, opt->thread_num);
    printf("\n[Done]\n");
}

int main(int argc, char **argv)
{
    jd_opt *opt = parse_opt(argc, argv);

    if (is_jvm_class(opt)) {
        run_for_jvm_class(opt);
        free_opt(opt);
    }
    else if (is_jar_file(opt)) {
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

    return 0;
}