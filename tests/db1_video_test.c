#include "db1_video.h"
#include "genesis_runtime.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include <assert.h>
#include <string.h>
uint8_t g_rom[0x400000],g_ram[65536];
static GVDP vdp;
static GVDPRasterRow raster;
static uint32_t native[320],out[398];
static uint32_t canvas[480*270];
static uint8_t ram_copy[65536];
static GVDP vdp_copy;
static unsigned calls;
static const uint32_t *row(void *ctx){(void)ctx;calls++;return native;}
static void be16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void be32(uint8_t *p,unsigned v){be16(p,v>>16);be16(p+2,v);}
static void draw(int line){
    db1_video.scanline_presented(&vdp,line,320,out,398,row,0,&raster);
}
int main(void){
    be32(g_rom+0x70010,0x1000);be32(g_rom+0x70014,0x2000);
    be16(g_rom+0x1000,0x100);be16(g_rom+0x2000,0x20);
    g_rom[0x2022]=255; /* frame zero, duration 255 */
    be16(g_rom+0x1100,512);
    unsigned p=0x1103;
    for(unsigned n=0;n<4;n++){g_rom[p++]=127;memset(g_rom+p,0x11,128);p+=128;}
    for(unsigned y=0;y<6;y++)for(unsigned x=0;x<6;x++)for(unsigned slot=0;slot<2;slot++){
        unsigned id=1+(y*6+x)*2+slot;
        g_ram[0x6f34+y*80+x*2+slot]=(uint8_t)id;
        g_ram[0x5f34+id*16]=1;
        g_ram[0x5f34+id*16+4]=(uint8_t)(slot*12);
    }
    g_ram[0x6f34+8*2]=100;g_ram[0x5f34+100*16]=1;
    vdp.cram[1]=0xeee;vdp.cram[2]=0xe;vdp.cram[3]=0xe0;
    raster.width=320;raster.count=320;raster.display=1;
    for(unsigned x=0;x<320;x++)native[x]=0xff123456;
    assert(db1_video.configure("16:9"));
    db1_video_reset();db1_video_observe(0x1c40);
    draw(0);draw(176); /* includes actors past entry 36 */
    assert(out[16]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    assert(out[188]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    draw(16);
    assert(out[272]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    /* A native-world pixel under a transparent HUD must never be copied right. */
    assert(out[286]!=native[208]);
    raster.plane_a[208]=0xc2;
    draw(16);assert(out[286]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.plane_a[208]=0;
    /* A native body hidden beneath the old HUD must not become a second body
     * after moving the HUD. Only classified HUD sprites change coordinates. */
    raster.sprite_owner[240]=2;raster.sprite[240]=2;raster.opaque[240]=1;
    vdp.vram[8+2]=15;be16(vdp.vram+8+4,0x2e6);
    draw(16);assert(out[318]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.sprite_owner[240]=0;
    raster.sprite_owner[240]=3;raster.sprite[240]=2;raster.opaque[240]=1;
    be16(vdp.vram+16+4,0x152);be16(vdp.vram+16+6,240+128);
    draw(16);assert(out[318]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.sprite_owner[240]=0;
    /* Original UI sprite stays ahead of host actors. */
    raster.sprite_owner[16]=1;raster.sprite[16]=2;raster.opaque[16]=1;raster.high[16]=1;
    be16(vdp.vram+4,0x2df);
    draw(16);assert(out[16]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* High-priority terrain covers a low-priority actor. */
    raster.sprite_owner[16]=0;raster.plane_b[16]=0xc3;
    draw(16);assert(out[16]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    raster.plane_b[16]=0;
    g_ram[0x5f34+16+5]=16;
    g_ram[0x5f34+32+4]=0;g_ram[0x5f34+32+5]=32;g_ram[0x5f34+32+6]=240;
    memset(vdp.vram+0x2e3*32,0x22,32);
    g_ram[0x152]=1;draw(0);draw(24);
    assert(out[16]==genesis_dac_cram_to_argb(vdp.cram[1],0)); /* front body hides rear selection */
    g_ram[0x152]=2;draw(24);
    assert(out[16]==genesis_dac_cram_to_argb(vdp.cram[2],0)); /* front selection remains visible */
    /* Map overlay and battle exit return to centered original image. */
    db1_video_observe(0x19980);draw(16);assert(out[39]==native[0]);
    db1_video_observe(0x1b040);db1_video_observe(0x1c44);draw(16);assert(out[39]==native[0]);
    assert(calls==13);
    assert(!db1_video.configure("bad"));
    assert(db1_video.configure("off")&&!db1_video.enabled());
    assert(db1_video.configure("fit"));
    assert(db1_video.width(1920,1080,320,224)==398);
    assert(db1_video.width(2560,1440,320,224)==398);
    assert(db1_video.width(3840,2160,320,224)==398);
    assert(db1_video.width(800,600,320,224)==320);
    assert(db1_video.width(16000,1000,320,224)==768);
    int cw,ch;
    assert(db1_video.canvas_geometry && db1_video.canvas_complete);
    assert(db1_video.canvas_geometry(1920,1080,320,224,&cw,&ch)==4&&cw==480&&ch==270);
    assert(db1_video.canvas_geometry(2560,1440,320,224,&cw,&ch)==6&&cw==427&&ch==240);
    assert(db1_video.canvas_geometry(3840,2160,320,224,&cw,&ch)==9&&cw==427&&ch==240);
    /* Added rows use authoritative world/actors, including actors above the
     * native top, and never retain the row provider or live RAM. */
    memset(g_ram,0,sizeof g_ram);memset(&raster,0,sizeof raster);
    raster.width=320;raster.count=320;raster.display=1;
    memset(g_ram+0x7bf8,1,1600);g_ram[0x7bd4]=10;g_ram[0x7bd5]=10;
    for(unsigned t=0;t<4;t++)be16(g_ram+0x60a+2*t,0);
    memset(vdp.vram+0x52*32,0x33,32);vdp.cram[51]=0x0e0;
    g_ram[0x6f34+9*80+10*2]=253;
    g_ram[0x5f34+253*16]=1;g_ram[0x5f34+253*16+5]=16;
    assert(db1_video.canvas_geometry(1920,1080,320,224,&cw,&ch)==4);
    db1_video_observe(0x1c40);
    memcpy(ram_copy,g_ram,sizeof g_ram);
    for(int y=0;y<224;y++)db1_video.scanline_presented(&vdp,y,320,canvas+(y+23)*480,480,row,0,&raster);
    assert(!memcmp(ram_copy,g_ram,sizeof g_ram));
    g_ram[0x7bd5]=39; /* Completion must use the captured scene, not this change. */
    memcpy(ram_copy,g_ram,sizeof g_ram);
    unsigned completed_calls=calls;
    db1_video.canvas_complete(&vdp,canvas,480,480,270,23,224);
    assert(calls==completed_calls&&!memcmp(ram_copy,g_ram,sizeof g_ram));
    assert(canvas[0]==genesis_dac_cram_to_argb(vdp.cram[51],0));
    assert(canvas[18*480+16]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    assert(canvas[269*480+479]==genesis_dac_cram_to_argb(vdp.cram[51],0));
    /* White is tied to the authored branding scene, never inferred from a
     * black gameplay frame. The original 320 pixels stay untouched. */
    db1_video_reset();db1_video_observe(0x1c5a4);
    assert(db1_video.canvas_geometry(1920,1080,320,224,&cw,&ch)==4);
    db1_video.scanline_presented(&vdp,0,320,out,398,row,0,NULL);
    assert(out[0]==0xffffffffu&&out[39]==native[0]);
    db1_video.canvas_complete(&vdp,canvas,480,480,270,23,224);
    assert(canvas[0]==0xffffffffu&&canvas[269*480+479]==0xffffffffu);
    db1_video_reset();db1_video_observe(0x1c40);
    memset(&raster,0,sizeof raster);raster.display=1;
    memset(g_ram,0,sizeof g_ram);
    db1_video.scanline_presented(&vdp,0,320,out,398,row,0,&raster);
    assert(out[0]!=0xffffffffu);
    /* Stage intro's falling egg uses tile 5D6, temporarily sharing the actor
     * cache range. Preserve it at its spawn-world coordinate, not as HUD. */
    db1_video_reset();db1_video_observe(0x1b42);
    vdp.vram[2]=15;be16(vdp.vram+4,0x5d6);be16(vdp.vram+6,16+128);
    raster.sprite_owner[16]=1;raster.sprite[16]=2;raster.opaque[16]=1;
    db1_video.scanline_presented(&vdp,0,320,out,398,row,0,&raster);
    db1_video.scanline_presented(&vdp,16,320,out,398,row,0,&raster);
    assert(out[16]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Classification uses the sprite's origin, never each pixel's X: a world
     * cursor crossing x208 must remain one intact sprite. */
    be16(vdp.vram+4,0x2af);be16(vdp.vram+6,200+128);
    raster.sprite_owner[16]=0;raster.sprite_owner[210]=1;
    raster.sprite[210]=2;raster.opaque[210]=1;
    db1_video.scanline_presented(&vdp,16,320,out,398,row,0,&raster);
    assert(out[210]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    assert(out[288]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Weather lists are world coordinates, not the old six-column mask. */
    db1_video_reset();db1_video_observe(0x1c40);
    memset(g_ram,0,sizeof g_ram);memset(&raster,0,sizeof raster);raster.display=1;
    g_ram[0x7bd4]=10;g_ram[0x7bd5]=10;g_ram[0x168]=4;
    g_ram[0x181a]=17;g_ram[0x181b]=10;g_ram[0x181c]=255;
    for(unsigned i=0;i<16;i++)be16(g_rom+0x64f4+i*2,0x2be);
    memset(vdp.vram+0x2be*32,0x22,32);
    draw(0);draw(16);assert(out[240]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    for(unsigned i=0;i<16;i++)be16(g_rom+0x6514+i*2,0x2c4);
    memset(vdp.vram+0x2c4*32,0x33,32);g_ram[0x168]=4|64;
    draw(16);assert(out[240]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    g_ram[0x168]=4;g_ram[0x181a]=18;raster.plane_a[208]=0xc3;
    draw(16);assert(out[286]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    raster.plane_a[208]=0;
    g_ram[0x168]=0;draw(16);assert(out[240]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Lightning's actual SAT geometry extends past native raster column320. */
    vdp.reg[5]=0x78;
    be16(vdp.vram+0xf000,16+128);vdp.vram[0xf002]=15;vdp.vram[0xf003]=0;
    be16(vdp.vram+0xf004,0x100);be16(vdp.vram+0xf006,310+128);
    memset(vdp.vram+0x100*32,0x22,512);
    draw(16);assert(out[330]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.plane_a[250]=0xc3;draw(16);
    assert(out[328]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    raster.plane_a[250]=0;be16(vdp.vram+0xf004,0x2e6);
    draw(16);assert(out[330]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Spell effects reuse the ordinary HUD art range, but remain world-space. */
    be16(vdp.vram+0xf004,0x152);memset(vdp.vram+0x152*32,0x22,512);
    draw(16);assert(out[330]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    db1_video_observe(0x5ff6);draw(16);
    assert(out[330]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    db1_video_observe(0x62fc);draw(16);
    assert(out[330]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Actual B scroll can shake independently of the camera RAM. Its terrain
     * phase must continue across the old 208px composition boundary. */
    memset(vdp.vram+0xf000,0,8);memset(g_ram+0x7bf8,1,1600);
    memset(g_ram+0x20a,0,4);for(unsigned i=0;i<4;i++)be16(g_ram+0x60a+i*2,0);
    vdp.reg[4]=7;vdp.reg[13]=0x34;
    be16(g_ram+0x200,0xffb0);be16(g_ram+0x202,16);
    be16(vdp.vram+0xd002,0xffb4);vdp.vsram[1]=16;
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<4;x++)vdp.vram[0x52*32+y*4+x]=(uint8_t)(0x12+x*0x22);
    vdp.cram[53]=0xeee;
    draw(16);assert(out[336]==genesis_dac_cram_to_argb(vdp.cram[53],0));
    be16(vdp.vram+0xd002,0xffb0);vdp.vsram[1]=19;
    for(unsigned y=0;y<8;y++)memset(vdp.vram+0x52*32+y*4,(int)((y+1)*17),4);
    vdp.cram[52]=0x0e0;
    memcpy(ram_copy,g_ram,sizeof g_ram);
    vdp.sprite_overflow=1;vdp.sprite_collision=1;
    memcpy(&vdp_copy,&vdp,sizeof vdp);
    draw(16);assert(out[336]==genesis_dac_cram_to_argb(vdp.cram[52],0));
    assert(!memcmp(ram_copy,g_ram,sizeof g_ram));
    assert(!memcmp(&vdp_copy,&vdp,sizeof vdp));
    /* Native lightning 132D8 authors the entire A plane, not the normal 6x6
     * weather mask. It must not be split into old-world and relocated HUD. */
    memset(&raster,0,sizeof raster);raster.display=1;
    memset(vdp.vram+0x2be*32,0x22,32);
    vdp.reg[2]=0x30;vdp.reg[16]=1;vdp.reg[11]=0;vdp.vsram[0]=0;
    be16(vdp.vram+0xd000,0);
    for(unsigned x=0;x<64;x++)be16(vdp.vram+0xc000+x*2,0x82be);
    db1_video_observe(0x132d8);draw(0);
    assert(out[210]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    assert(out[330]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    db1_video_observe(0x13630);draw(0);
    assert(out[210]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    return 0;
}
