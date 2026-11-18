#include "db2_markers.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 Db2WorldMarker out[6];unsigned count=db2_world_markers(0,255,100,50,out);
 assert(count==6);
 assert(out[0].attribute==0x6ee&&out[0].x==112&&out[0].y==46&&out[0].bit==0);
 assert(out[1].attribute==0x6ee&&out[1].x==112&&out[1].y==46&&out[1].bit==1);
 assert(out[5].attribute==0x6ed&&out[5].x==120&&out[5].y==46&&out[5].bit==5);
 assert(!db2_world_markers(1,255,0,0,out));
 assert(!db2_world_markers(0,0xc0,0,0,out));
 assert(db2_world_markers(0,4,-20,-5,out)==1&&out[0].x==-12&&out[0].y==-1);
 assert(db2_world_is_marker_pattern(0x6ec)&&db2_world_is_marker_pattern(0x6f0));
 assert(!db2_world_is_marker_pattern(0x6eb)&&!db2_world_is_marker_pattern(0x6f1));
 puts("DB2 marker tests passed");return 0;
}
