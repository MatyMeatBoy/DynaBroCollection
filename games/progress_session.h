#ifndef DYNA_PROGRESS_SESSION_H
#define DYNA_PROGRESS_SESSION_H
#include "progress_store.h"
typedef struct {
    DynaProgressStore *store;
    const uint8_t *rom;size_t rom_size;
    DynaProgressCode current;
    int has_current;
} DynaProgressSession;
int dyna_progress_session_init(DynaProgressSession *,DynaProgressStore *,const uint8_t *rom,size_t rom_size);
void dyna_progress_session_reset(DynaProgressSession *);
/* Call only in a qualified game/ROM session. A contains 68000 A0..A7.
 * No allocation, IO, CPU execution or guest mutation occurs here. */
void dyna_progress_session_before(DynaProgressSession *,uint32_t pc,
    const uint8_t *ram,size_t ram_size,const uint32_t a[8]);
const DynaProgressCode *dyna_progress_session_current(const DynaProgressSession *);
/* Explicit save action; memory and previous file remain valid on failure. */
int dyna_progress_session_save(DynaProgressSession *,const char *path);
unsigned dyna_progress_session_count(const DynaProgressSession *);
const DynaProgressCode *dyna_progress_session_item(const DynaProgressSession *,unsigned index);
/* UTF-8/ASCII display of the original code. Output capacity at least 9. */
int dyna_progress_session_text(const DynaProgressSession *,const DynaProgressCode *,char text[9]);
#endif
