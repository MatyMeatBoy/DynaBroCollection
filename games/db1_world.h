#ifndef DB1_WORLD_H
#define DB1_WORLD_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const uint8_t *rom; size_t rom_size;
    const uint8_t *ram; size_t ram_size;
    const uint8_t *vram; size_t vram_size;
} Db1WorldContext;
typedef struct { uint8_t pattern[512]; uint16_t flags; uint16_t frame; } Db1WorldActorFrame;
/* RAM addresses are offsets within FF0000. Coordinates describe the 40-stride
 * storage grid, not a claim that every cell is playable. ID zero is empty.
 * Randomized ID 0x80 reuses the guest's generated 16x8 terrain ring; outside
 * that ring it is unsupported (no guest RNG is consumed).
 * Pixel values include palette (0..63); low nibble zero is transparent. */
int db1_world_terrain_pixel(const Db1WorldContext *, unsigned cell_x, unsigned cell_y,
                            unsigned pixel_x, unsigned pixel_y, uint8_t *pixel);
/* Native packed plane sample: color[5:0], opaque[6], priority[7]. */
int db1_world_terrain_plane(const Db1WorldContext *, unsigned cell_x, unsigned cell_y,
                            unsigned pixel_x, unsigned pixel_y, uint8_t *pixel);
/* Reads original animation and compressed pattern streams without CPU execution.
 * actor_id is the occupancy ID (0..255), whose high bit chooses faction palette.
 * Flags use VDP palette/flip/priority positions. Malformed/unsupported data
 * returns zero without changing output. Input buffers must not alias output. */
int db1_world_actor_frame(const Db1WorldContext *, const uint8_t record[16], unsigned actor_id, Db1WorldActorFrame *);
int db1_world_actor_pixel(const Db1WorldActorFrame *, unsigned x, unsigned y, uint8_t *pixel);
int db1_world_decompress(const Db1WorldContext *, size_t stream, uint8_t output[512]);
#endif
