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

/**
 * Dump ALL analysis data for a previously-analysed ELF/ Mach-O
 * binary to CSV / text files alongside the original binary.
 *
 * Produces per-analysis files at <path>.<suffix>:
 *   .cfg_nodes   — control-flow-graph node list
 *   .cfg_edges   — control-flow-graph edges
 *   .dissembly   — disassembly listing
 *   .entries     — entry points
 *   .exports     — exported symbols
 *   .func_xref   — function cross-references
 *   .strings     — string constants with metadata
 *   .pc_xrefs    — string / PC cross-references
 *   .imports     — imported symbols
 *
 * These files can be imported into DuckDB for SQL analysis.
 *
 * @param elf  The jd_elf handle returned by jd_analysis_elf_from_path().
 */
void jd_dump_all_csv(jd_elf *elf);

#ifdef __cplusplus
}
#endif

#endif /* GARLIC_ROSEMARY_EMBED_H */
