#include "db1_hazard_effects.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    Db1HazardEffects s={0};
    static const uint32_t entry[]={0x132d8,0x12fe6,0x14070,0x1429e,0x15d2c,0x5650,0x5ff6};
    static const uint32_t leave[]={0x13630,0x13286,0x1429c,0x144f6,0x15e52,0x5d62,0x62fc};
    unsigned i;
    for(i=0;i<sizeof(entry)/sizeof(entry[0]);++i) {
        assert(db1_hazard_effects_observe(&s,entry[i]));
        assert(!db1_hazard_effects_observe(&s,0x770)); /* VBlank must retain state */
        assert(s.active);
        assert(db1_hazard_effects_observe(&s,leave[i]));
        assert(!s.active);
    }
    db1_hazard_effects_observe(&s,0x132d8);
    assert(db1_hazard_effects_full_plane_a(&s));
    db1_hazard_effects_observe(&s,0x5650);
    assert(db1_hazard_effects_screen_sprites(&s));
    db1_hazard_effects_observe(&s,0x5d62);
    assert(db1_hazard_effects_full_plane_a(&s));
    assert(!db1_hazard_effects_screen_sprites(&s));
    db1_hazard_effects_reset(&s);
    assert(!s.active);
    for(i=0;i<256;++i) assert(db1_hazard_effects_weather_lists((uint8_t)i)==((i>>2)&3));
    puts("DB1 hazard lifecycle and world-weather classification passed");
    return 0;
}
