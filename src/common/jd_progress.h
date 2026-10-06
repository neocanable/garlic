#ifndef GARLIC_JD_PROGRESS_H
#define GARLIC_JD_PROGRESS_H


#ifdef __cplusplus
extern "C" {
#endif

    typedef void (*jd_progress_fn)(int done, int total, const char *phase);

    void jd_progress_set(jd_progress_fn fn);
    int  jd_progress_has(void);
    void jd_progress_report(int done, int total, const char *phase);

#ifdef __cplusplus
}
#endif

#endif /* GARLIC_JD_PROGRESS_H */
