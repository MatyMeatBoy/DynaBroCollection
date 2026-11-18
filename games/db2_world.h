#ifndef DB2_WORLD_H
#define DB2_WORLD_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
 const uint8_t *rom; size_t rom_size;
 const uint8_t *ram; size_t ram_size;
 const uint8_t *vram; size_t vram_size;
} Db2WorldContext;
typedef struct {
 unsigned cell_x, cell_y, ring_x, ring_y;
 int hscroll, vscroll;
 int anchor_x, anchor_y; /* projected top-left of camera cell */
} Db2WorldCamera;
/* Pure readers. Caller must gate these on a live DB2 battlefield. */
int db2_world_read_camera(const Db2WorldContext *, Db2WorldCamera *);
/* Row-major 4x4 VDP name words for one 32x32 cell of the 40x40 grid. */
int db2_world_cell_names(const Db2WorldContext *, unsigned x, unsigned y, uint16_t names[16]);
/* Packed native sample: color[5:0], opaque[6], priority[7]. */
int db2_world_terrain_plane(const Db2WorldContext *, unsigned x, unsigned y, unsigned px, unsigned py, uint8_t *pixel);
int db2_world_project_x(const Db2WorldCamera *, int world_pixel_x);
int db2_world_project_y(const Db2WorldCamera *, int world_pixel_y);
#endif
