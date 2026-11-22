#pragma once
#include <stddef.h>
#include <stdint.h>
void dyna_progress_runtime_settings(const char *path);
int dyna_progress_runtime_start(unsigned game,const uint8_t *rom,size_t size);
void dyna_progress_runtime_observe(uint32_t pc,uint8_t *ram,size_t size,uint32_t d[8],const uint32_t a[8]);

/* The AOT executor checks this for every guest instruction. Keep the handler
 * call out of the common path and share the same PCs across both games. */
static inline void dyna_progress_runtime_before(uint32_t pc, uint8_t *ram, size_t size,
                                                uint32_t d[8], const uint32_t a[8]) {
    switch (pc) {
    case 0x1950: case 0x1a14: case 0x1c5a4: case 0x1d7b6:
    case 0x1becc: case 0x1dd90: case 0x28f1a: case 0x28f2e:
    case 0x3a38: case 0x6f9a: case 0x4913e: case 0x58ef2:
    case 0x1db7e: case 0x1db88: case 0x1dcec: case 0x1dd1a: case 0x1dce4:
    case 0x1bd6c: case 0x1bd7c: case 0x1c9ce: case 0x1caf6: case 0x1cb0c:
    case 0x1c40: case 0x1c44: case 0x49708: case 0x49908: case 0x49926:
        dyna_progress_runtime_observe(pc, ram, size, d, a);
        break;
    }
}
int dyna_progress_runtime_action(const char *key);
const char *dyna_progress_runtime_status(void);
/* Result-only inline prompt; visibility does not pause guest execution.
 * Optional outputs: selected0 Continue/1 Back to Title and save feedback. */
int dyna_progress_runtime_prompt(unsigned *selected,const char **feedback);
/* 0 hidden, 1 victory, 2 timed hard ending, 3 defeat. */
int dyna_progress_runtime_prompt_kind(void);
int dyna_progress_runtime_take_title_request(void);

uint16_t dyna_progress_runtime_input(unsigned player,uint16_t mask);

/* One-shot DB1 level request, consumed by qualified native title navigation. */
int dyna_progress_runtime_take_navigation_request(void);
void dyna_progress_runtime_navigation_status(const char *text,int cancel);

/* Static, qualified DB1 SHA list: canonical Japanese first, all accepted variants. */
void dyna_progress_runtime_db1_identities(const char *const *hashes,size_t count);
