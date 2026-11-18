#ifndef DYNA_ACHIEVEMENT_SMOKE_H
#define DYNA_ACHIEVEMENT_SMOKE_H
#include <stdint.h>
void dyna_achievement_smoke_init(void);
void dyna_achievement_smoke_frame(uint64_t frame);
void dyna_achievement_smoke_render(uint32_t *pixels,int width,int height,int pitch);
#endif
