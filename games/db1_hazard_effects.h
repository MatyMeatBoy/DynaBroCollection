#ifndef DB1_HAZARD_EFFECTS_H
#define DB1_HAZARD_EFFECTS_H
#include <stdint.h>
/* Host presentation state only. Observe before executing the named guest PC.
 * Never infer an effect from PC ranges: VBlank executes outside those ranges. */
enum {
    DB1_HAZARD_LIGHTNING = 1u,
    DB1_HAZARD_QUAKE = 2u,
    DB1_HAZARD_TSUNAMI = 4u,
    DB1_HAZARD_DISTORTION = 8u,
    DB1_HAZARD_METEOR = 16u,
    DB1_HAZARD_GENERIC = 32u
};
typedef struct { uint32_t active; } Db1HazardEffects;
void db1_hazard_effects_reset(Db1HazardEffects *);
/* Returns nonzero only when the presentation policy changes. */
int db1_hazard_effects_observe(Db1HazardEffects *, uint32_t pc);
int db1_hazard_effects_full_plane_a(const Db1HazardEffects *);
int db1_hazard_effects_screen_sprites(const Db1HazardEffects *);
/* Weather lists are world anchored, not a full-screen texture. */
unsigned db1_hazard_effects_weather_lists(uint8_t native_flags);
#endif
