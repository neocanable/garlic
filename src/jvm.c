#include "parser/class/metadata.h"
#include "jvm/jvm_decompile.h"
#include "common/str_tools.h"
#include "decompiler/klass.h"
#include "jar/jar.h"
#include "file_tools.h"
#include <unistd.h>
#include <stdint.h>

// Magic number constants for better readability and maintainability
#define JAVA_CLASS_MAGIC 0xCAFEBABE
#define JAR_FILE_MAGIC   0x504B0304
#define DEX_FILE_MAGIC   0x6465780A

typedef enum {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_JAVA_CLASS,
    FILE_TYPE_JAR,
    FILE_TYPE_DEX
} file_type_t;

typedef struct {
    char *path;
    char *out;
    file_type_t file_type;
    int option;
    int thread_num;
} jd_opt_t;

// More efficient magic number detection using 32-bit comparison
static file_type_t detect_file_type(const char *filepath) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "[garlic] Failed to open file: %s\n", filepath);
        return FILE_TYPE_UNKNOWN;
    }
    
    uint32_t magic = 0;
    size_t bytes_read = fread(&magic, 1, sizeof(magic), fp);
    fclose(fp);
    
    if (bytes_read != sizeof(magic)) {
        if (bytes_read == 0) {
            fprintf(stderr, "[garlic] File is empty: %s\n", filepath);
        } else {
            fprintf(stderr, "[garlic] File too small (< 4 bytes): %s\n", filepath);
        }
        return FILE_TYPE_UNKNOWN;
    }
    
    // Convert to big-endian for consistent comparison
    uint32_t be_magic = ((magic & 0xFF) << 24) | 
                        (((magic >> 8) & 0xFF) << 16) | 
                        (((magic >> 16) & 0xFF) << 8) | 
                        ((magic >> 24) & 0xFF);
    
    switch (be_magic) {
        case JAVA_CLASS_MAGIC:
            DEBUG_PRINT("Detected Java class file\n");
            return FILE_TYPE_JAVA_CLASS;
        case JAR_FILE_MAGIC:
            DEBUG_PRINT("Detected JAR file\n");
            return FILE_TYPE_JAR;
        case DEX_FILE_MAGIC:
            DEBUG_PRINT("Detected DEX file\n");
            return FILE_TYPE_DEX;
        default:
            fprintf(stderr, "[garlic] Unknown file format: %s (magic: 0x%08X)\n", 
                    filepath, be_magic);
            return FILE_TYPE_UNKNOWN;
    }
}

// More efficient output path preparation with better error handling
static int prepare_output_path(jd_opt_t *opt) {
    if (opt->out) {
        return 0; // Output path already specified
    }
    
    // Find the last slash and extract filename
    const char *filename = strrchr(opt->path, '/');
    filename = filename ? filename + 1 : opt->path;
    
    // Calculate required buffer size
    const char *dir = dirname(strdup(opt->path)); // dirname may modify input
    size_t filename_len = strlen(filename);
    size_t dir_len = strlen(dir);
    
    // Allocate buffer for modified filename (replace dots with underscores)
    char *modified_name = malloc(filename_len + 1);
    if (!modified_name) {
        fprintf(stderr, "[garlic] Memory allocation failed\n");
        return -1;
    }
    
    strcpy(modified_name, filename);
    str_replace_char(modified_name, '.', '_');
    
    // Allocate output path buffer
    opt->out = malloc(dir_len + strlen(modified_name) + 2);
    if (!opt->out) {
        free(modified_name);
        fprintf(stderr, "[garlic] Memory allocation failed\n");
        return -1;
    }
    
    sprintf(opt->out, "%s/%s", dir, modified_name);
    free(modified_name);
    
    return mkdir_p(opt->out);
}

// Validate and normalize thread count
static void normalize_thread_count(jd_opt_t *opt) {
    if (opt->thread_num <= 0) {
        opt->thread_num = 4; // Default
    } else if (opt->thread_num == 1) {
        // Keep single-threaded
    } else if (opt->thread_num > 16) {
        opt->thread_num = 16; // Cap at 16
        fprintf(stderr, "[garlic] Thread count capped at 16\n");
    }
    // 2-16 threads are valid as-is
}

static void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s <file> [options]\n", progname);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -p              Print class info (like javap/dexdump)\n");
    fprintf(stderr, "  -o <path>       Output directory for extracted files\n");
    fprintf(stderr, "  -t <count>      Number of threads (1-16, default: 4)\n");
    fprintf(stderr, "  -h, --help      Show this help message\n");
    fprintf(stderr, "\nSupported formats: Java .class, .jar, .dex files\n");
}

static jd_opt_t* parse_arguments(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return NULL;
    }
    
    // Check for help first
    if (STR_EQL(argv[1], "-h") || STR_EQL(argv[1], "--help")) {
        print_usage(argv[0]);
        return NULL;
    }
    
    // Detect file type early
    file_type_t file_type = detect_file_type(argv[1]);
    if (file_type == FILE_TYPE_UNKNOWN) {
        return NULL;
    }
    
    // Allocate and initialize options structure
    jd_opt_t *opt = calloc(1, sizeof(jd_opt_t));
    if (!opt) {
        fprintf(stderr, "[garlic] Memory allocation failed\n");
        return NULL;
    }
    
    opt->path = argv[1];
    opt->file_type = file_type;
    opt->thread_num = 4; // Default
    
    // Parse command line options
    int c;
    optind = 2; // Start after filename
    opterr = 0; // Suppress getopt error messages
    
    while ((c = getopt(argc, argv, "po:t:h")) != -1) {
        switch (c) {
            case 'p':
                opt->option = 1;
                break;
                
            case 'o':
                opt->out = strdup(optarg);
                if (!opt->out) {
                    fprintf(stderr, "[garlic] Memory allocation failed\n");
                    free(opt);
                    return NULL;
                }
                break;
                
            case 't':
                opt->thread_num = atoi(optarg);
                if (opt->thread_num < 0) {
                    fprintf(stderr, "[garlic] Invalid thread count: %s\n", optarg);
                    free(opt);
                    return NULL;
                }
                break;
                
            case 'h':
                print_usage(argv[0]);
                free(opt);
                return NULL;
                
            case '?':
                if (optopt == 'o') {
                    fprintf(stderr, "[garlic] Option -o requires an output path\n");
                } else if (optopt == 't') {
                    fprintf(stderr, "[garlic] Option -t requires a thread count\n");
                } else {
                    fprintf(stderr, "[garlic] Unknown option: -%c\n", optopt);
                }
                print_usage(argv[0]);
                free(opt);
                return NULL;
                
            default:
                print_usage(argv[0]);
                free(opt);
                return NULL;
        }
    }
    
    return opt;
}

static void cleanup_options(jd_opt_t *opt) {
    if (opt) {
        free(opt->out);
        free(opt);
    }
}

static int process_java_class(jd_opt_t *opt) {
    mem_init_pool();
    
    jclass_file *jc = parse_class_file(opt->path);
    if (!jc) {
        fprintf(stderr, "[garlic] Failed to parse class file: %s\n", opt->path);
        mem_free_pool();
        return -1;
    }
    
    if (opt->option == 1) {
        print_java_class_file_info(jc);
    } else {
        jvm_analyse_class_file(jc->jfile);
    }
    
    mem_free_pool();
    return 0;
}

static int process_jar_file(jd_opt_t *opt) {
    if (prepare_output_path(opt) != 0) {
        return -1;
    }
    
    normalize_thread_count(opt);
    
    printf("[Garlic] JAR file analysis\n");
    printf("File     : %s\n", opt->path);
    printf("Output   : %s\n", opt->out);
    printf("Threads  : %d\n", opt->thread_num);
    
    int result = jar_file_analyse(opt->path, opt->out, opt->thread_num);
    
    if (result == 0) {
        printf("\n[Analysis completed successfully]\n");
    } else {
        printf("\n[Analysis failed]\n");
    }
    
    return result;
}

int main(int argc, char **argv) {
    jd_opt_t *opt = parse_arguments(argc, argv);
    if (!opt) {
        return EXIT_FAILURE;
    }
    
    int result = EXIT_SUCCESS;
    
    switch (opt->file_type) {
        case FILE_TYPE_JAVA_CLASS:
            if (process_java_class(opt) != 0) {
                result = EXIT_FAILURE;
            }
            break;
            
        case FILE_TYPE_JAR:
            if (process_jar_file(opt) != 0) {
                result = EXIT_FAILURE;
            }
            break;
            
        case FILE_TYPE_DEX:
            fprintf(stderr, "[garlic] DEX files not supported in open source version\n");
            fprintf(stderr, "         Contact the author on GitHub for more information\n");
            result = EXIT_FAILURE;
            break;
            
        default:
            fprintf(stderr, "[garlic] Unsupported file type\n");
            result = EXIT_FAILURE;
            break;
    }
    
    cleanup_options(opt);
    return result;
}
