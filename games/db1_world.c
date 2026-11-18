#include "db1_world.h"
#include <string.h>
static int span(const uint8_t*b,size_t n,size_t p,size_t count){return b&&p<=n&&count<=n-p;}
static unsigned be16(const uint8_t*p){return ((unsigned)p[0]<<8)|p[1];}
static size_t be32(const uint8_t*p){return ((size_t)be16(p)<<16)|be16(p+2);}
static int rr(const Db1WorldContext*c,size_t p,size_t n){return c&&span(c->rom,c->rom_size,p,n);}
static int jump(size_t*p,int d){if(d<0){size_t n=(size_t)-d;if(*p<n)return 0;*p-=n;}else{if(*p>SIZE_MAX-(unsigned)d)return 0;*p+=(unsigned)d;}return 1;}
static int sx(unsigned v){return v<128?(int)v:(int)v-256;}
static int tile_pixel(const uint8_t*b,size_t n,unsigned t,unsigned x,unsigned y,uint8_t*p){
 size_t off=(size_t)t*32+y*4+x/2;if(!span(b,n,off,1))return 0;
 *p=(uint8_t)((b[off]>>(x&1?0:4))&15);return 1;
}
int db1_world_terrain_plane(const Db1WorldContext*c,unsigned cx,unsigned cy,unsigned x,unsigned y,uint8_t*p){
 size_t off;unsigned id,q,sub,tile;uint8_t color;
 if(!c||!p||cx>=40||cy>=40||x>=32||y>=32)return 0;
 off=0x7bf8+cy*40+cx;if(!span(c->ram,c->ram_size,off,1))return 0;id=c->ram[off];if(!id)return 0;
 if(id==0x80){
  /* 3F20 randomizes this decoration when the guest fills its terrain ring.
   * Reuse the actual generated pattern, without consuming guest RNG. */
  if(!span(c->ram,c->ram_size,0x7bd4,2)||!span(c->ram,c->ram_size,0x208,2))return 0;
  int dx=(int)cx-c->ram[0x7bd4],dy=(int)cy-c->ram[0x7bd5];
  if(dx< -3||dx>12||dy< -1||dy>6)return 0;
  unsigned tx=(c->ram[0x208]+4*(unsigned)(dx+3)+x/8)&63;
  unsigned ty=(c->ram[0x209]+4*(unsigned)(dy+1)+y/8)&31;
  off=0xe000+ty*128+tx*2;if(!span(c->vram,c->vram_size,off,2))return 0;
  tile=be16(c->vram+off);
 }else{
  off=0x20a+(id-1)*4+(y/16)*2+x/16;
  if(!span(c->ram,c->ram_size,off,1))return 0;
  q=c->ram[off];sub=((y%16)/8)*2+(x%16)/8;off=0x60a+q*8+sub*2;
  if(!span(c->ram,c->ram_size,off,2))return 0;
  tile=(be16(c->ram+off)+0x6052)&65535;
 }
 x%=8;y%=8;if(tile&0x800)x=7-x;if(tile&0x1000)y=7-y;
 if(!tile_pixel(c->vram,c->vram_size,tile&0x7ff,x,y,&color))return 0;
 *p=(uint8_t)(color+((tile>>13)&3)*16+(color?64:0)+((tile&0x8000)?128:0));return 1;
}
int db1_world_terrain_pixel(const Db1WorldContext*c,unsigned cx,unsigned cy,unsigned x,unsigned y,uint8_t*p){
 uint8_t plane;if(!p||!db1_world_terrain_plane(c,cx,cy,x,y,&plane))return 0;
 *p=plane&63;return 1;
}
int db1_world_decompress(const Db1WorldContext*c,size_t s,uint8_t out[512]){
 uint8_t tmp[512];size_t used=0;if(!out||!rr(c,s,3)||be16(c->rom+s)!=512)return 0;s+=3;
 while(used<512){unsigned code,count;if(!rr(c,s,1))return 0;code=c->rom[s++];count=(code&127)+1;
  if(count>512-used)return 0;
  if(code<128){if(!rr(c,s,count))return 0;memcpy(tmp+used,c->rom+s,count);s+=count;used+=count;}
  else{unsigned distance;if(!rr(c,s,1))return 0;distance=c->rom[s++]+1;if(distance>used)return 0;
   while(count--){tmp[used]=tmp[used-distance];used++;}}
 }memcpy(out,tmp,512);return 1;
}
int db1_world_actor_frame(const Db1WorldContext*c,const uint8_t record[16],unsigned actor_id,Db1WorldActorFrame*out){
 unsigned type,elapsed,loop=0;size_t entry,graphics,anim,p;Db1WorldActorFrame result;
 if(!record||!out||!record[0]||actor_id>255)return 0;
 type=record[0]-1;if(type==0x75)type=0x70;
 entry=0x70010+8*((type&15)+5*(type>>4));if(!rr(c,entry,8))return 0;
 graphics=be32(c->rom+entry);anim=be32(c->rom+entry+4);
 if(!graphics||!anim||!rr(c,graphics,2)||!rr(c,anim,2))return 0;
 p=anim+8*record[7]+(record[1]&254);if(p<anim||!rr(c,p,2))return 0;
 p=anim+be16(c->rom+p);elapsed=record[8];
 for(unsigned steps=0;steps<4096;steps++){
  unsigned op;if(!rr(c,p,1))return 0;op=c->rom[p++];
  if(op==255){int delta;unsigned count;if(!rr(c,p,2))return 0;delta=sx(c->rom[p]);count=(c->rom[p+1]-1)&255;p+=2;
   if(loop){count=(loop-1)&255;if(!count){loop=0;continue;}}loop=count;if(!jump(&p,delta))return 0;
  }else if(op==254){int delta;if(!rr(c,p,1))return 0;delta=sx(c->rom[p++]);if(!jump(&p,delta))return 0;
  }else if(op==253){if(!rr(c,p,1))return 0;p++;
  }else if(op==252){if(p==SIZE_MAX)return 0;p=(p+1)&~(size_t)1;if(!rr(c,p,4))return 0;p=be32(c->rom+p);
  }else{
   unsigned frame,duration,index;size_t stream;if(!rr(c,p,2))return 0;
   frame=(op<<8)|c->rom[p++];duration=c->rom[p++];
   if(elapsed>=duration){elapsed-=duration;continue;}index=frame&255;
   if(index>=192){size_t shared;if(!rr(c,0x70000,4))return 0;shared=be32(c->rom+0x70000);stream=shared+4*(index-192);
    if(stream<shared||!rr(c,stream,4))return 0;
    stream=be32(c->rom+stream);
   }else{stream=graphics+2*index;if(stream<graphics||!rr(c,stream,2))return 0;stream=graphics+be16(c->rom+stream);}
   if(!db1_world_decompress(c,stream,result.pattern))return 0;
   result.frame=(uint16_t)frame;result.flags=(uint16_t)(frame&0xf800);
   /* Native917A..91CA: designated life stages receive faction palettes. */
   if(record[0]>=0x10&&record[7]!=0x0a&&!(record[0]<0x30&&record[7]==6)&&!record[14])result.flags|=(actor_id&128)?0x4000:0x2000;
   *out=result;return 1;
  }
 }return 0;
}
int db1_world_actor_pixel(const Db1WorldActorFrame*f,unsigned x,unsigned y,uint8_t*p){
 uint8_t color;if(!f||!p||x>=32||y>=32)return 0;
 if(f->flags&0x800)x=31-x;
 if(f->flags&0x1000)y=31-y;
 if(!tile_pixel(f->pattern,512,(x/8)*4+y/8,x%8,y%8,&color))return 0;
 *p=(uint8_t)(color+((f->flags>>13)&3)*16);return 1;
}
