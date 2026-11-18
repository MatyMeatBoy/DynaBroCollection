#include "db2_effects.h"
static unsigned read_word(const uint8_t *p,unsigned a){return (unsigned)p[a]*256+p[a+1];}
static int signed_word(unsigned v){return v>=32768?(int)v-65536:(int)v;}
int db2_effects_is_marker(uint16_t attr){unsigned p=attr&2047;return p>=0x6ec&&p<=0x6f0;}
size_t db2_effects_collect(const Db2WorldContext *c,unsigned sat_base,Db2WorldEffect *out,size_t capacity){
 struct Owner{unsigned attr,rank;int depth;unsigned used;} owners[40];
 uint8_t gather_seen[72]={0},sat_seen[80]={0};
 unsigned node,nodes=0,owner_count=0,sat=0;size_t count=0;
 if(!c||!c->ram||c->ram_size<65536||!c->vram||c->vram_size<65536||!out||!capacity||sat_base>65536-80*8)return 0;
 if(c->ram[0x2a89]&8)return 0;
 node=read_word(c->ram,0x9d3a);
 while(node){
  unsigned index,owner;
  if(node<4||node>4+71*8||(node-4)%8)return 0;
  index=(node-4)/8;if(gather_seen[index])return 0;gather_seen[index]=1;
  owner=read_word(c->ram,0x9d3a+node+2);
  if(nodes<40&&(owner>>8)==255){
   unsigned record=0xbce6+8*(owner&255);
   owners[owner_count].attr=(0xc1f5+16*c->ram[record])&65535;
   owners[owner_count].rank=nodes;
   owners[owner_count].depth=signed_word(read_word(c->ram,0x9d3a+node+4));
   owners[owner_count].used=0;owner_count++;
  }
  nodes++;node=read_word(c->ram,0x9d3a+node);
 }
 for(unsigned walked=0;walked<80;walked++){
  unsigned a,sl,attr,next;
  if(sat>=80||sat_seen[sat])break;
  sat_seen[sat]=1;
  a=sat_base+sat*8;sl=read_word(c->vram,a+2);attr=read_word(c->vram,a+4);next=sl&127;
  if((sl>>8)==15){
   for(unsigned i=0;i<owner_count;i++)if(!owners[i].used&&owners[i].attr==attr){
    if(count==capacity)return count;
    out[count].attr=(uint16_t)attr;out[count].size=15;
    out[count].x=(int)(read_word(c->vram,a+6)&511)-128;
    out[count].y=(int)(read_word(c->vram,a)&1023)-128;
    out[count].depth=owners[i].depth;out[count].rank=owners[i].rank;
    out[count].sat_index=sat;out[count].kind=DB2_EFFECT_PSEUDO;
    owners[i].used=1;count++;break;
   }
  }
  if(!next)break;
  sat=next;
 }
 return count;
}

