#pragma once
#include <stddef.h>
#include <stdint.h>
#include "video/genesis_vdp.h"

/* English host overlay for the six DB2 Special biography scenes. Pure
 * presentation: reads guest ROM/CRAM, never writes guest state. Japanese or
 * unqualified scenes are a no-op. */
void db2_bio_overlay_reset(void);
/* Called at original instruction boundaries (PCs 05D3D4/05D57A/05D674/05D722). */
void db2_bio_overlay_observe(unsigned pc);
int db2_bio_overlay_active(void);
/* Draw into one native-coordinate row. `origin_x` is the x of native column 0
 * in `out`; columns outside [0,width) are skipped. */
void db2_bio_overlay_row(const GVDP *v,int line,uint32_t *out,int width,int origin_x);
void db2_bio_overlay_scanline(const GVDP *v,int line,uint32_t *out,int width);
/* Test hooks. */
int db2_bio_overlay_load_font(const uint8_t *rom,size_t size);
void db2_bio_overlay_set_glyph(unsigned ch,const uint8_t rows[8]);
void db2_bio_overlay_force(int record);
