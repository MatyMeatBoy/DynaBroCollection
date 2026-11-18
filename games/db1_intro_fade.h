#ifndef DB1_INTRO_FADE_H
#define DB1_INTRO_FADE_H
#include <stdint.h>
void db1_intro_fade_reset(void);
void db1_intro_fade_observe(unsigned pc);
int db1_intro_fade_active(void);
/* Presentation only, after native composition. Native height is 224; only the
 * outer sixteen pixels on each side of the centered 320px image are blended.
 * Pass the captured story flag when composing an owned frame snapshot. */
int db1_intro_fade_row(uint32_t *pixels,int width,int native_width,int line,
                       int enhanced,int story,uint32_t backdrop);
#endif
