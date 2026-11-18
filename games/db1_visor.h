#ifndef DB1_VISOR_H
#define DB1_VISOR_H
typedef struct {int x,y,w,h;} Db1VisorRect;
typedef struct {
 Db1VisorRect frame,interior;
 int hud_left,hud_shift,world_shift_x;
 int offset_tiles,min_tiles,max_tiles;
} Db1VisorLayout;
/* Native-line coordinates. One tile is8 physical canvas pixels, never scaled.
 * Requested offsets are clamped before multiplication, including INT_MIN/MAX. */
int db1_visor_layout(int canvas_width,int requested_tiles,Db1VisorLayout *out);
int db1_visor_canvas_x(const Db1VisorLayout *layout,int native_world_x);
int db1_visor_native_x(const Db1VisorLayout *layout,int canvas_x);
#endif
