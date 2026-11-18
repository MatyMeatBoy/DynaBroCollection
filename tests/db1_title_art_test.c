#include "db1_title_art.h"
#include "video/genesis_dac.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static GVDP v,before;
static void word(unsigned a,unsigned n){v.vram[a]=(unsigned char)(n>>8);v.vram[a+1]=(unsigned char)n;}
int main(void){
 assert(db1_title_art_band_offset(512,288,0,-32)==0);
 assert(db1_title_art_band_offset(512,288,-1,-32)==511*4);
 assert(db1_title_art_band_offset(1024,576,512,224)==-1);
 assert(db1_title_art_band_offset(1672,941,0,-32)==0);
 assert(db1_title_art_band_offset(1672,941,511,255)==-1);
 db1_title_art_geometry(270,23,224);assert(db1_title_art_background_line(246)==223);
 assert(db1_title_art_background_line(-23)==-46);
 db1_title_art_geometry(224,0,224);assert(db1_title_art_background_line(223)==223);
 assert(db1_title_art_logo_nearest(0)==31);
 assert(db1_title_art_logo_nearest(0x080808)==31);
 assert(db1_title_art_logo_nearest(0x313131)==30);
 assert(db1_title_art_logo_nearest(0x575757)==29);
 /* A green unused slot must never own opaque black from the approved PNG. */
 v.cram[16]=0x0e0;v.cram[31]=0;
 assert(genesis_dac_cram_to_argb(v.cram[db1_title_art_logo_nearest(0)],0)==0xff000000);
 v.cram[31]=0xeee;assert(genesis_dac_cram_to_argb(v.cram[db1_title_art_logo_nearest(0)],0)==0xffffffff);
 assert(db1_title_art_band_offset(512,288,0,0)==-1);
 assert(db1_title_art_band_offset(512,288,0,223)==-1);
 assert(db1_title_art_band_offset(512,288,0,256)==-1);
 assert(db1_title_art_band_offset(512,288,0,-33)==-1);
 assert(db1_title_art_band_offset(512,224,0,-1)==-1);
 assert(db1_title_art_alpha(0xff123456,0xffabcdef,0)==0xff123456);
 assert(db1_title_art_alpha(0xff123456,0xffabcdef,255)==0xffabcdef);
 assert(db1_title_art_alpha(0xff000000,0xffffffff,128)==0xff808080);
 uint16_t p[64]={0};p[49]=0xeee;
 unsigned m=db1_title_art_nearest(0xffffff,p,48,16);assert(m==49);
 m=db1_title_art_nearest(0x828282,p,48,16);assert(m==49);
 /* Native white fade must turn every mapped pixel white, without synthetic
  * shadow-level ghosts. */
 for(int i=0;i<64;i++)p[i]=0xeee;
 assert(genesis_dac_cram_to_argb(p[m&63],m>>6)==0xffffffff);
 for(int i=48;i<64;i++)p[i]=0;
 m=db1_title_art_nearest(0xffffff,p,48,16);assert(genesis_dac_cram_to_argb(p[m&63],m>>6)==0xff000000);
 db1_scenes_observe(0x1c9ce);v.reg[1]=64;v.reg[2]=48;v.reg[16]=1;
 word(0xc000+2*(3*64+13),0x2102);word(0xc000+2*(17*64+31),0x2262);
 assert(db1_title_art_identified(&v));before=v;
 uint32_t row[400],copy[400];for(int i=0;i<400;i++)row[i]=0xff123456;memcpy(copy,row,sizeof row);
 /* Opaque native plane A remains authoritative, including Japanese mode. */
 memset(v.vram+32,0x11,32);
 for(int x=0;x<64;x++){word(0xc000+2*(10*64+x),1);word(0xc000+2*(22*64+x),1);}
 before=v;
 db1_title_art_row(&v,180,row,400,40,0);assert(!memcmp(copy,row,sizeof row));assert(!memcmp(&v,&before,sizeof v));
 db1_title_art_row(&v,80,row,400,40,0);assert(!memcmp(copy,row,sizeof row));
 /* SAT foreground survives a backing shift even with an arbitrary color
  * equal to neither the old nor the newly sampled background. */
 v.reg[5]=0x78;word(0xf000,88+128);word(0xf002,0);word(0xf004,2);word(0xf006,100+128);
 memset(v.vram+64,0x11,32);memcpy(row,copy,sizeof row);
 db1_title_art_geometry(270,23,224);before=v;
 db1_title_art_row(&v,88,row,400,40,0);
 for(int x=140;x<148;x++)assert(row[x]==copy[x]);
 assert(row[139]!=copy[139]);assert(!memcmp(&v,&before,sizeof v));
 db1_title_art_geometry(224,0,224);
 v.reg[12]=6;assert(!db1_title_art_identified(&v));v.reg[12]=0;
 v.reg[1]=0;assert(!db1_title_art_identified(&v));v.reg[1]=64;
 word(0xc000+2*(3*64+13),0);assert(!db1_title_art_identified(&v));
 db1_scenes_observe(0x1e024);assert(!db1_title_art_identified(&before));
 puts("db1 title art: pure sampling, alpha, live palette, scene guards passed");return 0;
}
