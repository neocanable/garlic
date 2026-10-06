#include <errno.h>
#include <pthread.h>
#include <string.h>

#include "output_error.h"
#include "common/str_tools.h"

static pthread_mutex_t error_lock = PTHREAD_MUTEX_INITIALIZER;
static long failed = 0;
static int last_errno = 0;
static string first_path = NULL;

static mem_pool *names = NULL;

static void remember(string path)
{
    if (failed != 0)
        return;

    last_errno = errno;
    if (path == NULL)
        return;

    if (names == NULL)
        names = mem_create_pool();
    first_path = str_create_in(names, "%s", path);
}

static void record(string path)
{
    pthread_mutex_lock(&error_lock);
    remember(path);
    failed++;
    pthread_mutex_unlock(&error_lock);
}

void output_open_failed(string path)
{
    record(path);
}

void output_close(FILE *stream, string path)
{
    if (stream == NULL)
        return;

    int flush_failed = fflush(stream) != 0;
    int close_failed = fclose(stream) != 0;
    if (!flush_failed && !close_failed)
        return;

    record(path);
}

bool output_had_errors(void)
{
    return failed > 0;
}

bool output_report(void)
{
    if (failed == 0)
        return false;

    fprintf(stderr,
            "[garlic] error: %ld file(s) could not be written (%s); the "
            "output is incomplete\n",
            failed,
            last_errno != 0 ? strerror(last_errno) : "unknown error");
    if (first_path != NULL)
        fprintf(stderr, "[garlic] the first was %s\n", first_path);
    return true;
}
