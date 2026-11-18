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
    assert(out[286]==native[208]);
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
    assert(calls==10);
    assert(!db1_video.configure("bad"));
    assert(db1_video.configure("off")&&!db1_video.enabled());
    assert(db1_video.configure("fit"));
    assert(db1_video.width(1920,1080,320,224)==398);
    assert(db1_video.width(2560,1440,320,224)==398);
    assert(db1_video.width(3840,2160,320,224)==398);
    assert(db1_video.width(800,600,320,224)==320);
    assert(db1_video.width(16000,1000,320,224)==768);
    return 0;
}
