#ifndef DB1_ORIGINAL_TITLE_H
#define DB1_ORIGINAL_TITLE_H
#include <stdint.h>
#include "video/genesis_vdp.h"
/* Nonmodal, presentation only: English title replacement in original 4:3. */
int db1_original_title_active(const GVDP *,int english,int enhanced);
/* pitch is in bytes. Only native logo ROI x48..271,y24..143 can be written.
 * Returns1 when applied; invalid geometry/assets/language leave pixels exact. */
int db1_original_title_render(const GVDP *,uint32_t *pixels,int width,int height,
                              int pitch,int english,int enhanced);
#endif
