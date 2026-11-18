#include "db2_effects.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[65536],vram[65536],saved_ram[65536],saved_vram[65536];
static Db2WorldContext ctx={NULL,0,ram,sizeof ram,vram,sizeof vram};
static void w(uint8_t *p,unsigned a,unsigned v){p[a]=(uint8_t)(v>>8);p[a+1]=(uint8_t)v;}
static void node(unsigned offset,unsigned next,unsigned owner,unsigned depth){w(ram,0x9d3a+offset,next);w(ram,0x9d3c+offset,owner);w(ram,0x9d3e + offset,depth);}
static void sat(unsigned index,unsigned next,unsigned attr,unsigned size,unsigned x,unsigned y){unsigned a=0xd800+index*8;w(vram,a,y);w(vram,a+2,(size<<8)|next);w(vram,a+4,attr);w(vram,a+6,x);}
int main(void){
 Db2WorldEffect out[4];size_t n;
 w(ram,0x9d3a,4);node(4,12,0x1200,100);node(12,20,0xff01,90);node(20,0,0xff02,90);
 ram[0xbce6+8]=2;ram[0xbce6+16]=2;
 sat(0,5,0x82f4,10,130,140);sat(5,2,0xc215,15,150,160);sat(2,0,0xc215,15,170,180);
 memcpy(saved_ram,ram,sizeof ram);memcpy(saved_vram,vram,sizeof vram);
 n=db2_effects_collect(&ctx,0xd800,out,4);assert(n==2);
 assert(out[0].sat_index==5&&out[0].rank==1&&out[0].depth==90&&out[0].x==22&&out[0].y==32&&out[0].kind==DB2_EFFECT_PSEUDO);
 assert(out[1].sat_index==2&&out[1].rank==2&&out[1].depth==90&&out[1].size==15);
 assert(!memcmp(saved_ram,ram,sizeof ram)&&!memcmp(saved_vram,vram,sizeof vram));
 assert(db2_effects_collect(&ctx,0xd800,out,1)==1);
 sat(2,5,0xc215,15,170,180);assert(db2_effects_collect(&ctx,0xd800,out,4)==2); /* SAT cycle */
 sat(2,127,0xc215,15,170,180);assert(db2_effects_collect(&ctx,0xd800,out,4)==2); /* link bounds */
 node(20,4,0xff02,90);assert(!db2_effects_collect(&ctx,0xd800,out,4)); /* gather cycle */
 node(20,5,0xff02,90);assert(!db2_effects_collect(&ctx,0xd800,out,4)); /* misaligned */
 node(20,0,0xff02,0xfff0);assert(db2_effects_collect(&ctx,0xd800,out,4)==2&&out[1].depth==-16);
 ram[0x2a89]=8;assert(!db2_effects_collect(&ctx,0xd800,out,4));ram[0x2a89]=0;
 assert(!db2_effects_collect(&ctx,65000,out,4));
 assert(db2_effects_is_marker(0x86ec)&&db2_effects_is_marker(0x6f0)&&!db2_effects_is_marker(0x6f1));
 memset(ram,0,sizeof ram);w(ram,0x9d3a,4);
 for(unsigned i=0;i<41;i++)node(4+8*i,i==40?0:12+8*i,i==40?0xff01:0x1200,100-i);
 ram[0xbce6+8]=2;assert(!db2_effects_collect(&ctx,0xd800,out,4)); /* native40 cap */
 w(ram,0x9d3a,580);assert(!db2_effects_collect(&ctx,0xd800,out,4)); /* gather bound */
 puts("DB2 effects tests passed");return 0;
}


