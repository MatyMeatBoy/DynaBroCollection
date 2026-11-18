#include "db1_scenes.h"
#include "video/genesis_dac.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static GVDP v,before;
static void word(unsigned a,unsigned n) {v.vram[a]=(uint8_t)(n>>8);v.vram[a+1]=(uint8_t)n;}
int main(void) {
    db1_scenes_reset();assert(db1_scenes_kind()==DB1_SCENE_UNKNOWN);
    db1_scenes_observe(0x1c5a4);assert(db1_scenes_kind()==DB1_SCENE_BRANDING);
    /* The same native intro routine owns black SEGA, white logo, fades and
     * black cave/story. Its live VDP backdrop, not the latched scene enum,
     * is authoritative for the added margins. */
    for(unsigned n=0;n<8;n++) {
        v.cram[0]=(uint16_t)(n*0x222);
        assert(db1_scenes_background(&v,-20,-20)==genesis_dac_cram_to_argb(v.cram[0],0));
    }
    v.cram[0]=0;assert(db1_scenes_background(&v,0,0)==0xff000000u);
    db1_scenes_observe(0x1c9ce);assert(db1_scenes_kind()==DB1_SCENE_TITLE);
    v.reg[4]=6;v.reg[13]=0x3c;v.reg[16]=1;
    int sx,sy;
    assert(db1_scenes_plane_b_coordinates(&v,-23,-23,&sx,&sy)&&sx==-23&&sy==-23);
    /* 512x256 authored name table; crossing the left boundary reaches tile63. */
    word(0xc000+63*2,0xa001);memset(v.vram+32,0x12,32);
    assert(db1_scenes_plane_b(&v,-8,0)==0xd1);
    assert(db1_scenes_plane_b(&v,-7,0)==0xd2);
    assert(db1_scenes_plane_b(&v,504,0)==0xd1);
    word(0xc000+63*2,0xb801); /* horizontal + vertical flip */
    assert(db1_scenes_plane_b(&v,-8,0)==0xd2);
    word(0xc000,1);word(0xf002,8);
    assert(db1_scenes_plane_b_coordinates(&v,0,0,&sx,&sy)&&sx==-8&&sy==0);
    assert(db1_scenes_plane_b(&v,0,0)==0xd2);
    word(0xf002,0);v.vsram[1]=8;word(0xc000+64*2,1);
    assert(db1_scenes_plane_b(&v,0,0)==0x41);
    v.vsram[1]=0;
    v.reg[11]=3;word(0xf002+4*9,8);
    assert(db1_scenes_plane_b(&v,0,9)==0);
    word(0xc000+64*2+63*2,1);
    assert(db1_scenes_plane_b(&v,0,9)==0x41);
    v.reg[11]=4;v.vsram[3]=8;
    word(0xc000+64*2+2*2,1);
    assert(db1_scenes_plane_b(&v,16,0)==0x41);
    v.reg[11]=0;
    uint32_t native[320],out[400];for(unsigned i=0;i<320;i++)native[i]=0xff123400u+i;
    before=v;assert(db1_scenes_row(&v,0,native,320,out,400));
    assert(!memcmp(out+40,native,sizeof native));assert(!memcmp(&v,&before,sizeof v));
    assert(!db1_scenes_row(&v,0,native,320,out,319));
    db1_scenes_observe(0x1c5a4);
    assert(db1_scenes_row(&v,0,native,320,out,400));
    assert(out[0]==0xff000000u&&out[399]==0xff000000u);
    assert(!memcmp(out+40,native,sizeof native));
    db1_scenes_observe(0x1e024);assert(db1_scenes_kind()==DB1_SCENE_OPTIONS);
    db1_scenes_observe(0x1d7b6);assert(db1_scenes_kind()==DB1_SCENE_PASSWORD);
    db1_scenes_observe(0x1950);assert(db1_scenes_kind()==DB1_SCENE_UNKNOWN);
    db1_scenes_observe(0x1b42);assert(db1_scenes_kind()==DB1_SCENE_STAGE_INTRO);
    assert(!db1_scenes_row(&v,0,native,320,out,400));
    db1_scenes_observe(0x1c40);assert(db1_scenes_kind()==DB1_SCENE_BATTLE);
    v.reg[7]=3;v.cram[3]=0x666;
    assert(db1_scenes_background_for(DB1_SCENE_BRANDING,&v,0,0)==genesis_dac_cram_to_argb(0x666,0));
    db1_scenes_observe(0x1c44);assert(db1_scenes_kind()==DB1_SCENE_UNKNOWN);
    db1_scenes_observe(0x1bc04);assert(db1_scenes_kind()==DB1_SCENE_GAMEOVER);
    assert(db1_scenes_row(&v,0,native,320,out,400));
    assert(!memcmp(out+40,native,sizeof native));
    puts("DB1 scene provenance and immutable plane-B extension contracts passed");
    return 0;
}
