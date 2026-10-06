#include <ctype.h>
#include <pthread.h>
#include <stdio.h>

#include "output_path.h"
#include "common/str_tools.h"
#include "libs/hashmap/hashmap_tools.h"

/* Keyed on the folded path, so a second spelling of it finds the first;
 * the value is the file name that owns it. `resolved` remembers what each
 * requested path was actually written as, which is what a caller asking
 * twice has to get back.
 *
 * Both maps and everything in them come out of `paths` rather than the
 * caller's pool: a lookup happens in the middle of a worker's task, and
 * that task's pool is freed when the task ends - a key allocated from it
 * would be a dangling pointer in here the moment the next class asked. */
static mem_pool *paths;
static hashmap *claimed;
static hashmap *resolved;

static pthread_mutex_t path_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t path_once = PTHREAD_ONCE_INIT;

static void output_path_init(void)
{
    paths = mem_create_pool();
    claimed = hashmap_init_in(paths, (hcmp_fn) s2o_cmp, 0);
    resolved = hashmap_init_in(paths, (hcmp_fn) s2o_cmp, 0);
}

/* A copy in `paths`, lowercased, so that `Foo.java` and `foo.java` land on
 * one key. */
static string folded_copy(string s)
{
    string lower = str_create_in(paths, "%s", s);
    for (char *p = lower; *p != '\0'; ++p)
        *p = (char) tolower((unsigned char) *p);
    return lower;
}

/* `Foo.java` and 2 make `Foo_2.java` - the extension stays last. */
static string suffixed(string name, int n)
{
    char *dot = strrchr(name, '.');
    if (dot == NULL)
        return str_create_in(paths, "%s_%d", name, n);
    return str_create_in(paths, "%.*s_%d%s",
                         (int) (dot - name), name, n, dot);
}

/* The name this file would be written as, claiming it if it is free.
 * Called with the lock held; a returned string outlives the call. */
static string claim_locked(string dir, string name, bool *gave_way)
{
    string exact = str_create("%s/%s", dir, name);
    string done = hget_s2o(resolved, exact);
    if (done != NULL)
        return done;

    string folded = folded_copy(exact);
    if (hget_s2o(claimed, folded) == NULL) {
        string owned = str_create_in(paths, "%s", name);
        hset_s2o(claimed, folded, owned);
        hset_s2o(resolved, str_create_in(paths, "%s", exact), owned);
        return owned;
    }

    /* A class spelled differently already owns this name. The suffix is
     * handed out in the order the names were reserved, and the classes
     * are reserved in the order they are declared, so a given build
     * always resolves a pair the same way. */
    for (int n = 2; ; ++n) {
        string candidate = suffixed(name, n);
        string candidate_key = folded_copy(
                str_create("%s/%s", dir, candidate));
        if (hget_s2o(claimed, candidate_key) == NULL) {
            hset_s2o(claimed, candidate_key, candidate);
            hset_s2o(resolved, str_create_in(paths, "%s", exact), candidate);
            if (gave_way != NULL)
                *gave_way = true;
            return candidate;
        }
    }
}

bool output_path_reserve(string dir, string name)
{
    pthread_once(&path_once, output_path_init);
    bool gave_way = false;
    pthread_mutex_lock(&path_lock);
    claim_locked(dir, name, &gave_way);
    pthread_mutex_unlock(&path_lock);
    return gave_way;
}

string output_path_resolve(string dir, string name)
{
    pthread_once(&path_once, output_path_init);
    pthread_mutex_lock(&path_lock);
    string assigned = claim_locked(dir, name, NULL);
    pthread_mutex_unlock(&path_lock);
    return assigned;
}
