#include "db1_original_title.h"
#include <assert.h>
#include <stddef.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
static int ready=1,geometry_calls,row_calls;
_Alignas(uint32_t) static unsigned char buffer[(320*4+28)*224],before[sizeof buffer];
static GVDP vdp,snapshot;
int db1_title_art_logo_ready(void){return ready;}
int db1_title_art_identified(const GVDP *v){return v && v->reg[0]==0x42;}
void db1_title_art_geometry(int h,int top,int nh){assert(h==224&&top==0&&nh==224);geometry_calls++;}
void db1_title_art_row(const GVDP *v,int line,uint32_t *out,int width,int left,int english){
 assert(v==&vdp&&line>=24&&line<144&&width==320&&left==0&&english==1);
 /* Deliberately modify the entire row; adapter must copy back only the ROI. */
 for(int x=0;x<320;x++)out[x]=0xff000000u|((unsigned)line<<12)|(unsigned)x;
 row_calls++;
}
static void fill(void){memset(buffer,0x5a,sizeof buffer);memcpy(before,buffer,sizeof buffer);}
int main(void){
 vdp.reg[0]=0x42;snapshot=vdp;
 assert(db1_original_title_active(&vdp,1,0));
 assert(!db1_original_title_active(NULL,1,0));
 assert(!db1_original_title_active(&vdp,0,0));assert(!db1_original_title_active(&vdp,1,1));
 ready=0;assert(!db1_original_title_active(&vdp,1,0));ready=1;
 vdp.reg[0]=0;assert(!db1_original_title_active(&vdp,1,0));vdp=snapshot;
 for(int pitch=1280;pitch<=1308;pitch+=28){
  fill();geometry_calls=row_calls=0;
  assert(db1_original_title_render(&vdp,(uint32_t*)buffer,320,224,pitch,1,0));
  assert(geometry_calls==1&&row_calls==120);assert(!memcmp(&vdp,&snapshot,sizeof vdp));
  for(int y=0;y<224;y++)for(int byte=0;byte<pitch;byte++){
   int changed=y>=24&&y<144&&byte>=48*4&&byte<272*4;
   if(!changed)assert(buffer[(size_t)y*pitch+byte]==before[(size_t)y*pitch+byte]);
  }
  for(int y=24;y<144;y++)for(int x=48;x<272;x++){
   uint32_t pixel;memcpy(&pixel,buffer+(size_t)y*pitch+x*4,4);
   assert(pixel==(0xff000000u|((unsigned)y<<12)|(unsigned)x));
  }
 }
 const int invalid[][5]={{320,224,INT_MAX-3,1,0},{319,224,1280,1,0},{321,224,1284,1,0},{320,223,1280,1,0},{320,225,1280,1,0},{320,224,-1,1,0},{320,224,1276,1,0},{320,224,1281,1,0},{320,224,1280,0,0},{320,224,1280,1,1}};
 for(unsigned i=0;i<sizeof invalid/sizeof *invalid;i++){
  fill();const int*c=invalid[i];geometry_calls=row_calls=0;
  assert(!db1_original_title_render(&vdp,(uint32_t*)buffer,c[0],c[1],c[2],c[3],c[4]));
  assert(!memcmp(buffer,before,sizeof buffer)&&!geometry_calls&&!row_calls);
 }
 fill();ready=0;
 assert(!db1_original_title_render(&vdp,(uint32_t*)buffer,320,224,1280,1,0));assert(!memcmp(buffer,before,sizeof buffer));ready=1;
 vdp.reg[0]=0;assert(!db1_original_title_render(&vdp,(uint32_t*)buffer,320,224,1280,1,0));assert(!memcmp(buffer,before,sizeof buffer));vdp=snapshot;
 assert(!db1_original_title_render(&vdp,NULL,320,224,1280,1,0));
 assert(!db1_original_title_render(NULL,(uint32_t*)buffer,320,224,1280,1,0));
 assert(!memcmp(&vdp,&snapshot,sizeof vdp));
 puts("DB1 original English title ROI, fallback, stride and immutable VDP contracts passed");return 0;
}
