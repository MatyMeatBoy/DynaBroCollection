#include "db1_scenes.h"
#include "video/genesis_dac.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static GVDP v,before;
static void word(unsigned p,unsigned value) {
    v.vram[p]=(uint8_t)(value>>8);v.vram[p+1]=(uint8_t)value;
}
int main(void) {
    v.reg[4]=7;v.reg[13]=0x34;v.reg[16]=1;
    for(unsigned i=0;i<16;i++) {
        memset(v.vram+(i+1)*32,(int)((i%15+1)*17),32);
        word(0xe000+i*2,(i+1)|((i&3)<<13)|(i&1?0x9800:0));
    }
    memset(v.vram+30*32,0x33,32);word(0xe080,30);
    /* Original39-column writer, rolling16 glyphs through15 row pairs. */
    for(unsigned y=0;y<30;y++)for(unsigned x=0;x<39;x++) {
        unsigned index=(x+7*(y/2))&15;
        unsigned attr=y&1?30u:(index+1)|((index&3)<<13)|(index&1?0x9800:0);
        word(0xe000+2*(y*64+x),attr);
    }
    /* Later panel overwrites must never appear in reconstructed backdrop. */
    for(unsigned y=8;y<24;y++)for(unsigned x=6;x<34;x++)
        word(0xe000+2*(y*64+x),0);
    before=v;
    for(int y=0;y<240;y++)for(int x=0;x<312;x++) {
        if(y>=64 && y<192 && x>=48 && x<272)continue;
        assert(db1_scenes_menu_background_b(&v,x,y)==db1_scenes_plane_b(&v,x,y));
    }
    for(int y=-300;y<300;y+=3)for(int x=-180;x<450;x+=3) {
        uint8_t p=db1_scenes_menu_background_b(&v,x,y);
        assert(p==db1_scenes_menu_background_b(&v,x+128,y));
        assert(p==db1_scenes_menu_background_b(&v,x,y+256));
    }
    assert(!memcmp(&v,&before,sizeof v));
    uint32_t native[320],out[427];
    for(unsigned x=0;x<320;x++)native[x]=0xffabcdefu+x;
    db1_scenes_reset();db1_scenes_observe(0x1e024);
    assert(db1_scenes_row(&v,0,native,320,out,427));
    assert(!memcmp(out+53,native,312*sizeof *native));
    for(int x=312;x<320;x++)
        assert(out[x+53]==db1_scenes_background_for(DB1_SCENE_OPTIONS,&v,x,0));
    uint8_t p=db1_scenes_menu_background_b(&v,-13,-7);
    word(0xd002,13);v.vsram[1]=7;
    assert(db1_scenes_menu_background_b(&v,0,-14)==p);
    v.reg[11]=3;word(0xd002+4*5,13);
    assert(db1_scenes_menu_background_b(&v,0,5)==
           db1_scenes_menu_background_b(&before,-13,12));
    v.reg[16]=2;assert(!db1_scenes_menu_background_b(&v,0,0));
    v.reg[16]=1;v.reg[12]=6;assert(!db1_scenes_menu_background_b(&v,0,0));
    puts("DB1 native UI backdrop128x256: PASS");
    return 0;
}
