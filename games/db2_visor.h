#ifndef DB2_VISOR_H
#define DB2_VISOR_H
typedef struct {int x,y,w,h;} Db2VisorRect;
typedef struct {
 Db2VisorRect frame;
 int hud_left,hud_shift,world_shift_x;
 int offset_tiles,min_tiles,max_tiles;
} Db2VisorLayout;
/* DB2 normal-layout outer frame only; no interior/effect clip is inferred.
 * Native frame is192x192 at(16,16), independent of the104px right HUD.
 * Requested8px tile offsets clamp before multiplication, including INT_MIN/MAX.
 * Invalid width (outside320..768) or NULL out leaves output unchanged. */
int db2_visor_layout(int canvas_width,int requested_tiles,Db2VisorLayout *out);
/* Apply the SAME host projection to terrain, actors, command cursors, effects
 * and all four frame corners. No guest camera/command/eligibility mutation.
 * NULL layout is identity; unrepresentable coordinates saturate to int bounds. */
int db2_visor_canvas_x(const Db2VisorLayout *,int native_world_x);
int db2_visor_native_x(const Db2VisorLayout *,int canvas_x);
#endif
