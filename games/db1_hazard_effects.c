#include "db1_hazard_effects.h"
void db1_hazard_effects_reset(Db1HazardEffects *s) { if(s) s->active=0; }
int db1_hazard_effects_observe(Db1HazardEffects *s,uint32_t pc) {
    uint32_t before, set=0, clear=0;
    if(!s) return 0;
    before=s->active;
    switch(pc) {
    case 0x132d8: set=DB1_HAZARD_LIGHTNING; break;
    case 0x13630: clear=DB1_HAZARD_LIGHTNING; break;
    case 0x12fe6: set=DB1_HAZARD_QUAKE; break;
    case 0x13286: clear=DB1_HAZARD_QUAKE; break;
    case 0x14070: case 0x1429e: set=DB1_HAZARD_TSUNAMI; break;
    case 0x1429c: case 0x144f6: clear=DB1_HAZARD_TSUNAMI; break;
    case 0x15d2c: set=DB1_HAZARD_DISTORTION; break;
    case 0x15e52: clear=DB1_HAZARD_DISTORTION; break;
    case 0x5650: set=DB1_HAZARD_METEOR; break;
    case 0x5d62: clear=DB1_HAZARD_METEOR; break;
    case 0x5ff6: set=DB1_HAZARD_GENERIC; break;
    case 0x62fc: clear=DB1_HAZARD_GENERIC; break;
    default: break;
    }
    s->active=(before|set)&~clear;
    return before!=s->active;
}
int db1_hazard_effects_full_plane_a(const Db1HazardEffects *s) {
    return s && (s->active&(DB1_HAZARD_LIGHTNING|DB1_HAZARD_TSUNAMI))!=0;
}
int db1_hazard_effects_screen_sprites(const Db1HazardEffects *s) {
    return s && (s->active&(DB1_HAZARD_METEOR|DB1_HAZARD_GENERIC))!=0;
}
unsigned db1_hazard_effects_weather_lists(uint8_t flags) { return (flags>>2)&3u; }
