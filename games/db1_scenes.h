#ifndef DB1_SCENES_H
#define DB1_SCENES_H
#include <stdint.h>
#include "video/genesis_vdp.h"
typedef enum { DB1_SCENE_UNKNOWN, DB1_SCENE_BRANDING, DB1_SCENE_TITLE,
    DB1_SCENE_OPTIONS, DB1_SCENE_PASSWORD, DB1_SCENE_STAGE_INTRO,
    DB1_SCENE_BATTLE, DB1_SCENE_GAMEOVER, DB1_SCENE_VICTORY,
    DB1_SCENE_VICTORY_RESULT } Db1Scene;
void db1_scenes_reset(void);
void db1_scenes_observe(unsigned pc);
Db1Scene db1_scenes_kind(void);
int db1_scenes_has_background(Db1Scene);
/* Presentation only: signed native-screen coordinates, immutable VDP snapshot.
 * Returns packed color[5:0], opaque[6], priority[7] from authored plane B. */
uint8_t db1_scenes_plane_b(const GVDP *, int x, int y);
/* Options/password backdrop writer emits a 16-glyph sequence across39
 * columns, then a blank row; phase advances7 glyphs per pair of rows.
 * Repeat that authored128x256 pattern from untouched rows0/1, excluding
 * the later panel fill. Uses live scroll, patterns, palette and flips. */
uint8_t db1_scenes_menu_background_b(const GVDP *, int x, int y);
/* Unwrapped authored plane coordinates before name-table wrap. Uses the same
 * nearest native raster/column scroll contract as the plane sampler. */
int db1_scenes_plane_b_coordinates(const GVDP *, int x, int y,
                                  int *source_x, int *source_y);
uint32_t db1_scenes_background(const GVDP *, int x, int y);
uint32_t db1_scenes_background_for(Db1Scene, const GVDP *, int x, int y);
/* Extend only the background; copy the native foreground and center unchanged. */
int db1_scenes_row(const GVDP *, int line, const uint32_t *native, int nw,
                   uint32_t *out, int width);
#endif
