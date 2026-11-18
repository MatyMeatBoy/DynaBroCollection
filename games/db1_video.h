#ifndef DB1_VIDEO_H
#define DB1_VIDEO_H
#include "game_video.h"
extern const GameVideo db1_video;
void db1_video_observe(unsigned pc);
void db1_video_reset(void);
void db1_video_set_english(int enabled);
#endif
