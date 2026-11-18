#ifndef DYNA_MANUAL_DATA_H
#define DYNA_MANUAL_DATA_H
#include <stddef.h>

typedef struct {
    int page;
    char image[160];
    char *title;       /* malloc'd; NULL if the JSON value was null */
    char *translation; /* malloc'd; empty string if the field was missing */
} DynaManualPage;

typedef struct {
    DynaManualPage *pages;
    size_t count;
} DynaManualBook;

/* Parses the page-array schema this project's manual-translation JSON files
 * use: [{"page":N,"image":"...","title":"..."|null,"translation":"..."}, ...].
 * Not a general-purpose JSON parser. Returns 1 and fills *out on success
 * (caller must dyna_manual_free it); returns 0 on any malformed input and
 * leaves *out zeroed. */
int dyna_manual_load_text(const char *json_text, DynaManualBook *out);
int dyna_manual_load_file(const char *path, DynaManualBook *out);
void dyna_manual_free(DynaManualBook *book);

#endif
