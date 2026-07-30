#ifndef GARLIC_REPORT_H
#define GARLIC_REPORT_H

#include "mem_pool.h"
#include "types.h"

/**
 * Generate a data-collection analysis report.
 *
 * Reads from the analyze output at @p analysis_dir (must contain
 * analysis.duckdb, decompiled/, cg/, native_libs/) and writes
 * report.md + report.html into @p report_dir.
 *
 * @param pool         Memory pool for allocations.
 * @param analysis_dir Path to the analyze output directory.
 * @param report_dir   Path where report files will be written.
 * @return Pool-allocated summary string, or error message.
 */
string report_generate(mem_pool *pool, const char *analysis_dir,
                       const char *report_dir);

#endif /* GARLIC_REPORT_H */
