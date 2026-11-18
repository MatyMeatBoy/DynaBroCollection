#ifndef DB1_SCENES_H
#define DB1_SCENES_H
#include <stdint.h>
#include "video/genesis_vdp.h"
typedef enum { DB1_SCENE_UNKNOWN, DB1_SCENE_BRANDING, DB1_SCENE_TITLE,
    DB1_SCENE_OPTIONS, DB1_SCENE_PASSWORD, DB1_SCENE_STAGE_INTRO,
    DB1_SCENE_BATTLE, DB1_SCENE_GAMEOVER } Db1Scene;
void db1_scenes_reset(void);
void db1_scenes_observe(unsigned pc);
Db1Scene db1_scenes_kind(void);
/* Presentation only: signed native-screen coordinates, immutable VDP snapshot.
 * Returns packed color[5:0], opaque[6], priority[7] from authored plane B. */
uint8_t db1_scenes_plane_b(const GVDP *, int x, int y);
uint32_t db1_scenes_background(const GVDP *, int x, int y);
uint32_t db1_scenes_background_for(Db1Scene, const GVDP *, int x, int y);
/* Extend only the background; copy the native foreground and center unchanged. */
int db1_scenes_row(const GVDP *, int line, const uint32_t *native, int nw,
                   uint32_t *out, int width);
#endif
