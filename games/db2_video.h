#ifndef DB2_VIDEO_H
#define DB2_VIDEO_H
#include "game_video.h"
#include "video/genesis_vdp.h"
extern const GameVideo db2_video;
void db2_video_observe(unsigned pc);
void db2_video_reset(void);
int db2_video_title_active(const GVDP *);
void db2_video_visor_offset(int tiles);
void db2_video_free_hud(int enabled);
int db2_video_visor_requested(void);
/* Diagnostic only (see evidence/db2-visor-research/coherence.md step 1):
 * producer_epoch counts completed cache/gather producer passes (0x15220
 * entries preceded by a 0x146C0 dirty mark); producer_dirty live-checks
 * whether one is currently in progress. Frame accept/reject behavior does
 * not yet use these. */
unsigned db2_video_producer_epoch(void);
int db2_video_producer_dirty(void);
#endif
