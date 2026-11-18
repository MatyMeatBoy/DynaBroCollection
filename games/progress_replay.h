#ifndef DYNA_PROGRESS_REPLAY_H
#define DYNA_PROGRESS_REPLAY_H
#include "progress_store.h"
typedef enum {
    DYNA_REPLAY_IDLE, DYNA_REPLAY_WAIT_PASSWORD, DYNA_REPLAY_PREFILLED,
    DYNA_REPLAY_SUBMITTED, DYNA_REPLAY_ACCEPTED, DYNA_REPLAY_LOADED,
    DYNA_REPLAY_FAILED, DYNA_REPLAY_CANCELED
} DynaProgressReplayStatus;
/* Abstract PHYSICAL pad buttons; adapter translates these to its pad masks. */
enum { DYNA_REPLAY_PAD_A=1, DYNA_REPLAY_PAD_B=2, DYNA_REPLAY_PAD_C=4 };
typedef struct {
    unsigned game;
    const uint8_t *rom;size_t rom_size;
    DynaProgressCode requested;
    DynaProgressReplayStatus status;
    unsigned elapsed,pulse_frame,confirm_button;
    int supplied;
} DynaProgressReplay;
int dyna_progress_replay_init(DynaProgressReplay *,unsigned game,const uint8_t *rom,size_t rom_size);
/* Code must resolve to a qualified native campaign/special-stage entry.
 * DB1 Alien16 retains its native4 bytes including trailing0x6A blank.
 * Request queues; it never changes the current guest screen or program counter. */
int dyna_progress_replay_request(DynaProgressReplay *,const DynaProgressCode *);
void dyna_progress_replay_cancel(DynaProgressReplay *);
/* Mutable RAM and D/A register arrays refer to the original CPU. Only the
 * password buffer/typed length is changed at documented password-screen hooks.
 * No PC parameter is writable and no game initialization is bypassed. */
void dyna_progress_replay_before(DynaProgressReplay *,uint32_t pc,
    uint8_t *ram,size_t ram_size,uint32_t d[8],const uint32_t a[8]);
/* Call once per guest video frame. Optional DB1 native confirm pulse: when
 * override is1, set player1 pad buttons to buttons (0 means release all).
 * Buttons use the abstract physical masks above. Restore ordinary input after
 * override returns0. DB2 queues until a normal native submit; no unproved
 * automatic controller mapping is used there. Times out after1800 frames. */
void dyna_progress_replay_frame(DynaProgressReplay *,int *override,unsigned *buttons);
const char *dyna_progress_replay_status_text(const DynaProgressReplay *);
#endif
