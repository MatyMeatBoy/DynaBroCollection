#ifndef DB2_MARKERS_H
#define DB2_MARKERS_H
#include <stdint.h>
typedef struct { int x,y; uint16_t attribute; uint8_t bit; } Db2WorldMarker;
/* Native SAT order, front first. Each marker is one8x8 VDP sprite.
 * visual_flags = RAM FF2A89; actor_flags = actor record byte28(hex).
 * body_x/y must include the current animation offsets and elevation, exactly
 * as the body renderer does. Draw in reverse returned order over the body. */
unsigned db2_world_markers(uint8_t visual_flags,uint8_t actor_flags,int body_x,int body_y,Db2WorldMarker out[6]);
int db2_world_is_marker_pattern(unsigned pattern);
#endif
