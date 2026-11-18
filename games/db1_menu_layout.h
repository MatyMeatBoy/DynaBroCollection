#ifndef DB1_MENU_LAYOUT_H
#define DB1_MENU_LAYOUT_H
#include <stdint.h>
#include "db1_scenes.h"

/* Apply after the scene background/native center row has been composed.
 * The authored options/password panel grows horizontally; its entire text and
 * cursor rectangle remains one centered, unscaled layer. Does not touch VDP.
 * Returns zero for other scenes, rows outside the panel, or invalid geometry.
 * out must already contain width pixels. native contains exactly 320 pixels. */
int db1_menu_layout_row(Db1Scene scene, int line, const uint32_t *native,
                        uint32_t *out, int width);
/* Explicit 19980..1B040 map/menu ownership only. background is an independently
 * sampled authored plane B row, width pixels (may alias out). The complete
 * 160px map remains unchanged; the enlarged frame exposes background padding.
 * Sidebar labels, icons and confirmations move as one 88px rectangle. */
int db1_menu_layout_map_row(int line, const uint32_t *native,
                            const uint32_t *background, uint32_t *out, int width);
#endif
