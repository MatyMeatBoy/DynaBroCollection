#pragma once
#include <stddef.h>
#include <stdint.h>

#define DYNA_LANGUAGE_VARIANTS 8
typedef struct {
    size_t address, length;
    const uint8_t *bytes[DYNA_LANGUAGE_VARIANTS];
} DynaLanguageSpan;
typedef struct {
    size_t size;
    unsigned variants;
    uint32_t crc[DYNA_LANGUAGE_VARIANTS]; /* 0 Japanese, 1 current English */
    const DynaLanguageSpan *spans;
    size_t count;
} DynaLanguageImage;

uint32_t dyna_language_crc(const uint8_t *data, size_t size);
/* Returns 1 on success; rejected input/metadata leaves ROM untouched. */
int dyna_language_apply(uint8_t *rom, size_t size, const DynaLanguageImage *image,
                        int english, char *error, size_t error_size);
