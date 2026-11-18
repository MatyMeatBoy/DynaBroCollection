#include "db1_intro_fade.h"
#include <stdio.h>
#include <stdlib.h>
static int story_active;
void db1_intro_fade_reset(void){story_active=0;}
int db1_intro_fade_active(void){return story_active;}
void db1_intro_fade_observe(unsigned pc){
 int next=story_active;
 switch(pc){
 case 0x1c7d0:next=1;break; /* native mural palette/assets begin */
 case 0x1c5a4: /* intro restart, before SEGA and CRI */
 case 0x1c9ce:case 0x1caf6:case 0x1cb0c: /* title */
 case 0x1950:next=0;break; /* gameplay dispatch */
 default:break;
 }
 if(next!=story_active){
  story_active=next;
  if(getenv("DYNA_INTRO_FADE_TRACE"))fprintf(stderr,"[DB1-INTRO-FADE] pc=%06X story=%d\n",pc,story_active);
 }
}
static uint32_t blend(uint32_t pixel,uint32_t backdrop,unsigned weight){
 /* Smoothstep with exact endpoints, evaluated on sixteen native pixels.
  * The denominator is 15^3; no floating point, filtering, or extra sampling. */
 const unsigned denominator=3375;
 uint32_t result=pixel&0xff000000u;
 for(unsigned shift=0;shift<24;shift+=8){
  unsigned a=(pixel>>shift)&255,b=(backdrop>>shift)&255;
  unsigned channel=(a*weight+b*(denominator-weight)+denominator/2)/denominator;
  result|=channel<<shift;
 }
 return result;
}
int db1_intro_fade_row(uint32_t *pixels,int width,int native_width,int line,
                       int enhanced,int story,uint32_t backdrop){
 if(!pixels||!enhanced||!story||native_width!=320||width<=native_width||line<0||line>=224)return 0;
 int left=(width-native_width)/2;
 for(unsigned x=0;x<16;x++){
  unsigned weight=x*x*(45-2*x);
  int a=left+(int)x,b=left+native_width-1-(int)x;
  pixels[a]=blend(pixels[a],backdrop,weight);
  pixels[b]=blend(pixels[b],backdrop,weight);
 }
 return 1;
}
