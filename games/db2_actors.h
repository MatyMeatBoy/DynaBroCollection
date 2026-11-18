#ifndef DB2_ACTORS_H
#define DB2_ACTORS_H
#include <stddef.h>
#include <stdint.h>
typedef struct {const uint8_t *rom;size_t rom_size;const uint8_t *ram;size_t ram_size;} Db2ActorsContext;
typedef struct {uint16_t frame,flags;int dx,dy;uint8_t pattern[512];} Db2ActorComponent;
typedef struct {unsigned id;int x,y,depth;unsigned component_count;Db2ActorComponent part[2];} Db2Actor;
/* decode_cached requires current native list membership and exact actor identity. */
int db2_actor_decode_cached(const Db2ActorsContext*,unsigned,Db2Actor*);
int db2_actor_decode(const Db2ActorsContext*,unsigned,Db2Actor*);
unsigned db2_actors_visible(const Db2ActorsContext*,int,int,Db2Actor out[254]);
int db2_actor_pixel(const Db2ActorComponent*,unsigned,unsigned,uint8_t*);
int db2_actor_native_sat_body(const Db2ActorsContext*,unsigned tile_attribute);
#endif
