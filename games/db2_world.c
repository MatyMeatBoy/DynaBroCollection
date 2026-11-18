#include "db2_world.h"
#include <string.h>
static unsigned word(const uint8_t *p, unsigned a) { return ((unsigned)p[a]<<8)|p[a+1]; }
static int valid(const Db2WorldContext *c) { return c && c->ram && c->ram_size>=65536; }
static int signed_word(unsigned n) { return n>=32768?(int)n-65536:(int)n; }
int db2_world_read_camera(const Db2WorldContext *c, Db2WorldCamera *out) {
 Db2WorldCamera v; unsigned p;
 if(!valid(c)||!out)return 0;
 p=(word(c->ram,0x2a56)<<16)|word(c->ram,0x2a58);
 if(p<0xff0000 || p>0xfffffe)return 0;
 p&=65535;v.cell_x=c->ram[p];v.cell_y=c->ram[p+1];
 v.ring_x=c->ram[0x2c08];v.ring_y=c->ram[0x2c09];
 if(v.cell_x<1||v.cell_x>33||v.cell_y<1||v.cell_y>33||v.ring_x>63||v.ring_y>31)return 0;
 v.hscroll=signed_word(word(c->ram,0x2bf8));v.vscroll=signed_word(word(c->ram,0x2bfa));
 v.anchor_x=(v.ring_x*8+160+v.hscroll)&511;
 if(v.anchor_x>271)v.anchor_x-=512;
 v.anchor_y=(v.ring_y*8+32-v.vscroll)&255;
 if(v.anchor_y>143)v.anchor_y-=256;
 *out=v;return 1;
}
int db2_world_project_x(const Db2WorldCamera *c,int x){return x-(int)c->cell_x*32+c->anchor_x;}
int db2_world_project_y(const Db2WorldCamera *c,int y){return y-(int)c->cell_y*32+c->anchor_y;}
int db2_world_cell_names(const Db2WorldContext *c,unsigned x,unsigned y,uint16_t out[16]){
 static const int neighbor[4]={-80,2,80,-2};
 static const unsigned quadrant[4]={0,1,3,2};
 static const unsigned subtile[4][2]={{0,2},{2,6},{6,4},{4,0}};
 static const unsigned dest[4][2]={{0,2},{3,11},{15,13},{12,4}};
 static const uint8_t special[8]={7,13,10,5,12,3,6,9};
 unsigned a,t,id,q,j;uint16_t n[16];
 if(!valid(c)||!out||x>=40||y>=40)return 0;
 a=0x9fd0+80*y+2*x;t=word(c->ram,a);id=t&255;
 if(!id)return 0;
 if(id==20){
  unsigned seed=(0xffu+a)&65535, pair=0x037c037d;
  for(q=0;q<4;q++){
   unsigned bits=special[seed&7];seed>>=4;
   for(j=0;j<4;j++){
    n[q*4+j]=0x6075;
    if(bits&1){n[q*4+j]=(uint16_t)pair;pair=(pair<<16)|(pair>>16);}
    bits>>=1;
   }
  }
 }else{
  for(q=0;q<4;q++){
   unsigned block=word(c->ram,0x2c0a+8*(id-1)+2*q)&511;
   for(j=0;j<4;j++)n[8*(q/2)+2*(q%2)+4*(j/2)+j%2]=(uint16_t)(word(c->ram,0x340a+8*block+2*j)+0x6075);
  }
  for(q=0;q<4;q++){
   unsigned typ=(t>>(14-2*q))&3,sel,other,block;
   if(!typ)continue;
   if(typ==3){sel=((a+(a>>1))&8)>>3;other=9;}
   else{int na=(int)a+neighbor[q];if(na<0||na>65534)return 0;sel=typ-1;other=word(c->ram,(unsigned)na)&255;}
   if(!other)return 0;
   block=word(c->ram,0x2c0a+8*(other-1)+2*quadrant[q])&511;
   n[dest[q][sel]]=(uint16_t)(word(c->ram,0x340a+8*block+subtile[q][sel])+0x6075);
  }
 }
 memcpy(out,n,sizeof(n));return 1;
}
int db2_world_terrain_plane(const Db2WorldContext *c,unsigned x,unsigned y,unsigned px,unsigned py,uint8_t *out){
 uint16_t n[16];unsigned name,ix,iy,a,p;
 if(!out||!c||!c->vram||c->vram_size<65536||px>=32||py>=32||!db2_world_cell_names(c,x,y,n))return 0;
 name=n[(py/8)*4+px/8];ix=px&7;iy=py&7;
 if(name&0x0800)ix=7-ix;
 if(name&0x1000)iy=7-iy;
 a=(name&2047)*32+iy*4+ix/2;p=c->vram[a];p=(ix&1)?p&15:p>>4;
 *out=(uint8_t)(((name>>9)&0x30)|p|(p?0x40:0)|((name&0x8000)?0x80:0));return 1;
}



