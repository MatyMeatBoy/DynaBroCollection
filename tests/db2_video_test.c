#include "db2_video.h"
#include "genesis_runtime.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8_t g_rom[0x400000],g_ram[65536];
M68KState g_cpu;
static GVDP v;
static GVDPRasterRow r;
static uint32_t original[320],out[768],canvas[480*480];
static unsigned calls;
static void w16(uint8_t *p,unsigned x){p[0]=(uint8_t)(x>>8);p[1]=(uint8_t)x;}
static void w32(uint8_t *p,unsigned x){w16(p,x>>16);w16(p+2,x);}
static const uint32_t *native(void *unused){(void)unused;calls++;return original;}
static void row(int y,int w){db2_video.scanline_presented(&v,y,320,out,w,native,NULL,&r);}
static void setup(void){
 memset(&v,0,sizeof v);memset(&r,0,sizeof r);memset(g_ram,0,sizeof g_ram);
 g_ram[0x2a56]=0;g_ram[0x2a57]=255;g_ram[0x2a58]=0x9c;g_ram[0x2a59]=0x8e;
 g_ram[0x9c8e]=15;g_ram[0x9c8f]=15;
 /* ring anchors16,16: ring14,30 with zero scroll. */
 g_ram[0x2c08]=14;g_ram[0x2c09]=30;
 for(unsigned i=0;i<1600;i++)g_ram[0x9fd1+i*2]=1;
 memset(v.vram+0x75*32,255,32);v.cram[63]=0xeee;v.cram[2]=0x00e;v.cram[3]=0x0e0;
 r.display=1;r.width=320;
 for(unsigned i=0;i<320;i++){original[i]=0xff123456;r.plane_b[i]=0x7f;}
 r.plane_a[216]=0x43;
 /* Legacy synthetic polarity is explicit: A=UI E000, B=terrain C000. */
 v.reg[2]=0x38;v.reg[4]=6;
 w16(g_ram+0x2a5a,0xe036);w16(g_ram+0x2a5c,0xe004);
 w16(g_ram+0x2a5e,216);w16(g_ram+0x2a60,16);
 calls=0;db2_video_reset();assert(db2_video.configure("16:9"));
}
static void actual_plane_contract(void){
 setup();v.reg[2]=0x30;v.reg[4]=7;
 uint8_t tmp[320];memcpy(tmp,r.plane_a,320);memcpy(r.plane_a,r.plane_b,320);memcpy(r.plane_b,tmp,320);
 db2_video_observe(0x3bb0);row(0,427);
 assert(out[100]==genesis_dac_cram_to_argb(v.cram[63],0));
 /* Equal-priority PlaneA terrain wins over PlaneB UI, as on hardware. */
 assert(out[323]==genesis_dac_cram_to_argb(v.cram[63],0));
 r.plane_b[216]=0xc3;row(1,427);
 assert(out[323]==genesis_dac_cram_to_argb(v.cram[3],0));
 /* Native world-plane UI overlay keeps its original relative placement. */
 r.plane_b[20]=0xc3;row(1,427);
 assert(out[70]==genesis_dac_cram_to_argb(v.cram[3],0));
 assert(out[20]==genesis_dac_cram_to_argb(v.cram[63],0));
}
static void owned_sprite_mode(unsigned source,unsigned attr,unsigned size,int x,int y,int expected,unsigned mode){
 setup();g_ram[0x2a88]=(uint8_t)mode;uint8_t *p=g_ram+0x2752+8*source;
 w16(p,(unsigned)y);w16(p+2,size<<8);w16(p+4,attr);w16(p+6,(unsigned)x);
 w16(v.vram,(unsigned)y+128);w16(v.vram+2,size<<8);w16(v.vram+4,attr);w16(v.vram+6,(unsigned)x+128);
 unsigned color=3+((attr>>9)&48);v.cram[color]=0x0e0;
 r.sprite_owner[x]=1;r.sprite[x]=(uint8_t)color;r.opaque[x]=1;r.high[x]=(uint8_t)(attr>>15);
 db2_video_observe(0x3bb0);row(0,427);row(y,427);
 assert(out[expected]==genesis_dac_cram_to_argb(v.cram[color],0));
}
static void owned_sprite_contract(unsigned source,unsigned attr,unsigned size,int x,int y,int expected){
 owned_sprite_mode(source,attr,size,x,y,expected,3);
}
int main(void){
 /* Demo ownership is explicit and independent of campaign/player acceptance. */
 setup();db2_video_observe(0xf794);assert(db2_video.raster_enabled(&v));
 db2_video_observe(0xf728);assert(!db2_video.raster_enabled(&v));
 db2_video_observe(0x3bb0);assert(db2_video.raster_enabled(&v));
 db2_video_observe(0x3bb4);assert(!db2_video.raster_enabled(&v));
 actual_plane_contract();
 for(unsigned mode=0;mode<=2;mode++)owned_sprite_mode(6,0x82f4,10,260,178,367,mode);
 owned_sprite_mode(6,0x82f4,10,260,178,367,0x82);
 owned_sprite_mode(6,0x82f4,10,100,20,150,5);
 owned_sprite_contract(12,0x86e1,5,258,62,365); /* Minimap owner follows HUD. */
 owned_sprite_contract(13,0x8372,5,258,62,365);
 owned_sprite_contract(10,0x96e8,5,192,16,242); /* Right corner stays192-wide. */
 owned_sprite_contract(14,0x836a,13,296,32,403);
 owned_sprite_contract(15,0x833f,13,296,32,403);
 for(unsigned source=22;source<=30;source++){
  owned_sprite_contract(source,0x8364,9,224,152,331);
  owned_sprite_contract(source,0x8360,5,224,152,331);
 }
 for(unsigned palette=0;palette<4;palette++)owned_sprite_contract(22,0x87e9+0x2000*palette,15,248,152,355);
 owned_sprite_contract(6,0x82f4,10,100,20,150); /* World cursor follows world. */
 owned_sprite_contract(31,0x032f,15,100,20,150);
 /* Valid-looking temporary record16 is not blanket-classified as HUD. */
 setup();w16(g_ram+0x2752+16*8+4,0x82fd);w16(g_ram+0x2752+16*8+2,0x0f00);
 w16(v.vram+4,0x82fd);w16(v.vram+2,0x0f00);w16(v.vram,128);w16(v.vram+6,128);
 v.vram[0x2fd*32]=0x11; /* Visible pattern data from an unowned entry: its native pixels are
  * kept in the widened view (placed by region) instead of rolling the frame back to 4:3. */
 r.sprite_owner[0]=1;r.sprite[0]=1;r.opaque[0]=1;
 db2_video_observe(0x3bb0);row(0,427);row(1,427);assert(out[53]!=original[0]);
 setup();v.reg[2]=0;db2_video_observe(0x3bb0);row(0,427);assert(out[53]==original[0]);
 setup();w16(g_ram+0x2a60,112);db2_video_observe(0x3bb0);row(0,427);assert(out[53]==original[0]);
 setup();row(0,398);assert(calls==1&&out[39]==original[0]&&out[0]==0xff000000);
 db2_video_observe(0x3bb0);row(0,398);
 assert(calls==2);assert(out[100]==genesis_dac_cram_to_argb(v.cram[63],0));
 assert(out[216]==genesis_dac_cram_to_argb(v.cram[63],0));
 assert(out[294]==genesis_dac_cram_to_argb(v.cram[3],0));
 assert(out[295]==genesis_dac_cram_to_argb(v.cram[63],0));
 uint8_t saved[65536];memcpy(saved,g_ram,sizeof saved);row(10,398);assert(!memcmp(saved,g_ram,sizeof saved));
 g_ram[0x286]=128;row(11,398);assert(calls==4&&out[39]==original[0]);
 g_ram[0x286]=0;r.plane_a[20]=0x41;row(0,398);assert(calls==5);r.plane_a[20]=0;
 row(0,398);int w,h;assert(db2_video.canvas_geometry(1920,1080,320,224,&w,&h)==4&&w==480&&h==270);
 memset(canvas,0,sizeof canvas);db2_video.canvas_complete(&v,canvas,480,480,270,23,224);
 assert(canvas[0]==genesis_dac_cram_to_argb(v.cram[63],0));
 /* Completion uses owned state, even if live RAM/VDP change. */
 memset(g_ram,0,sizeof g_ram);memset(&v,0,sizeof v);db2_video.canvas_complete(&v,canvas,480,480,270,23,224);
 assert(canvas[0]==0xffffffff);
 assert(db2_video.canvas_geometry(2560,1440,320,224,&w,&h)==6&&w==427&&h==240);
 assert(db2_video.canvas_geometry(3840,2160,320,224,&w,&h)==9&&w==427&&h==240);
 db2_video_observe(0x734e);assert(!db2_video.raster_enabled(&v));
 /* A 41st actor, absent from all native SAT/cache entries, is still drawn
  * beyond the original right edge. All animation bytes are guest-derived. */
 setup();w32(g_rom+0x78810,0x1000);w32(g_rom+0x78814,0x1100);w32(g_rom+0x78804,0x1200);
 w32(g_rom+0x1000,0x1300);w32(g_rom+0x1100,0x1200);w16(g_rom+0x1200,0x20);w32(g_rom+0x1300,0x1400);
 w16(g_rom+0x1400,512);g_rom[0x1403]=0;g_rom[0x1404]=0xab;
 unsigned pos=0x1405;for(unsigned i=0;i<3;i++){g_rom[pos++]=255;g_rom[pos++]=0;}g_rom[pos++]=254;g_rom[pos]=0;
 g_rom[0x1220]=0;g_rom[0x1221]=0;g_rom[0x1222]=5;
 for(unsigned id=1;id<=41;id++){uint8_t *rec=g_ram+0x5f58+48*id;rec[0]=1;rec[2]=15;rec[3]=15;}
 g_ram[0x5f58+48*41+2]=26;v.cram[26]=0xe;v.cram[27]=0xe0;
 /* A 42nd actor with an unconfigured type (db2_actor_decode fails on
  * type>92) must not drop the whole frame to native fallback -- only
  * that actor's own sprite should be skipped. Regression coverage for
  * the 2026-11-18 fix mirroring the DB1 prepare_actors()/center_supported
  * fix (confirmed live: reason 04 fired during ordinary DB2 battle play,
  * not just the documented pre-battle window). */
 g_ram[0x5f58+48*42]=200;
 db2_video_observe(0x3bb0);row(0,480);row(4,480);
 assert(calls==2&&out[444]==genesis_dac_cram_to_argb(v.cram[26],0));
 /* A late unknown world sprite at x300 must neither become HUD nor leave
  * earlier enhanced rows mixed with native fallback. Test both canvases. */
 setup();db2_video_observe(0x3bb0);
 for(int y=0;y<224;y++){
  if(y==100){r.sprite_owner[300]=1;r.sprite[300]=1;r.opaque[300]=1;v.vram[0x6f1*32]=0x11;w16(v.vram+4,0x6f1);w16(v.vram+6,428);w16(v.vram,228);v.vram[2]=5;}
  row(y,398);memcpy(canvas+(size_t)y*480,out,398*sizeof *out);
 }
 db2_video.frame_complete(&v,canvas,480,398,224,0,224);
 /* The unknown entry keeps its native pixels in place; no row of the frame is
  * left as native fallback (no mixed enhanced/native frame, no 4:3 flash). */
 for(int y=0;y<224;y++)assert(canvas[(size_t)y*480+39]!=original[0]);
 /* Per-row palette updates apply to BOTH sides of old native boundary. */
 setup();db2_video_observe(0x3bb0);row(0,398);v.cram[63]=0xe;row(1,398);
 assert(out[100]==out[216]&&out[216]==genesis_dac_cram_to_argb(0xe,0));
 /* Native plane mismatch from unknown weather/scroll also rolls back all. */
 r.plane_b[10]=0x41;row(2,398);memcpy(canvas+2*480,out,398*sizeof *out);
 db2_video.frame_complete(&v,canvas,480,398,224,0,224);assert(canvas[39]==original[0]);
 puts("DB2 video synthetic terrain/HUD/fallback/snapshot/canvas contracts passed");return 0;
}
