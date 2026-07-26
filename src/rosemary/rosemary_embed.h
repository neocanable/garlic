#ifndef GARLIC_ROSEMARY_EMBED_H
#define GARLIC_ROSEMARY_EMBED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration — the full struct is defined inside the embedded library */
typedef struct jd_elf jd_elf;

/**
 * Analyze an ELF binary from a file path.
 *
 * On the first call, the embedded librosemarylib is extracted to a
 * temporary file and loaded with dlopen().  Subsequent calls reuse
 * the already-loaded library.
 *
 * @param path  Path to an ELF binary file.
 * @return      Opaque jd_elf handle, or NULL on failure (library could
 *              not be extracted / loaded, or the binary could not be parsed).
 */
jd_elf* jd_analysis_elf_from_path(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* GARLIC_ROSEMARY_EMBED_H */
