#ifndef DB2_EFFECTS_H
#define DB2_EFFECTS_H
#include "db2_world.h"
typedef enum { DB2_EFFECT_PSEUDO=1, DB2_EFFECT_GLOBAL6F1=2 } Db2EffectKind;
typedef struct {
 uint16_t attr;
 uint8_t size;
 int x,y,depth;
 unsigned rank,sat_index;
 Db2EffectKind kind;
} Db2WorldEffect;
/* Pure readers. sat_base is the actual VDP SAT byte address. Return actual
 * native-emitted pseudo effects only, in SAT order; rank is the native gather
 * order, including actors. At most40 gather entries and80 SAT entries are used.
 * GLOBAL6F1 is reserved: its coarse-row insertion lacks a proven common depth,
 * so those entries remain native. Coordinates remove VDP bias128 (X9/Y10bits).
 * Invalid gather lists produce zero effects; malformed SAT links terminate
 * the scan after the unique valid prefix. Inputs and output must not alias. */
size_t db2_effects_collect(const Db2WorldContext *,unsigned sat_base,Db2WorldEffect *,size_t capacity);
int db2_effects_is_marker(uint16_t attr);
#endif
