#ifndef DYNA_LAUNCHER_PROFILE_H
#define DYNA_LAUNCHER_PROFILE_H

#include <stdio.h>

/* The runner still builds individual profile file paths in 512-byte buffers. */
#define DYNA_PROFILE_PATH_CAP 448
#define DYNA_LOG_KEEP 5

typedef enum {
    DYNA_PROFILE_UNAVAILABLE, DYNA_PROFILE_PORTABLE,
    DYNA_PROFILE_FALLBACK, DYNA_PROFILE_OVERRIDE
} DynaProfileChoice;

DynaProfileChoice dyna_profile_select(const char *portable, const char *override,
                                     const char *fallback, char *out, size_t capacity);
DynaProfileChoice dyna_profile_resolve(const char *portable, char *out, size_t capacity);
FILE *dyna_log_open(const char *profile, const char *component, char *path, size_t capacity);

#endif
