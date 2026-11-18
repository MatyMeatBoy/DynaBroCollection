#include "db1_visor.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
int main(void){
 const int widths[]={320,398,427,480,768};
 const int requests[]={INT_MIN,-99,-2,-1,0,1,2,99,INT_MAX};
 for(unsigned i=0;i<5;i++)for(unsigned j=0;j<9;j++){
  Db1VisorLayout a,base;assert(db1_visor_layout(widths[i],requests[j],&a));assert(db1_visor_layout(widths[i],0,&base));
  assert(a.frame.w==208&&a.frame.h==208&&a.frame.y==8);
  assert(a.interior.w==192&&a.interior.h==192&&a.interior.y==16);
  assert(a.hud_left==widths[i]-104&&a.hud_shift==widths[i]-320);
  assert(a.frame.x>=8&&a.frame.x+a.frame.w<=a.hud_left);
  assert(a.frame.x==base.frame.x+8*a.offset_tiles);
  assert(a.offset_tiles>=a.min_tiles&&a.offset_tiles<=a.max_tiles);
  assert(a.interior.x==16+a.world_shift_x);
  assert(db1_visor_canvas_x(&a,16)==a.interior.x);
  for(int x=-64;x<widths[i]+64;x+=7)assert(db1_visor_native_x(&a,db1_visor_canvas_x(&a,x))==x);
  if(j==4&&i){int left=a.frame.x+6,right=a.hud_left-a.frame.x-201;assert(right-left>=0&&right-left<=1);}
 }
 Db1VisorLayout a;assert(db1_visor_layout(427,0,&a));assert(a.frame.x==58&&a.world_shift_x==50&&a.min_tiles==-6&&a.max_tiles==7);
 assert(db1_visor_layout(427,-2,&a)&&a.frame.x==42);assert(db1_visor_layout(427,2,&a)&&a.frame.x==74);
 assert(db1_visor_layout(427,INT_MIN,&a)&&a.frame.x==10);assert(db1_visor_layout(427,INT_MAX,&a)&&a.frame.x==114);
 assert(db1_visor_layout(320,99,&a)&&a.frame.x==8&&a.world_shift_x==0&&a.offset_tiles==0);
 assert(!db1_visor_layout(319,0,&a));assert(!db1_visor_layout(769,0,&a));assert(!db1_visor_layout(427,0,NULL));
 puts("db1 visor geometry:45 width/offset cases plus signed tile clamps and inverse projection passed");return 0;
}
