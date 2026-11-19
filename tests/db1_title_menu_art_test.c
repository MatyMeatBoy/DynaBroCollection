#include "../games/db1_title_menu_art.h"
#include "../games/db1_title_menu.h"
#include "video/genesis_dac.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static GVDP v,before;
static uint32_t out[1026],saved[1026];
static unsigned backgrounds;
static void word(unsigned a,unsigned n){v.vram[a&65535]=(unsigned char)(n>>8);v.vram[(a+1)&65535]=(unsigned char)n;}
static void pix(unsigned tile,int x,int y,unsigned nibble){unsigned a=(tile*32u+(unsigned)y*4+(unsigned)x/2)&65535;unsigned shift=(x&1)?0:4;v.vram[a]=(unsigned char)((v.vram[a]&~(15u<<shift))|(nibble<<shift));}
static uint32_t bg(const GVDP *p,int x,int y){assert(p==&v);++backgrounds;return 0xff100000u|((unsigned)x<<8)|(unsigned)y;}
static void fill(void){for(unsigned i=0;i<1026;i++)out[i]=0xff123456u;backgrounds=0;}
static void sprite(unsigned index,int x,int y,unsigned attr,unsigned link){unsigned a=0xf000+index*8;word(a,(unsigned)(y+128));word(a+2,link);word(a+4,attr);word(a+6,(unsigned)(x+128));}
static void setup(void){
 memset(&v,0,sizeof v);v.reg[1]=64;v.reg[2]=0x30;v.reg[4]=7;v.reg[5]=0x78;v.reg[12]=0x81;v.reg[13]=0x34;v.reg[16]=1;
 sprite(0,116,151,0x232c,1);sprite(1,195,151,0x2b2c,2);
 sprite(2,115,152,0x032d,3);sprite(3,194,152,0x0b2d,4);
 sprite(4,250,100,0x350,0);
 pix(0x32c,0,0,1);pix(0x32d,1,0,2);
 pix(0x30e,0,0,1);pix(0x30e,7,0,2);
 const unsigned glyphs[6]={0x30e,0x321,0x31d,0x31b,0x30a,0x31c};
 for(unsigned i=0;i<6;i++)pix(glyphs[i],0,0,i+1);
 v.cram[1]=0x00e;v.cram[2]=0x0e0;v.cram[17]=0xe00;v.cram[18]=0xeee;
 v.cram[3]=0xe00;v.cram[4]=0xeee;v.cram[5]=0x222;v.cram[6]=0x666;
 v.cram[19]=0x00e;v.cram[20]=0x0e0;v.cram[21]=0x444;v.cram[22]=0x888;
 fill();
}
int main(void){
 setup();before=v;
 assert(db1_title_menu_art_row(&v,200,out+1,320,0,0,0,bg));
 assert(out[1+136]==genesis_dac_cram_to_argb(v.cram[1],0));
 assert(out[1+143]==genesis_dac_cram_to_argb(v.cram[2],0));
 for(unsigned i=0;i<6;i++)assert(out[1+136+8*i]==genesis_dac_cram_to_argb(v.cram[i+1],0));
 assert(out[1+137]==0xff123456u&&out[0]==0xff123456u&&out[321]==0xff123456u);
 assert(!memcmp(&before,&v,sizeof v)&&!backgrounds);
 fill();assert(db1_title_menu_art_row(&v,200,out+1,320,0,DB1_TITLE_PENDING_ROW,0,bg));
 assert(out[1+136]==genesis_dac_cram_to_argb(v.cram[1],0)); /* Native target already visible. */
 fill();assert(db1_title_menu_art_row(&v,200,out+1,320,0,DB1_TITLE_PENDING_ROW+3,0,bg));
 assert(out[1+136]==genesis_dac_cram_to_argb(v.cram[17],0)); /* Still hiding the old cursor. */
 fill();assert(db1_title_menu_art_row(&v,200,out+1,427,53,4,0,bg));
 assert(out[1+53+136]==genesis_dac_cram_to_argb(v.cram[17],0));
 assert(out[1+53+143]==genesis_dac_cram_to_argb(v.cram[18],0));
 for(unsigned i=0;i<6;i++)assert(out[1+53+136+8*i]==genesis_dac_cram_to_argb(v.cram[17+i],0));
 assert(out[0]==0xff123456u&&out[428]==0xff123456u);
 assert(out[1+53+116]==genesis_dac_cram_to_argb(v.cram[2],0)); /* Native shadow palette. */
 fill();assert(db1_title_menu_art_row(&v,199,out+1,320,0,4,0,bg));
 assert(out[1+116]==genesis_dac_cram_to_argb(v.cram[17],0));
 assert(out[1+202]==genesis_dac_cram_to_argb(v.cram[17],0)); /* SAT right H flip. */
 assert(out[1+195]==0xff123456u);
 fill();assert(db1_title_menu_art_row(&v,151,out+1,320,0,4,0,bg));
 assert(out[1+116]==(0xff100000u|(116u<<8)|151u));
 assert(out[1+202]==(0xff100000u|(202u<<8)|151u));
 assert(backgrounds==2&&out[1+117]==0xff123456u); /* Never erase rectangles. */
 assert(!memcmp(&before,&v,sizeof v));
 setup();sprite(4,136,152,0x230e,0);
 out[1+136]=genesis_dac_cram_to_argb(v.cram[17],0);
 assert(db1_title_menu_art_row(&v,152,out+1,320,0,4,0,bg));
 assert(out[1+136]==genesis_dac_cram_to_argb(v.cram[1],0));
 setup();
 fill();memcpy(saved,out,sizeof out);
 assert(!db1_title_menu_art_row(&v,151,out+1,320,0,0,0,bg));assert(!memcmp(saved,out,sizeof out));
 setup();sprite(4,116,151,0x350,0);pix(0x350,0,0,4);
 assert(db1_title_menu_art_row(&v,151,out+1,320,0,4,0,bg));assert(out[1+116]==0xff123456u&&backgrounds==1);
 setup();sprite(4,136,200,0x350,0);pix(0x350,0,0,4);
 assert(db1_title_menu_art_row(&v,200,out+1,320,0,4,0,bg));assert(out[1+136]==0xff123456u);
 assert(out[1+143]==genesis_dac_cram_to_argb(v.cram[18],0));
 setup();word(0xc000+2*(25*64+17),0x360);pix(0x360,0,0,5);
 assert(db1_title_menu_art_row(&v,200,out+1,320,0,4,0,bg));assert(out[1+136]==0xff123456u); /* Opaque plane A. */
 setup();pix(0x32c,0,1,1);
 assert(db1_title_menu_art_row(&v,200,out+1,320,0,4,0,bg));assert(out[1+116]==genesis_dac_cram_to_argb(v.cram[17],0)); /* Lower SAT index wins. */
 setup();memset(v.cram,0,sizeof v.cram);
 assert(db1_title_menu_art_row(&v,200,out+1,320,0,4,0,bg));assert(out[1+136]==0xff000000u&&out[1+137]==0xff123456u);
 setup();assert(db1_title_menu_art_row(&v,200,out+1,80,-120,4,0,bg));
 assert(out[1+16]==genesis_dac_cram_to_argb(v.cram[17],0)&&out[0]==0xff123456u&&out[81]==0xff123456u);
 setup();memcpy(saved,out,sizeof out);
 assert(db1_title_menu_art_row(&v,200,out+1,80,INT_MAX,4,0,bg));assert(!memcmp(saved,out,sizeof out));
 assert(!db1_title_menu_art_row(&v,200,out+1,320,0,5,0,bg));
 assert(!db1_title_menu_art_row(&v,200,out+1,320,0,4,0,NULL));
 word(0xf000+4,0x232b);assert(!db1_title_menu_art_row(&v,200,out+1,320,0,4,0,bg));assert(!memcmp(saved,out,sizeof out));
 puts("Native title Extras glyphs, SAT cursor masks, flips, live fades, protected foreground and clipping passed");
}
