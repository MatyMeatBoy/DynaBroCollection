#ifndef DB1_TITLE_MENU_ART_H
#define DB1_TITLE_MENU_ART_H
#include <stdint.h>
#include "video/genesis_vdp.h"
typedef uint32_t (*Db1TitleMenuBackground)(const GVDP *,int x,int line);
/* Root calls only for a mature Enhanced title menu. Coordinates are native;
 * left locates native x0 within this output row. selected0..4,4 is Extras.
 * Reads live glyphs/SAT/CRAM, writes only owned opaque pixels, never VDP data.
 * Background must preserve TitleArt's anchored plane B geometry. Returns1
 * when the row belongs to this identified menu overlay,0 otherwise. */
int db1_title_menu_art_row(const GVDP *,int line,uint32_t *out,int width,int left,
 unsigned selected,int modal,Db1TitleMenuBackground background);
#endif
