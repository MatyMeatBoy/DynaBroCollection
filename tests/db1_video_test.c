#include "db1_video.h"
#include "db1_visor.h"
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
static int world_x(int x){Db1VisorLayout v;assert(db1_visor_layout(398,0,&v));return x+v.world_shift_x;}
static int canvas_x(int x){Db1VisorLayout v;assert(db1_visor_layout(480,0,&v));return x+v.world_shift_x;}
static const uint32_t *row(void *ctx){(void)ctx;calls++;return native;}
static void be16(uint8_t *p,unsigned v){p[0]=(uint8_t)(v>>8);p[1]=(uint8_t)v;}
static void be32(uint8_t *p,unsigned v){be16(p,v>>16);be16(p+2,v);}
static void draw(int line){
    unsigned before=calls;
    db1_video.scanline_presented(&vdp,line,320,out,398,row,0,&raster);
    assert(calls==before+1);
}
static void frame_draw(int line){draw(0);if(line)draw(line);}
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
    draw(0);frame_draw(176); /* includes actors past entry 36 */
    assert(out[world_x(16)]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    assert(out[world_x(188)]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    frame_draw(16);
    assert(out[world_x(272)]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    /* A native-world pixel under a transparent HUD must never be copied right. */
    assert(out[286]!=native[208]);
    raster.plane_a[208]=0xc2;
    frame_draw(16);assert(out[world_x(208)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.plane_a[208]=0;
    /* A native body hidden beneath the old HUD must not become a second body
     * after moving the HUD. Only classified HUD sprites change coordinates. */
    raster.sprite_owner[240]=2;raster.sprite[240]=2;raster.opaque[240]=1;
    vdp.vram[8+2]=15;be16(vdp.vram+8+4,0x2e6);
    frame_draw(16);assert(out[318]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.sprite_owner[240]=0;
    raster.sprite_owner[240]=3;raster.sprite[240]=2;raster.opaque[240]=1;
    be16(vdp.vram+16+4,0x152);be16(vdp.vram+16+6,240+128);
    frame_draw(16);assert(out[318]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.sprite_owner[240]=0;
    /* Original UI sprite stays ahead of host actors. */
    raster.sprite_owner[16]=1;raster.sprite[16]=2;raster.opaque[16]=1;raster.high[16]=1;
    be16(vdp.vram+4,0x2df);
    frame_draw(16);assert(out[world_x(16)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* High-priority terrain covers a low-priority actor. */
    raster.sprite_owner[16]=0;raster.plane_b[16]=0xc3;
    frame_draw(16);assert(out[world_x(16)]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    raster.plane_b[16]=0;
    g_ram[0x5f34+16+5]=16;
    g_ram[0x5f34+32+4]=0;g_ram[0x5f34+32+5]=32;g_ram[0x5f34+32+6]=240;
    memset(vdp.vram+0x2e3*32,0x22,32);
    g_ram[0x152]=1;draw(0);frame_draw(24);
    assert(out[world_x(16)]==genesis_dac_cram_to_argb(vdp.cram[1],0)); /* front body hides rear selection */
    g_ram[0x152]=2;frame_draw(24);
    assert(out[world_x(16)]==genesis_dac_cram_to_argb(vdp.cram[2],0)); /* front selection remains visible */
    /* Map overlay and battle exit return to centered original image. */
    db1_video_observe(0x19980);frame_draw(16);assert(out[39]==native[0]);
    db1_video_observe(0x1b040);db1_video_observe(0x1c44);frame_draw(16);assert(out[39]==native[0]);
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
    assert(canvas[18*480+canvas_x(16)]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    assert(canvas[269*480+479]==genesis_dac_cram_to_argb(vdp.cram[51],0));
    /* Intro margins follow the native backdrop through white, fade and black.
     * The original 320 pixels stay untouched and completion owns its snapshot. */
    db1_video_reset();db1_video_observe(0x1c5a4);
    assert(db1_video.canvas_geometry(1920,1080,320,224,&cw,&ch)==4);
    vdp.cram[0]=0xeee;vdp.reg[7]=0;
    db1_video.scanline_presented(&vdp,0,320,out,398,row,0,NULL);
    assert(out[0]==0xffffffffu&&out[39]==native[0]);
    db1_video.canvas_complete(&vdp,canvas,480,480,270,23,224);
    assert(canvas[0]==0xffffffffu&&canvas[269*480+479]==0xffffffffu);
    for(unsigned n=0;n<8;n++) {
        vdp.cram[0]=(uint16_t)(n*0x222);
        uint32_t expected=genesis_dac_cram_to_argb(vdp.cram[0],0);
        db1_video.scanline_presented(&vdp,0,320,out,398,row,0,NULL);
        assert(out[0]==expected&&out[397]==expected&&out[39]==native[0]);
        vdp.cram[0]=0; /* completion must retain this raster's palette */
        db1_video.canvas_complete(&vdp,canvas,480,480,270,23,224);
        assert(canvas[0]==expected&&canvas[269*480+479]==expected);
    }
    db1_video.scanline_presented(&vdp,0,320,out,398,row,0,NULL);
    assert(out[0]==0xff000000u&&out[397]==0xff000000u);
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
    assert(out[world_x(16)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Classification uses the sprite's origin, never each pixel's X: a world
     * cursor crossing x208 must remain one intact sprite. */
    be16(vdp.vram+4,0x2af);be16(vdp.vram+6,200+128);
    raster.sprite_owner[16]=0;raster.sprite_owner[210]=1;
    raster.sprite[210]=2;raster.opaque[210]=1;
    db1_video.scanline_presented(&vdp,16,320,out,398,row,0,&raster);
    assert(out[world_x(210)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    assert(out[288]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Weather lists retain native six-by-six display culling and world identity. */
    db1_video_reset();db1_video_observe(0x1c40);
    memset(g_ram,0,sizeof g_ram);memset(&raster,0,sizeof raster);raster.display=1;
    g_ram[0x7bd4]=10;g_ram[0x7bd5]=10;g_ram[0x168]=4;
    g_ram[0x181a]=15;g_ram[0x181b]=10;g_ram[0x181c]=255;
    for(unsigned i=0;i<16;i++)be16(g_rom+0x64f4+i*2,0x2be);
    memset(vdp.vram+0x2be*32,0x22,32);
    draw(0);frame_draw(16);assert(out[world_x(176)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    for(unsigned i=0;i<16;i++)be16(g_rom+0x6514+i*2,0x2c4);
    memset(vdp.vram+0x2c4*32,0x33,32);g_ram[0x168]=4|64;
    frame_draw(16);assert(out[world_x(176)]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    g_ram[0x168]=4;g_ram[0x181a]=18;raster.plane_a[216]=0xc3;
    frame_draw(16);assert(out[294]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    raster.plane_a[216]=0;
    g_ram[0x168]=0;frame_draw(16);assert(out[world_x(176)]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Lightning's actual SAT geometry extends past native raster column320. */
    vdp.reg[5]=0x78;
    be16(vdp.vram+0xf000,16+128);vdp.vram[0xf002]=15;vdp.vram[0xf003]=0;
    be16(vdp.vram+0xf004,0x100);be16(vdp.vram+0xf006,310+128);
    memset(vdp.vram+0x100*32,0x22,512);
    frame_draw(16);assert(out[world_x(330)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    raster.plane_a[250]=0xc3;frame_draw(16);
    assert(out[328]==genesis_dac_cram_to_argb(vdp.cram[3],0));
    raster.plane_a[250]=0;be16(vdp.vram+0xf004,0x2e6);
    frame_draw(16);assert(out[world_x(330)]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Spell effects reuse the ordinary HUD art range, but remain world-space. */
    be16(vdp.vram+0xf004,0x152);memset(vdp.vram+0x152*32,0x22,512);
    frame_draw(16);assert(out[world_x(330)]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    db1_video_observe(0x5ff6);frame_draw(16);
    assert(out[world_x(330)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    db1_video_observe(0x62fc);frame_draw(16);
    assert(out[world_x(330)]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Actual B scroll can shake independently of the camera RAM. Its terrain
     * phase must continue across the old 208px composition boundary. */
    memset(vdp.vram+0xf000,0,8);memset(g_ram+0x7bf8,1,1600);
    memset(g_ram+0x20a,0,4);for(unsigned i=0;i<4;i++)be16(g_ram+0x60a+i*2,0);
    vdp.reg[4]=7;vdp.reg[13]=0x34;
    be16(g_ram+0x200,0xffb0);be16(g_ram+0x202,16);
    be16(vdp.vram+0xd002,0xffb4);vdp.vsram[1]=16;
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<4;x++)vdp.vram[0x52*32+y*4+x]=(uint8_t)(0x12+x*0x22);
    vdp.cram[53]=0xeee;
    frame_draw(16);assert(out[world_x(336)]==genesis_dac_cram_to_argb(vdp.cram[53],0));
    be16(vdp.vram+0xd002,0xffb0);vdp.vsram[1]=19;
    for(unsigned y=0;y<8;y++)memset(vdp.vram+0x52*32+y*4,(int)((y+1)*17),4);
    vdp.cram[52]=0x0e0;
    memcpy(ram_copy,g_ram,sizeof g_ram);
    vdp.sprite_overflow=1;vdp.sprite_collision=1;
    memcpy(&vdp_copy,&vdp,sizeof vdp);
    frame_draw(16);assert(out[world_x(336)]==genesis_dac_cram_to_argb(vdp.cram[52],0));
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
    assert(out[world_x(210)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    assert(out[world_x(319)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    assert(out[world_x(320)]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    db1_video_observe(0x13630);draw(0);
    assert(out[world_x(210)]!=genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Camera changes are committed as one frame: transient RAM writes during
     * scanout cannot split a body. Both signs and tile-origin rollover must
     * appear together at the next frame boundary.
     * Empty native planes isolate the extended-world sprite boundaries. */
    db1_video_reset();db1_video_observe(0x1c40);
    memset(g_ram,0,sizeof g_ram);memset(&vdp,0,sizeof vdp);
    memset(&raster,0,sizeof raster);raster.display=1;
    vdp.cram[1]=0xeee;
    g_ram[0x7bd4]=7;g_ram[0x7bd5]=7;
    g_ram[0x6f34+8*80+15*2]=1;g_ram[0x5f34+16]=1;
    draw(0);draw(40);
    uint32_t body=genesis_dac_cram_to_argb(vdp.cram[1],0);
    uint32_t blank=genesis_dac_cram_to_argb(vdp.cram[0],0);
    assert(out[world_x(272)]==body&&out[world_x(303)]==body&&out[world_x(304)]==blank);
    be16(g_ram+0x204,8);draw(40);
    assert(out[world_x(272)]==body&&out[world_x(303)]==body&&out[world_x(304)]==blank);
    draw(0);draw(40);
    assert(out[world_x(272)]==blank&&out[world_x(280)]==body&&out[world_x(311)]==body);
    be16(g_ram+0x204,0xfff8);draw(0);draw(40);
    assert(out[world_x(264)]==body&&out[world_x(295)]==body&&out[world_x(296)]==blank);
    /* Equivalent camera representation across a 32px coarse-cell rollover. */
    be16(g_ram+0x204,24);g_ram[0x7bd4]=8;draw(0);draw(40);
    assert(out[world_x(264)]==body&&out[world_x(295)]==body&&out[world_x(296)]==blank);
    be16(g_ram+0x206,8);draw(0);draw(40);assert(out[world_x(264)]==blank);
    draw(48);assert(out[world_x(264)]==body);
    be16(g_ram+0x206,0xfff8);draw(0);draw(32);assert(out[world_x(264)]==body);
    draw(64);assert(out[world_x(264)]==blank);
    be16(g_ram+0x206,24);g_ram[0x7bd5]=8;
    draw(0);draw(32);assert(out[world_x(264)]==body);draw(64);assert(out[world_x(264)]==blank);
    /* A guest occupancy/depth update invalidates the same projection cache. */
    g_ram[0x6f34+8*80+15*2]=0;g_ram[0x6f34+8*80+16*2]=1;
    draw(0);draw(32);assert(out[world_x(264)]==blank&&out[world_x(296)]==body);
    g_ram[0x5f34+16+4]=8;draw(0);draw(32);
    assert(out[world_x(296)]==blank&&out[world_x(304)]==body);
    /* Both canvas margins share the frame's geometry even when live camera
     * RAM changes before line223. Completion retains owned palette/state. */
    memset(g_ram,0,sizeof g_ram);
    g_ram[0x6f34+6*80+8*2]=1;g_ram[0x5f34+16]=1;
    g_ram[0x5f34+16+5]=16;
    assert(db1_video.canvas_geometry(1920,1080,320,224,&cw,&ch)==4);
    db1_video.scanline_presented(&vdp,0,320,canvas+23*480,480,row,0,&raster);
    be16(g_ram+0x206,8);
    db1_video.scanline_presented(&vdp,223,320,canvas+246*480,480,row,0,&raster);
    memcpy(ram_copy,g_ram,sizeof g_ram);memcpy(&vdp_copy,&vdp,sizeof vdp);
    memset(g_ram,0,sizeof g_ram);memset(&vdp,0,sizeof vdp);
    db1_video.canvas_complete(&vdp,canvas,480,480,270,23,224);
    assert(canvas[246*480+canvas_x(272)]==body); /* same body continues from native y216 */
    assert(canvas[247*480+canvas_x(272)]==body&&canvas[269*480+canvas_x(272)]==body);
    assert(!memcmp(g_ram,(uint8_t[65536]){0},sizeof g_ram));
    /* Native96C6 changes the scroll base before its VBlank commit, then
     * changes coarse/ring fields before204/206. These equivalent phases must
     * project the same body, rather than introduce a32px jump or missing row. */
    db1_video_reset();db1_video_observe(0x1c40);
    memset(g_ram,0,sizeof g_ram);memset(&vdp,0,sizeof vdp);
    memset(&raster,0,sizeof raster);raster.display=1;vdp.cram[1]=0xeee;
    g_ram[0x7bd4]=7;g_ram[0x7bd5]=7;
    g_ram[0x6f34+8*80+15*2]=1;g_ram[0x5f34+16]=1;
    vdp.reg[4]=7;vdp.reg[13]=0x34;
    be16(g_ram+0x200,0xffb0);be16(g_ram+0x202,16);
    be16(vdp.vram+0xd002,0xffb0);vdp.vsram[1]=16;
    be16(g_ram+0x204,16);be16(g_ram+0x206,16); /* stale animation phase */
    draw(0);draw(40);assert(out[world_x(272)]==body&&out[world_x(304)]==blank);
    be16(g_ram+0x200,0xffa0); /* pending horizontal scroll, VDP still old */
    draw(0);draw(40);assert(out[world_x(272)]==body&&out[world_x(304)]==blank);
    be16(vdp.vram+0xd002,0xffa0); /* actual scroll commit */
    draw(0);draw(40);assert(out[world_x(256)]==body&&out[world_x(288)]==blank);
    g_ram[0x7bd4]=8;g_ram[0x208]=4; /* ring rollover, fine RAM still stale */
    draw(0);draw(40);assert(out[world_x(256)]==body&&out[world_x(288)]==blank);
    be16(g_ram+0x202,32); /* pending vertical commit */
    draw(0);draw(40);assert(out[world_x(256)]==body);
    vdp.vsram[1]=32;
    draw(0);draw(24);assert(out[world_x(256)]==body);draw(56);assert(out[world_x(256)]==blank);
    g_ram[0x7bd5]=8;g_ram[0x209]=4;
    draw(0);draw(24);assert(out[world_x(256)]==body);draw(56);assert(out[world_x(256)]==blank);
    /* Reverse commits and ring rollover preserve the same geometry too. */
    be16(g_ram+0x200,0xffb0);draw(0);draw(24);assert(out[world_x(256)]==body);
    be16(vdp.vram+0xd002,0xffb0);draw(0);draw(24);assert(out[world_x(272)]==body);
    g_ram[0x7bd4]=7;g_ram[0x208]=0;
    draw(0);draw(24);assert(out[world_x(272)]==body);
    be16(g_ram+0x202,16);draw(0);draw(24);assert(out[world_x(272)]==body);
    vdp.vsram[1]=16;draw(0);draw(40);assert(out[world_x(272)]==body);
    g_ram[0x7bd5]=7;g_ram[0x209]=0;
    draw(0);draw(40);assert(out[world_x(272)]==body);
    vdp.cram[1]=0xe;draw(40); /* palette changes remain authentic raster state */
    assert(out[world_x(272)]==genesis_dac_cram_to_argb(vdp.cram[1],0));
    /* Terrain fade begins before1B42. Its foreground remains one centered
     * span across old column208, and native falling SAT egg is the only egg. */
    db1_video_reset();db1_video_observe(0x1b38);
    memset(g_ram,0,sizeof g_ram);memset(&vdp,0,sizeof vdp);
    memset(&raster,0,sizeof raster);raster.display=1;
    vdp.cram[1]=0xeee;vdp.cram[2]=0xe;
    g_ram[0x6f34]=1;g_ram[0x5f34+16]=1;
    vdp.vram[2]=15;be16(vdp.vram+4,0x5d6);be16(vdp.vram+6,16+128);
    raster.sprite_owner[16]=1;raster.sprite[16]=2;raster.opaque[16]=1;
    raster.plane_a[207]=0xc1;raster.plane_a[208]=0xc2;
    draw(0);draw(16);
    assert(out[world_x(16)]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    assert(out[world_x(17)]==blank); /* no second host egg at its destination */
    assert(out[246]==body&&out[247]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    assert(out[207]==blank&&out[286]==blank);
    /* Native ordinary actor rendering starts only after the drop completes. */
    db1_video_observe(0x1c04);draw(16);assert(out[world_x(17)]==body);
    /* The authored frame keeps exactly196 painted pixels, not a stretched
     * horizontal run. Its complete tile box is208px and HUD stays right. */
    db1_video_set_visor_offset_tiles(0);db1_video_reset();db1_video_observe(0x1c40);
    memset(g_ram,0,sizeof g_ram);memset(&vdp,0,sizeof vdp);
    memset(&raster,0,sizeof raster);raster.display=1;vdp.cram[1]=0xeee;vdp.cram[2]=0xe;
    for(int x=14;x<=209;x++)raster.plane_a[x]=0xc1;
    for(int x=216;x<320;x++)raster.plane_a[x]=0xc2;
    db1_video.scanline_presented(&vdp,0,320,canvas,427,row,0,&raster);
    db1_video.scanline_presented(&vdp,14,320,canvas,427,row,0,&raster);
    unsigned white=0;for(int x=0;x<427;x++)white+=canvas[x]==body;
    assert(white==196&&canvas[64]==body&&canvas[259]==body&&canvas[260]==blank);
    assert(canvas[323]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    db1_video_set_visor_offset_tiles(2); /* apply complete geometry next frame */
    db1_video.scanline_presented(&vdp,14,320,canvas,427,row,0,&raster);assert(canvas[64]==body);
    db1_video.scanline_presented(&vdp,0,320,canvas,427,row,0,&raster);
    db1_video.scanline_presented(&vdp,14,320,canvas,427,row,0,&raster);
    assert(canvas[64]==blank&&canvas[80]==body&&canvas[275]==body&&canvas[323]==genesis_dac_cram_to_argb(vdp.cram[2],0));
    /* Maximum positive tile offset needs actors beyond the old left overscan.
     * Complete bodies continue above/below native rows, outside visor borders. */
    db1_video_set_visor_offset_tiles(7);db1_video_reset();db1_video_observe(0x1c40);
    memset(g_ram,0,sizeof g_ram);memset(&vdp,0,sizeof vdp);
    memset(&raster,0,sizeof raster);raster.display=1;vdp.cram[1]=0xeee;
    g_ram[0x7bd4]=10;g_ram[0x7bd5]=10;
    g_ram[0x6f34+10*80+6*2]=1;g_ram[0x5f34+16]=1;
    g_ram[0x6f34+9*80+10*2]=2;g_ram[0x5f34+32]=1;g_ram[0x5f34+32+5]=16;
    g_ram[0x6f34+16*80+11*2]=129;g_ram[0x5f34+129*16]=1;g_ram[0x5f34+129*16+5]=16; /* upper-ID faction */
    assert(db1_video.canvas_geometry(2560,1440,320,224,&cw,&ch)==6&&cw==427&&ch==240);
    memcpy(ram_copy,g_ram,sizeof g_ram);memcpy(&vdp_copy,&vdp,sizeof vdp);
    for(int y=0;y<224;y++)db1_video.scanline_presented(&vdp,y,320,canvas+(y+8)*480,427,row,0,&raster);
    db1_video.canvas_complete(&vdp,canvas,480,427,240,8,224);
    assert(canvas[16*480]==body); /* nativeX-112 + shift106: body crosses left edge */
    assert(canvas[7*480+122]==body&&canvas[8*480+122]==body);
    assert(canvas[231*480+154]==body&&canvas[232*480+154]==body&&canvas[239*480+154]==body);
    assert(!memcmp(ram_copy,g_ram,sizeof g_ram)&&!memcmp(&vdp_copy,&vdp,sizeof vdp));
    db1_video_set_visor_offset_tiles(0);
    return 0;
}
