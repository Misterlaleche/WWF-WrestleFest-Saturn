/* Relocate the ancillary Xvfb keymap compiler path. Never preload into Mednafen. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
__attribute__((constructor)) static void relocate_xkb_compiler(void) {
    const char *path = getenv("WF_XKB_BIN_DIRECTORY");
    const char **directory = (const char **)dlsym(RTLD_DEFAULT, "XkbBinDirectory");
    if (path && directory) *directory = path;
}
