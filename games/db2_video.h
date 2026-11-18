#ifndef DB2_VIDEO_H
#define DB2_VIDEO_H
#include "game_video.h"
extern const GameVideo db2_video;
void db2_video_observe(unsigned pc);
void db2_video_reset(void);
void db2_video_visor_offset(int tiles);
int db2_video_visor_requested(void);
#endif
