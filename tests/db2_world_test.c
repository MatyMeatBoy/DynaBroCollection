#include "db2_world.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[65536],vram[65536];
static Db2WorldContext ctx={NULL,0,ram,sizeof ram,vram,sizeof vram};
static void w(unsigned a,unsigned v){ram[a]=(uint8_t)(v>>8);ram[a+1]=(uint8_t)v;}
static unsigned rw(const uint8_t *p,unsigned a){return (unsigned)p[a]*256+p[a+1];}
int main(int argc,char **argv){
 uint16_t names[16];Db2WorldCamera c;uint8_t p;
 w(0x9fd0,1);
 for(unsigned q=0;q<4;q++){
  w(0x2c0a+q*2,q);
  for(unsigned j=0;j<4;j++)w(0x340a+q*8+j*2,0x8000+q*4+j-0x6075);
 }
 assert(db2_world_cell_names(&ctx,0,0,names));
 assert(names[0]==0x8000&&names[3]==0x8005&&names[15]==0x800f);
 memset(vram,0x11,sizeof(vram));
 assert(db2_world_terrain_plane(&ctx,0,0,0,0,&p)&&p==0xc1);
 assert(!db2_world_cell_names(&ctx,40,0,names));
 w(0x9fd0,20); assert(db2_world_cell_names(&ctx,0,0,names));
 assert(names[0]==0x037d&&names[1]==0x6075&&names[3]==0x037c);
 assert(names[4]==0x6075&&names[6]==0x037d&&names[7]==0x037c);
 w(0x2a56,0xff);w(0x2a58,0x9c8e);ram[0x9c8e]=15;ram[0x9c8f]=25;
 ram[0x2c08]=20;ram[0x2c09]=16;w(0x2bf8,208);w(0x2bfa,0xff90);
 assert(db2_world_read_camera(&ctx,&c));assert(c.anchor_x==16&&c.anchor_y==16);
 assert(db2_world_project_x(&c,15*32)==16&&db2_world_project_y(&c,25*32)==16);
 if(argc==3){
  FILE *f=fopen(argv[1],"rb");assert(f);assert(fread(ram,1,65536,f)==65536);fclose(f);
  f=fopen(argv[2],"rb");assert(f);assert(fread(vram,1,65536,f)==65536);fclose(f);
  assert(db2_world_read_camera(&ctx,&c));unsigned count=0;
  for(unsigned y=c.cell_y-1;y<c.cell_y+7;y++)for(unsigned x=c.cell_x-5;x<c.cell_x+11;x++){
   assert(db2_world_cell_names(&ctx,x,y,names));
   for(unsigned iy=0;iy<4;iy++)for(unsigned ix=0;ix<4;ix++){
    unsigned tx=(c.ring_x+4*(x-c.cell_x+5)+ix)&63,ty=(c.ring_y+4*(y-c.cell_y+1)+iy)&31;
    assert(names[iy*4+ix]==rw(vram,0xc000+128*ty+2*tx));count++;
   }
  }
  printf("DB2 resident terrain matched %u name words\n",count);
 }
 puts("DB2 world tests passed");return 0;
}

