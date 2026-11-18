#include "db1_visible.h"
#include <assert.h>
#include <string.h>
static uint8_t ram[65536],copy[65536];
int main(void) {
    Db1WorldContext c={.ram=ram,.ram_size=sizeof ram};
    Db1VisibleActor actors[256];
    /* Two actors in every cell: 72 visible, twice the original cache. */
    for(unsigned y=0;y<6;y++)for(unsigned x=0;x<6;x++) {
        unsigned id=1+(y*6+x)*2;
        ram[0x6f34+y*80+x*2]=(uint8_t)id;
        ram[0x6f35+y*80+x*2]=(uint8_t)(id+1);
    }
    memcpy(copy,ram,sizeof ram);
    assert(db1_visible_actors(&c,208,208,actors)==72);
    assert(!memcmp(copy,ram,sizeof ram));
    ram[0x6f34+7*2]=200; /* outside classic viewport, inside expansion */
    assert(db1_visible_actors(&c,208,208,actors)==72);
    assert(db1_visible_actors(&c,286,208,actors)==73);
    ram[0x6f34+8*2]=200; /* duplicate references never draw twice */
    assert(db1_visible_actors(&c,320,208,actors)==73);
    ram[0x6f34+9*2]=254;ram[0x6f35+9*2]=255;
    assert(db1_visible_actors(&c,400,208,actors)==73);
    ram[0x5f34+16+5]=31;
    ram[0x5f34+32+5]=0;
    assert(db1_visible_actors(&c,400,208,actors)==73);
    for(unsigned i=1;i<73;i++)assert(actors[i-1].sort_y>=actors[i].sort_y);
    ram[0x5f34+16+6]=240; /* A flying/jumping body moves up without changing depth. */
    assert(db1_visible_actors(&c,400,208,actors)==73);
    for(unsigned i=0;i<73;i++)if(actors[i].id==1)assert(actors[i].y==actors[i].sort_y-16);
    memset(ram,0,sizeof ram);
    for(unsigned id=1;id<=253;id++) {
        unsigned cell=(id-1)/2;
        ram[0x6f34+(cell/24)*80+(cell%24)*2+(id-1)%2]=(uint8_t)id;
    }
    assert(db1_visible_actors(&c,768,208,actors)==253);
    memset(ram,0,sizeof ram);
    ram[0x7bd4]=15;ram[0x7bd5]=12;
    ram[0x204]=255;ram[0x205]=240;ram[0x207]=4;
    ram[0x6f34+13*80+23*2]=7;
    ram[0x5f34+7*16+5]=253;ram[0x5f34+7*16+6]=240;
    assert(db1_visible_actors(&c,398,208,actors)==1);
    assert(actors[0].id==7 && actors[0].x==256 && actors[0].y==25 && actors[0].sort_y==37);
    c.ram_size=10;
    assert(db1_visible_actors(&c,320,208,actors)==0);
    return 0;
}
