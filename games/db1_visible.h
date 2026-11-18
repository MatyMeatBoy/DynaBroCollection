#ifndef DB1_VISIBLE_H
#define DB1_VISIBLE_H
#include "db1_world.h"
typedef struct { unsigned id; int x,y,sort_y; } Db1VisibleActor;
/* All 256 occupancy IDs are considered; no dependency on the guest SAT cache.
 * Output follows native descending-Y SAT priority. Paint in reverse order.
 * Duplicate occupancy references draw once. */
unsigned db1_visible_actors(const Db1WorldContext *, int width, int height,
                           Db1VisibleActor out[256]);
/* Presentation rectangle in native world-pixel coordinates, including added
 * canvas rows (negative top is valid). No simulation activation is changed. */
unsigned db1_visible_actors_rect(const Db1WorldContext *, int left, int top,
                                int right, int bottom, Db1VisibleActor out[256]);
#endif
