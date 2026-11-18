#include "db2_markers.h"
unsigned db2_world_markers(uint8_t visual_flags,uint8_t actor_flags,int x,int y,Db2WorldMarker out[6]){
 static const uint16_t attr[6]={0x06ee,0x06ee,0x06ef,0x06f0,0x06ec,0x06ed};
 static const int dx[6]={12,12,8,16,4,20},dy[6]={-4,-4,4,4,-4,-4};
 unsigned count=0;
 if(!out||(visual_flags&1))return 0;
 for(unsigned i=0;i<6;i++)if(actor_flags&(1u<<i)){
  out[count].x=x+dx[i];out[count].y=y+dy[i];
  out[count].attribute=attr[i];out[count].bit=(uint8_t)i;count++;
 }
 return count;
}
int db2_world_is_marker_pattern(unsigned pattern){return pattern>=0x6ec&&pattern<=0x6f0;}
