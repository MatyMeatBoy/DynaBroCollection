#include "db1_achievement_events.h"
#include <string.h>
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static const uint32_t normal_ids[20]={183311,183247,183312,183248,183249,
    183250,183251,183252,183253,183254,183255,183256,183257,183258,183259,
    183260,183261,183262,183263,183264};
static const uint32_t hard_ids[20]={183313,183265,183314,183266,183267,
    183268,183269,183270,183271,183272,183273,183274,183275,183276,183277,
    183278,183279,183280,183281,183282};
void db1_achievement_events_reset(Db1AchievementEvents *s) {
    if(s) memset(s,0,sizeof(*s));
}
/* The canonical RAM descriptor table has 0x4A-byte records. Practice advances
 * this pointer without updating FF0034 (ROM 1C90), so the pointer is primary.
 * Normal/Hard/Back use indices20..79; special passwords use80..96 and
 * ordinal zero. Index8..19 is unallocated. */
static int stage_index(const uint8_t *ram, uint32_t descriptor) {
    uint32_t delta;
    unsigned index,ordinal;
    if(descriptor<0xffaddeu) return -1;
    delta=descriptor-0xffaddeu;
    if(delta%0x4au) return -1;
    index=delta/0x4au;
    if(index>=97 || (index>=8 && index<20)) return -1;
    ordinal=index>=80?0:index<8?index+1:index%20+1;
    if(ram[descriptor-0xff0000u]!=ordinal) return -1;
    return (int)index;
}
void db1_achievement_events_observe_cpu(Db1AchievementEvents *s,uint32_t pc,
    const uint8_t *ram,size_t size,int eligible,uint32_t a2,Db1AchievementEmit emit,void *user) {
    uint32_t descriptor;
    int index;
    if(!s) return;
    if(!eligible || !ram || size<65536) { db1_achievement_events_reset(s); return; }
    pc&=0xffffffu;
    if(pc!=0x1c40u && pc!=0x1c44u && pc!=0x28aeu &&
       pc!=0xa10eu && pc!=0xa13cu && pc!=0xa162u && pc!=0x11ef4u &&
       pc!=0x11ff6u && pc!=0x12224u && pc!=0x12194u && pc!=0xd37au &&
       pc!=0x1429eu && pc!=0x14070u && pc!=0x14566u && pc!=0xc7a6u && pc!=0xc7b8u) return;
    descriptor=be32(ram+0x2e);
    if(pc==0x1c40u) {
        db1_achievement_events_reset(s);
        index=stage_index(ram,descriptor);
        if(index<0) return;
        /* Practice stageword intentionally remains zero; the other campaigns
         * must agree with their descriptor index. */
        if(index>=20 && (((unsigned)ram[0x34]<<8)|ram[0x35])!=(unsigned)index) return;
        s->descriptor=descriptor; s->stage=(uint16_t)index; s->armed=1;
        s->previous_enemy_defeats=(uint16_t)(((unsigned)ram[0x258e]<<8)|ram[0x258f]);
        s->enemy_counter_valid=(s->previous_enemy_defeats==0);
        return;
    }
    if(!s->armed) return;
    if(descriptor!=s->descriptor || stage_index(ram,descriptor)!=(int)s->stage) {
        db1_achievement_events_reset(s); return;
    }
    /* 288A checks the live battle result and 28AE is its nonzero-result RTS.
     * Require this real return, then 1C44 immediately after the battle BSR.
     * Results UI is bypassed on final stages, so 1B9DA is insufficient. */
    /* C778 is the winning Tyranno (species low nibble3) combat branch.
     * Opponent species3/4 increments A2+F; the fourth kill takes C7A6,
     * which writes the King marker4B at A2+E. C7B8 follows the event trap.
     * Require a canonical PLAYER record and the real before/after transition,
     * in a numbered (non-??) descriptor. Never infer evolution from a loaded
     * existing King, enemy actor, four-kill counter alone, or secret stage. */
    if(pc==0xc7a6u) {
        uint32_t offset;
        s->king_candidate=0;
        a2&=0xffffffu;
        if(s->stage<80 && a2>=0xff5f44u && a2<0xff6734u &&
           (a2-0xff5f34u)%16==0) {
            offset=a2-0xff0000u;
            if(ram[offset]>=0x30 && (ram[offset]&15)==3 && ram[offset+15]==4 &&
               ram[offset+14]!=0x4b && ram[offset+14]!=0x6b) s->king_candidate=a2;
        }
    }
    if(pc==0xc7b8u && s->king_candidate) {
        uint32_t offset=s->king_candidate-0xff0000u;
        if((a2&0xffffffu)==s->king_candidate && ram[offset+14]==0x4b &&
           ram[offset+15]==4 && !s->king_emitted && emit) {
            emit(user,205017u); s->king_emitted=1;
        }
        s->king_candidate=0;
    }
    /* D352 counts removed creatures: IDbit7 selects the enemy counter
     * FF258E; D374 increments it (cap999), D37A returns after the update.
     * D182 initializes both faction counters to zero. Count only observed
     * single increments; a discontinuity disqualifies this goal for battle. */
    if(pc==0xd37au && s->enemy_counter_valid) {
        unsigned now=((unsigned)ram[0x258e]<<8)|ram[0x258f];
        if(now!=s->previous_enemy_defeats) {
            if(now==(unsigned)s->previous_enemy_defeats+1 && now<=999) {
                if(s->enemy_defeats<100) ++s->enemy_defeats;
                s->previous_enemy_defeats=(uint16_t)now;
            } else s->enemy_counter_valid=0;
        }
    }
    /* Player Rain: 11ECC checks sufficient BCD power at128D4 and the
     * rain-active flag, then charges at11EF0. Only success reaches11EF4.
     * Terrain10B6C creates grass/grows it; 12A5C starts the rain duration.
     * Sega's original instructions identify this grass-growing action as Rain. */
    if(pc==0x11ef4u && s->rain_casts<100) ++s->rain_casts;
    /* Each hook is immediately after the player's successful power charge.
     * Earthquake12FE6 shakes both VDP scroll planes with alternating random
     * offsets; Tsunami14070/1429E render the original wave-pattern streams
     *15110/15A4C; Meteor5650 spawns falling diagonal projectiles and121BC
     * applies the destructive terrain effect11298. Enemy casts are separate. */
    if(pc==0x11ff6u) s->disaster_casts|=1;
    if(pc==0x12224u) { s->disaster_casts|=2; s->surf_pending=1; }
    /* Public author hint193205: Tsunami with no units in the way.
     *1222C/12238 count the two factions; both zero selects1429E, whose
     *14566 animates the surfer sprites over the wave. The populated branch
     *14070 does not call this sprite animator. Unlock on actual animation,
     * not on command selection or on a future victory. */
    if(pc==0x14070u) s->surf_pending=0;
    if(pc==0x1429eu && s->surf_pending==1) s->surf_pending=2;
    if(pc==0x14566u && s->surf_pending==2) {
        s->surf_pending=0;
        if(!s->surf_emitted && emit) { emit(user,193205u); s->surf_emitted=1; }
    }
    if(pc==0x12194u) s->disaster_casts|=4;
    /* A10E decrements FF7BB4. A13C is reached only on the first expiry
     * (FF7BC9 was clear), sets the event flag, then A15E calls the disaster
     * selected by descriptor+45. A162 is reached after that call returns.
     * Zero-initialized timers and pre-existing extinction flags do not count. */
    if(pc==0xa10eu) s->timeout_pending=(be32(ram+0x7bb4)==1 && ram[0x7bc9]==0);
    if(pc==0xa13cu && s->timeout_pending==1 &&
       be32(ram+0x7bb4)==0 && ram[0x7bc9]==0) s->timeout_pending=2;
    if(pc==0xa162u && s->timeout_pending==2) {
        if(be32(ram+0x7bb4)==0 && ram[0x7bc9]==255) s->timeout_triggered=1;
        s->timeout_pending=0;
    }
    if(pc==0x28aeu) s->saw_victory_return=(ram[0x3d]==1);
    if(pc!=0x1c44u) return;
    index=s->stage;
    if(s->saw_victory_return && ram[0x3d]==1 && emit) {
        if(index>=20 && index<80 && s->enemy_counter_valid && s->enemy_defeats>=100)
            emit(user,193202u);
        if(s->disaster_casts==7) emit(user,205018u);
        if(s->rain_casts>=100) emit(user,193201u);
        if(index>=20 && index<80 && s->timeout_triggered) emit(user,193203u);
        if(index<8) emit(user,183303u+(unsigned)index);
        else if(index<40) emit(user,normal_ids[index-20]);
        else if(index<60) {
            emit(user,normal_ids[index-40]); emit(user,hard_ids[index-40]);
        } else if(index<80) emit(user,183283u+(unsigned)(index-60));
        else if(index==87) emit(user,205061u); /* password table PPPP */
        else if(index==92) emit(user,193204u); /* password table PUGU */
    }
    db1_achievement_events_reset(s);
}

void db1_achievement_events_observe(Db1AchievementEvents *s,uint32_t pc,
    const uint8_t *ram,size_t size,int eligible,Db1AchievementEmit emit,void *user) {
    db1_achievement_events_observe_cpu(s,pc,ram,size,eligible,0,emit,user);
}
