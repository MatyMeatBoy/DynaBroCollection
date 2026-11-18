#pragma once
#include <stddef.h>
#include <wchar.h>

typedef struct DynaLauncherOptions {
    int game; /* 0 = picker, 1 = db1, 2 = db2 */
    int no_launcher, setup, fullscreen, help, integration_info;
    const wchar_t *rom, *widescreen; /* borrowed argv values */
} DynaLauncherOptions;

/* argv includes executable name. Unknown or contradictory options fail. */
int dyna_launcher_options_parse(int argc, const wchar_t *const *argv,
    DynaLauncherOptions *out, wchar_t *error, size_t error_count);
/* Writable CreateProcessW command line, including quoted executable name.
 * Always pass exe separately as lpApplicationName; no shell is used. */
int dyna_launcher_options_command(const DynaLauncherOptions *options,
    const wchar_t *exe, wchar_t *out, size_t count);
const wchar_t *dyna_launcher_options_help(void);
const char *dyna_launcher_integration_info(void);
