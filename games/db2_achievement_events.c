#include "db2_achievement_events.h"
#include <string.h>
static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
/* Decoded original-ROM descriptor streams at pointer table184400, width5C.
 * Counts and mode/index identities independently checked for every record. */
static const uint8_t maps_0[8]={51,53,52,54,55,57,58,56};
static const uint8_t maps_1[45]={1,21,22,23,2,24,3,25,4,5,26,28,7,8,33,29,34,30,11,35,27,9,36,44,16,13,42,40,39,43,14,15,12,31,32,17,6,10,18,38,45,41,19,37,20};
static const uint8_t maps_3[10]={80,81,67,66,68,70,73,76,72,71};
static const uint8_t maps_4[10]={69,82,74,85,84,78,88,86,89,112};
static const uint8_t maps_5[10]={128,114,131,113,119,120,130,127,123,124};
static const uint8_t maps_6[10]={138,140,135,141,146,144,145,149,147,150};
static int identity(const uint8_t *ram, unsigned *mode, unsigned *stage) {
    const uint8_t *maps; unsigned count;
    *mode=ram[0x27c]; *stage=ram[0x27d];
    switch(*mode) {
    case 0: maps=maps_0; count=8; break;
    case 1: maps=maps_1; count=45; break;
    case 3: maps=maps_3; count=10; break;
    case 4: maps=maps_4; count=10; break;
    case 5: maps=maps_5; count=10; break;
    case 6: maps=maps_6; count=10; break;
    default: return 0;
    }
    if(!*stage || *stage>count || be32(ram+0x278)!=0xff0292u) return 0;
    return ram[0x292]==*mode && ram[0x293]==*stage &&
        ram[0x294]==(*stage==count?255u:0u) && ram[0x295]==maps[*stage-1];
}
void db2_achievement_events_reset(Db2AchievementEvents *s) {
    if(s) memset(s,0,sizeof(*s));
}
void db2_achievement_events_observe(Db2AchievementEvents *s,uint32_t pc,
    const uint8_t *ram,size_t size,int eligible,Db2AchievementEmit emit,void *user) {
    unsigned mode,stage;
    if(!s) return;
    if(!eligible || !ram || size<65536 || be32(ram+0x4c2)==0x5548596fu) {
        db2_achievement_events_reset(s); return;
    }
    pc&=0xffffffu;
    /* New entry/load and title loop invalidate a previously armed battle. */
    if(pc==0x3b1eu || pc==0x6f9au || pc==0x3a38u || pc==0x3c70u) {
        db2_achievement_events_reset(s); return;
    }
    if(pc!=0x3bb0u && pc!=0x3bb4u && pc!=0x3cb2u) return;
    if(!identity(ram,&mode,&stage)) { db2_achievement_events_reset(s); return; }
    if(pc==0x3bb0u) {
        db2_achievement_events_reset(s);
        s->mode=(uint8_t)mode; s->stage=(uint8_t)stage; s->armed=1;
        return;
    }
    if(!s->armed) return;
    if(mode!=s->mode || stage!=s->stage) { db2_achievement_events_reset(s); return; }
    /* 3BB4 follows the real gameplay BSR. 3BCA then checks FF9C86==1 and
     * only the genuine campaign win branch reaches3CB2. Result UI alone,
     * versus results, final-stage endings and password previews cannot emit. */
    if(pc==0x3bb4u) {
        if(ram[0x9c86]!=1) { db2_achievement_events_reset(s); return; }
        s->saw_battle_return=1; return;
    }
    if(s->saw_battle_return && ram[0x9c86]==1 && emit)
        emit(user,2000000u+mode*100u+stage);
    db2_achievement_events_reset(s);
}
