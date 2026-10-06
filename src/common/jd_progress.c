#include "jd_progress.h"

#include <stddef.h>

static volatile jd_progress_fn jd_progress_cb = NULL;

void jd_progress_set(jd_progress_fn fn) {
    jd_progress_cb = fn;
}

int jd_progress_has(void) {
    return jd_progress_cb != NULL;
}

void jd_progress_report(int done, int total, const char *phase) {
    if (jd_progress_cb)
        jd_progress_cb(done, total, phase);
}
