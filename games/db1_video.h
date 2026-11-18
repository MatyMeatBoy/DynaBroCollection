#ifndef DB1_VIDEO_H
#define DB1_VIDEO_H
#include "game_video.h"
extern const GameVideo db1_video;
void db1_video_observe(unsigned pc);
void db1_video_reset(void);
void db1_video_set_english(int enabled);
/* Requested signed8px tile offset; actual layout clamps to native border/HUD gaps. */
void db1_video_set_visor_offset_tiles(int tiles);
#endif

/* Host-only title extension. -1 clears it; native selection is never written. */
int db1_video_enhanced(void);
void db1_video_set_title_menu_selection(int selected);

/* Language actually applied to this running session, independent of restart preferences. */
int db1_video_english(void);
