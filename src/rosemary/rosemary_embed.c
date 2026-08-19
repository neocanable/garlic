#include "rosemary/rosemary_embed.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------
 * Platform-specific includes and helpers
 * --------------------------------------------------------------- */
#ifdef _WIN32
  #include <windows.h>
  #include <io.h>
  #include <fcntl.h>
  /* mkstemp / unlink equivalents */
  static int mkstemp_win(char *template)
  {
      if (_mktemp_s(template, strlen(template) + 1) != 0)
          return -1;
      HANDLE h = CreateFileA(template, GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
      if (h == INVALID_HANDLE_VALUE)
          return -1;
      int fd = _open_osfhandle((intptr_t)h, _O_BINARY | _O_APPEND);
      return fd;
  }
  #define TMPDIR_VAR "TMP"
  #define PATHSEP   "\\"
#else
  #include <dlfcn.h>
  #include <unistd.h>
  #define TMPDIR_VAR "TMPDIR"
  #define PATHSEP   "/"
#endif

#ifdef ROSEMARYLIB_EMBEDDED
/* ---------------------------------------------------------------
 * Embedded dynamic-library data
 * (generated at build time by: xxd -i -n rosemarylib_data)
 * --------------------------------------------------------------- */
extern const unsigned char rosemarylib_data[];
extern const unsigned int  rosemarylib_data_len;

/* ---------------------------------------------------------------
 * Runtime state
 * --------------------------------------------------------------- */
#ifdef _WIN32
  static HMODULE g_lib_handle = NULL;
#else
  static void   *g_lib_handle = NULL;
#endif

/* Resolved function pointers */
static jd_elf* (*g_real_analysis)(const char *) = NULL;
static void    (*g_real_dump_all)(jd_elf *)     = NULL;

/* ---------------------------------------------------------------
 * Build a temp-file path, write the embedded library to it, then
 * load it with dlopen (or LoadLibrary on Windows).
 * --------------------------------------------------------------- */
static int ensure_library_loaded(void)
{
    if (g_lib_handle)
        return 0;

    /* ---- temp path ------------------------------------------------- */
    char tmp_path[1024];
    const char *tmpdir;

#ifdef _WIN32
    tmpdir = getenv("TEMP");
    if (!tmpdir) tmpdir = "C:\\Windows\\Temp";
    if (snprintf(tmp_path, sizeof(tmp_path), "%s\\garlic_rosemary_XXXXXX",
                 tmpdir) < 0)
        return -1;
    int fd = mkstemp_win(tmp_path);
#else
    tmpdir = getenv("TMPDIR");
    if (!tmpdir) tmpdir = "/tmp";
    if (snprintf(tmp_path, sizeof(tmp_path), "%s/garlic_rosemary_XXXXXX",
                 tmpdir) < 0)
        return -1;
    int fd = mkstemp(tmp_path);
#endif
    if (fd < 0)
        return -1;

    /* ---- write embedded data to the temp file --------------------- */
#ifdef _WIN32
    FILE *fp = _fdopen(fd, "wb");
#else
    FILE *fp = fdopen(fd, "wb");
#endif
    if (!fp) {
#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif
        remove(tmp_path);
        return -1;
    }

    size_t written = fwrite(rosemarylib_data, 1, rosemarylib_data_len, fp);
    fclose(fp);

    if (written != rosemarylib_data_len) {
        remove(tmp_path);
        return -1;
    }

    /* ---- load the library ----------------------------------------- */
#ifdef _WIN32
    g_lib_handle = LoadLibraryA(tmp_path);
    if (!g_lib_handle) {
        fprintf(stderr, "[garlic] Failed to load embedded library "
                        "(error %lu)\n", GetLastError());
        remove(tmp_path);
        return -1;
    }
    g_real_analysis = (jd_elf *(*)(const char *))
                    GetProcAddress(g_lib_handle, "jd_analysis_elf_from_path");
    if (!g_real_analysis) {
        fprintf(stderr, "[garlic] Symbol 'jd_analysis_elf_from_path' "
                        "not found (error %lu)\n", GetLastError());
        FreeLibrary(g_lib_handle);
        g_lib_handle = NULL;
        remove(tmp_path);
        return -1;
    }
    g_real_dump_all = (void (*)(jd_elf *))
                    GetProcAddress(g_lib_handle, "jd_dump_all_csv");
    if (!g_real_dump_all) {
        fprintf(stderr, "[garlic] Symbol 'jd_dump_all_csv' "
                        "not found (error %lu)\n", GetLastError());
        FreeLibrary(g_lib_handle);
        g_lib_handle = NULL;
        remove(tmp_path);
        return -1;
    }
#else
    g_lib_handle = dlopen(tmp_path, RTLD_NOW | RTLD_LOCAL);
    if (!g_lib_handle) {
        fprintf(stderr, "[garlic] Failed to load embedded library: %s\n",
                dlerror());
        remove(tmp_path);
        return -1;
    }
    g_real_analysis = (jd_elf *(*)(const char *))
                    dlsym(g_lib_handle, "jd_analysis_elf_from_path");
    if (!g_real_analysis) {
        fprintf(stderr, "[garlic] Symbol 'jd_analysis_elf_from_path' "
                        "not found: %s\n", dlerror());
        dlclose(g_lib_handle);
        g_lib_handle = NULL;
        remove(tmp_path);
        return -1;
    }
    g_real_dump_all = (void (*)(jd_elf *))
                    dlsym(g_lib_handle, "jd_dump_all_csv");
    if (!g_real_dump_all) {
        fprintf(stderr, "[garlic] Symbol 'jd_dump_all_csv' "
                        "not found: %s\n", dlerror());
        dlclose(g_lib_handle);
        g_lib_handle = NULL;
        remove(tmp_path);
        return -1;
    }
#endif

    /* Keep the temp file — macOS and older Linux need it while the
     * library is mapped.  On modern Linux (kernel ≥ 3.17) we could
     * delete it after dlopen, but keeping it is safe everywhere. */
    return 0;
}
#endif /* ROSEMARYLIB_EMBEDDED */

/* ---------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------- */
jd_elf* jd_analysis_elf_from_path(const char *path)
{
#ifdef ROSEMARYLIB_EMBEDDED
    if (ensure_library_loaded() != 0)
        return NULL;
    return g_real_analysis(path);
#else
    (void)path;
    return NULL; /* ELF analysis unavailable (no embedded library) */
#endif
}

void jd_dump_all_csv(jd_elf *elf)
{
#ifdef ROSEMARYLIB_EMBEDDED
    if (!g_real_dump_all)
        return;
    g_real_dump_all(elf);
#else
    (void)elf;
#endif
}
