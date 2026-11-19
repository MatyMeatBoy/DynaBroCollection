#ifndef DB2_WIDE_MENU_H
#define DB2_WIDE_MENU_H
#include <stdint.h>
struct GVDP;
/* Widescreen margins for DB2 screens outside the battlefield (intro, title,
 * main menu, submenus, briefings). The authentic 320-pixel image stays exact in
 * the centre; the columns left and right of it are composed from the guest's
 * own plane A/B name tables (with the live per-line/per-cell scroll) and the
 * sprites positioned there. Nothing is invented: a column only shows what the
 * guest's planes and SAT already contain beyond the 4:3 window.
 *
 * line: native raster line (may be outside 0..223 for the letterbox rows).
 * out/w: one canvas row; left: x of native column 0 inside the canvas.
 * Columns [left, left+320) are not touched when native_row is true.
 * Returns 1 when margins were drawn. Pure presentation; never mutates VDP. */
int db2_wide_menu_row(const struct GVDP *v, int line, uint32_t *out, int w, int left, int native_row);
/* Call once per frame (line 0) before rows: caches which plane columns hold art. */
void db2_wide_menu_frame(const struct GVDP *v);
/* Packed plane sample (color[5:0], opaque[6], priority[7]) for screen x/line,
 * exposed for the contract test. plane: 0=A, 1=B. */
uint8_t db2_wide_menu_plane_pixel(const struct GVDP *v, int plane, int x, int line);
/* DB1-intro smoothstep fade; weight 0..3375 (i*i*(45-2i) for i=0..15). */
uint32_t db2_wide_menu_blend(uint32_t pixel, uint32_t backdrop, unsigned weight);
/* True when this frame continued authored art past the 4:3 window (intro). */
int db2_wide_menu_extended(void);
/* While on, plane B outside the window/letterbox is generated from the confetti
 * tiles of the outer columns (stage briefing). */
void db2_wide_menu_mirror(const struct GVDP *v, int on);
#endif
