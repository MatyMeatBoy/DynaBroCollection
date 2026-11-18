#ifndef DB1_TITLE_ART_H
#define DB1_TITLE_ART_H
#include "db1_scenes.h"
/* Presentation only. line/x are native-screen coordinates, including margins.
 * Call after native row composition. English=0 leaves the original logo. */
void db1_title_art_row(const GVDP *,int line,uint32_t *out,int width,int native_left,int english);
/* Geometry is presentation-only. Anchor the 224-row original backing to the
 * canvas bottom while native plane A / sprite screen coordinates stay put. */
void db1_title_art_geometry(int canvas_height,int native_top,int native_height);
int db1_title_art_background_line(int native_line);
uint32_t db1_title_art_background_pixel(const GVDP *,int x,int native_line);
unsigned db1_title_art_logo_nearest(uint32_t rgb);
/* Pure helpers, also useful for validating the optional asset contract. */
int db1_title_art_band_offset(int image_w,int image_h,int sx,int sy);
unsigned db1_title_art_nearest(uint32_t rgb,const uint16_t palette[64],unsigned first,unsigned count);
uint32_t db1_title_art_alpha(uint32_t dst,uint32_t src,unsigned alpha);
int db1_title_art_identified(const GVDP *);
int db1_title_art_logo_ready(void);
#endif
