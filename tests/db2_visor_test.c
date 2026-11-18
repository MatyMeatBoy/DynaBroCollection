#include "../games/db2_visor.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
int main(void){
 const int widths[]={320,398,427,480,768};
 const int requests[]={INT_MIN,-99,-2,-1,0,1,2,99,INT_MAX};
 for(unsigned i=0;i<5;i++)for(unsigned j=0;j<9;j++){
  Db2VisorLayout a,base;assert(db2_visor_layout(widths[i],requests[j],&a));
  assert(db2_visor_layout(widths[i],0,&base));
  assert(a.frame.w==192&&a.frame.h==192&&a.frame.y==16);
  assert(a.hud_left==widths[i]-104&&a.hud_shift==widths[i]-320);
  assert(a.frame.x>=8&&a.hud_left-a.frame.x-a.frame.w>=8);
  assert(a.frame.x==base.frame.x+8*a.offset_tiles);
  assert(a.offset_tiles>=a.min_tiles&&a.offset_tiles<=a.max_tiles);
  assert(a.world_shift_x==a.frame.x-16);
  assert(db2_visor_canvas_x(&a,16)==a.frame.x);
  assert(db2_visor_canvas_x(&a,208)==a.frame.x+192);
  for(int x=-1024;x<2048;x+=7)assert(db2_visor_native_x(&a,db2_visor_canvas_x(&a,x))==x);
  if(j==4){int delta=a.frame.x-(a.hud_left-a.frame.x-192);assert(delta>=0&&delta<=1);}
  if(j==0){assert(a.offset_tiles==a.min_tiles);assert(a.frame.x-8<8);}
  if(j==8){assert(a.offset_tiles==a.max_tiles);assert(a.hud_left-(a.frame.x+8)-192<8);}
 }
 Db2VisorLayout a;assert(db2_visor_layout(427,0,&a));
 assert(a.frame.x==66&&a.world_shift_x==50&&a.min_tiles==-7&&a.max_tiles==7);
 assert(db2_visor_layout(427,-2,&a)&&a.frame.x==50);
 assert(db2_visor_layout(427,2,&a)&&a.frame.x==82);
 assert(db2_visor_layout(320,INT_MAX,&a)&&a.frame.x==12&&a.offset_tiles==0);
 assert(db2_visor_canvas_x(&a,INT_MIN)==INT_MIN);
 assert(db2_visor_native_x(&a,INT_MAX)==INT_MAX);
 assert(db2_visor_layout(768,0,&a));
 assert(db2_visor_canvas_x(&a,INT_MAX)==INT_MAX);
 assert(db2_visor_native_x(&a,INT_MIN)==INT_MIN);
 assert(db2_visor_canvas_x(NULL,INT_MIN)==INT_MIN&&db2_visor_native_x(NULL,INT_MAX)==INT_MAX);
 Db2VisorLayout before;memcpy(&before,&a,sizeof a);
 const int invalid[]={INT_MIN,-1,0,319,769,INT_MAX};
 for(unsigned i=0;i<6;i++){assert(!db2_visor_layout(invalid[i],0,&a));assert(!memcmp(&a,&before,sizeof a));}
 assert(!db2_visor_layout(427,0,NULL));
 puts("DB2 visor:45 balanced width/offset cases, native192 frame, clamps, common projection and transactional rejection passed");
 return 0;
}
