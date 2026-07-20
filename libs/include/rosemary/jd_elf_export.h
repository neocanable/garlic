#ifndef GARLIC_JD_ELF_EXPORT_H
#define GARLIC_JD_ELF_EXPORT_H

typedef struct jd_elf jd_elf;

#if defined(_MSC_VER) // Windows (MSVC)
#ifdef EXPORTING_ROSEMARY
        #define ROSEMARY_API __declspec(dllexport)
    #else
        #define ROSEMARY_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__) || defined(__clang__) // Linux / macOS
#ifdef EXPORTING_ROSEMARY
#define ROSEMARY_API __attribute__((visibility("default")))
#else
#define ROSEMARY_API
#endif
#else
#define ROSEMARY_API
#endif

ROSEMARY_API jd_elf* jd_analysis_elf_from_path(string path);

ROSEMARY_API jd_elf* jd_analysis_elf_from_buffer(u1 *buffer, size_t size);

ROSEMARY_API void jd_dump_all_csv(jd_elf *elf);

#endif //GARLIC_JD_ELF_EXPORT_H
