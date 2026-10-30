#define _GNU_SOURCE
#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Host dependency wiring for this supplied Xvfb only. No emulator symbols,
 * executable bytes, clocks, input state or memory mappings are modified. */
#define XVFB_BINARY "/workspace/scratch/1e822aeaeac7/wp1979/tooling/host/usr/bin/Xvfb"
#define XKB_HELPER "/workspace/scratch/1e822aeaeac7/wp1979/tooling/host/usr/bin/xkbcomp"
static char helper_directory[] = "/workspace/scratch/1e822aeaeac7/wp1979/tooling/host/usr/bin";

static void reject(const char *reason)
{
    fprintf(stderr, "WP1980 Xvfb helper redirect refused: %s\n", reason);
    _exit(125);
}

__attribute__((constructor)) static void redirect_xkb_helper(void)
{
    char executable[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", executable, sizeof executable - 1u);
    if (length < 0 || (size_t)length >= sizeof executable - 1u)
        return;
    executable[length] = '\0';
    if (strcmp(executable, XVFB_BINARY))
        return; /* Never act in Mednafen or another executable. */

    /* The launcher supplies only this shim in Xvfb's private environment.
     * Remove exactly that entry before Xvfb execs its ordinary helpers; do
     * not remove or rewrite any other preload an environment may contain. */
    Dl_info own;
    const char *preload = getenv("LD_PRELOAD");
    if (!dladdr((const void *)redirect_xkb_helper, &own) || !own.dli_fname ||
        !preload || strcmp(preload, own.dli_fname))
        reject("expected this shim as the sole explicitly scoped preload");

    char **directory = (char **)dlsym(RTLD_DEFAULT, "XkbBinDirectory");
    if (!directory || !*directory || strcmp(*directory, "/usr/bin"))
        reject("XkbBinDirectory is absent or has an unexpected initial value");
    if (access(XKB_HELPER, R_OK | X_OK))
        reject("the supplied xkbcomp is not readable/executable");
    if (unsetenv("LD_PRELOAD"))
        reject("could not clear this shim for child helpers");

    *directory = helper_directory;
    fprintf(stderr, "WP1980 Xvfb only: XkbBinDirectory=/workspace/scratch/1e822aeaeac7/wp1979/tooling/host/usr/bin; preload removed for children\n");
}
