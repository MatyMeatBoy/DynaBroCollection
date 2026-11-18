#include "db1_intro_fade.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t pixels[770],before[770];
static void fill(void){for(unsigned i=0;i<770;i++)pixels[i]=0xfff08040u;memcpy(before,pixels,sizeof pixels);}
int main(void){
 db1_intro_fade_reset();assert(!db1_intro_fade_active());
 db1_intro_fade_observe(0x1c5a4);assert(!db1_intro_fade_active());
 db1_intro_fade_observe(0x1c7d0);assert(db1_intro_fade_active());
 db1_intro_fade_observe(0x1c8ee);assert(db1_intro_fade_active());
 db1_intro_fade_observe(0x1c9ce);assert(!db1_intro_fade_active());
 const unsigned exits[]={0x1caf6,0x1cb0c,0x1950,0x1c5a4};
 for(unsigned i=0;i<sizeof exits/sizeof *exits;i++){
  db1_intro_fade_observe(0x1c7d0);db1_intro_fade_observe(exits[i]);assert(!db1_intro_fade_active());
 }
 for(int width=398;width<=480;width+=41)for(int line=0;line<224;line++){
  fill();int left=(width-320)/2;
  assert(db1_intro_fade_row(pixels+1,width,320,line,1,1,0xff000000));
  assert(pixels[0]==before[0]&&pixels[width+1]==before[width+1]);
  for(int x=0;x<width;x++){
   int native_x=x-left;uint32_t p=pixels[x+1];
   if(native_x<0||native_x>=320||(native_x>=16&&native_x<304))assert(p==before[x+1]);
  }
  assert(pixels[left+1]==0xff000000&&pixels[left+320]==0xff000000);
  assert(pixels[left+16]==0xfff08040&&pixels[left+305]==0xfff08040);
  for(int x=0;x<16;x++){
   assert(pixels[left+x+1]==pixels[left+320-x]);
   if(x)assert((pixels[left+x+1]&255)>=(pixels[left+x]&255));
  }
 }
 /* Fade target comes from the live authored palette; channels blend toward
  * colored/gray backdrop too, rather than assuming black or multiplying RGB. */
 fill();assert(db1_intro_fade_row(pixels,398,320,200,1,1,0xff80a0e0));
 assert(pixels[39]==0xff80a0e0&&pixels[358]==0xff80a0e0);
 assert(pixels[54]==0xfff08040&&pixels[343]==0xfff08040);
 /* Logo/4:3/native-only/invalid rows remain byte-identical. */
 const int cases[][5]={{398,320,0,1,0},{398,320,0,0,1},{320,320,0,1,1},{319,320,0,1,1},{398,256,0,1,1},{398,320,-1,1,1},{398,320,224,1,1}};
 for(unsigned i=0;i<sizeof cases/sizeof *cases;i++){
  fill();const int *c=cases[i];assert(!db1_intro_fade_row(pixels,c[0],c[1],c[2],c[3],c[4],0xff000000));assert(!memcmp(pixels,before,sizeof pixels));
 }
 assert(!db1_intro_fade_row(NULL,398,320,0,1,1,0));
 puts("DB1 story-only full-height side fade contracts passed");return 0;
}
