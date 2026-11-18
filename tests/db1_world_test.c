#include "db1_world.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x71000],ram[0x10000],vram[0x10000];
static Db1WorldContext ctx={rom,sizeof rom,ram,sizeof ram,vram,sizeof vram};
static void w16(uint8_t*p,unsigned n){p[0]=(uint8_t)(n>>8);p[1]=(uint8_t)n;}
static void w32(uint8_t*p,unsigned n){w16(p,n>>16);w16(p+2,n);}
int main(void){
 uint8_t p=99,out[512],record[16]={1};Db1WorldActorFrame f;
 ram[0x7bf8]=1;ram[0x20a]=0;ram[0x20b]=1;ram[0x20c]=2;ram[0x20d]=3;
 for(unsigned q=0;q<4;q++)for(unsigned t=0;t<4;t++)w16(ram+0x60a+q*8+t*2,0xA000+q*4+t-0x6052);
 for(unsigned t=0;t<16;t++)memset(vram+t*32,(int)((t%15+1)*17),32);
 assert(db1_world_terrain_pixel(&ctx,0,0,0,0,&p)&&p==17);
 assert(db1_world_terrain_pixel(&ctx,0,0,16,0,&p)&&p==21);
 assert(db1_world_terrain_pixel(&ctx,0,0,0,16,&p)&&p==25);
 assert(db1_world_terrain_pixel(&ctx,0,0,16,16,&p)&&p==29);
 assert(!db1_world_terrain_pixel(&ctx,40,0,0,0,&p));
 assert(!db1_world_terrain_pixel(&ctx,0,1,0,0,&p));
 assert(!db1_world_terrain_pixel(&ctx,0,0,32,0,&p));
 w16(ram+0x60a,0xA800-0x6052);vram[3]=0x02;
 assert(db1_world_terrain_pixel(&ctx,0,0,0,0,&p)&&p==18);
 w16(ram+0x60a,0xB000-0x6052);vram[28]=0x30;
 assert(db1_world_terrain_pixel(&ctx,0,0,0,0,&p)&&p==19);
 /* Random terrain comes from the already generated native ring. */
 ram[0x7bf8]=0x80;w16(vram+0xe000+4*128+12*2,0xa000);
 vram[0]=0x40;
 assert(db1_world_terrain_pixel(&ctx,0,0,0,0,&p)&&p==20);
 ram[0x7bf8+13]=0x80;
 assert(!db1_world_terrain_pixel(&ctx,13,0,0,0,&p));
 ram[0x7bf8]=1;
 /* 512-byte stream: one literal then overlapping distance-one matches. */
 w16(rom+0x100,512);rom[0x103]=0;rom[0x104]=0xAB;
 unsigned s=0x105;for(unsigned i=0;i<3;i++){rom[s++]=255;rom[s++]=0;}rom[s++]=254;rom[s++]=0;
 assert(db1_world_decompress(&ctx,0x100,out));for(unsigned i=0;i<512;i++)assert(out[i]==0xAB);
 w32(rom+0x70010,0x200);w32(rom+0x70014,0x300);w16(rom+0x200,0x100);w16(rom+0x300,0x10);
 /* graphics stream relocated from fixture at100 to graphicsBase+100. */
 memcpy(rom+0x400,rom+0x100,s-0x100);w16(rom+0x200,0x200);
 rom[0x310]=8;rom[0x311]=0;rom[0x312]=5;
 assert(db1_world_actor_frame(&ctx,record,1,&f)&&f.frame==0x800&&f.flags==0x800);
 /* The parser follows relative commands and skips sound metadata. */
 rom[0x310]=253;rom[0x311]=9;rom[0x312]=254;rom[0x313]=2;
 rom[0x316]=8;rom[0x317]=0;rom[0x318]=5;
 assert(db1_world_actor_frame(&ctx,record,1,&f)&&f.frame==0x800);
 /* Absolute script indirection aligns the pointer to an even address. */
 rom[0x310]=252;w32(rom+0x312,0x316);
 assert(db1_world_actor_frame(&ctx,record,1,&f)&&f.frame==0x800);
 /* FF loop iteration consumes elapsed time, then exits to the next frame. */
 rom[0x310]=0;rom[0x311]=0;rom[0x312]=1;
 rom[0x313]=255;rom[0x314]=250;rom[0x315]=2;
 rom[0x316]=8;rom[0x317]=0;rom[0x318]=5;record[8]=2;
 assert(db1_world_actor_frame(&ctx,record,1,&f)&&f.frame==0x800);record[8]=0;
 /* Shared-frame pointer table works without loading patterns into VRAM. */
 rom[0x310]=0;rom[0x311]=192;rom[0x312]=5;
 w32(rom+0x70000,0x600);w32(rom+0x600,0x400);
 assert(db1_world_actor_frame(&ctx,record,1,&f)&&f.frame==192);
 /* Faction palette depends on ID and life-stage fields, not VRAM cache. */
 memcpy(rom+0x70038,rom+0x70010,8);record[0]=0x11;
 assert(db1_world_actor_frame(&ctx,record,1,&f)&&f.flags==0x2000);
 assert(db1_world_actor_frame(&ctx,record,128,&f)&&f.flags==0x4000);
 record[14]=1;assert(db1_world_actor_frame(&ctx,record,128,&f)&&f.flags==0);record[14]=0;record[0]=1;
 rom[0x310]=8;rom[0x311]=0;
 assert(db1_world_actor_frame(&ctx,record,1,&f));
 assert(db1_world_actor_pixel(&f,0,0,&p)&&p==11);
 memset(f.pattern,0,sizeof f.pattern);f.flags=0;f.pattern[4*32]=0xC0;
 assert(db1_world_actor_pixel(&f,8,0,&p)&&p==12);
 f.flags=0x4800;assert(db1_world_actor_pixel(&f,23,0,&p)&&p==44);
 assert(!db1_world_actor_pixel(&f,32,0,&p));
 record[0]=0;assert(!db1_world_actor_frame(&ctx,record,1,&f));record[0]=1;
 rom[0x310]=254;rom[0x311]=254;assert(!db1_world_actor_frame(&ctx,record,1,&f));
 w16(rom+0x100,513);assert(!db1_world_decompress(&ctx,0x100,out));
 w16(rom+0x100,512);rom[0x103]=255;assert(!db1_world_decompress(&ctx,0x100,out));
 assert(!db1_world_decompress(&ctx,sizeof rom-1,out));
 assert(!db1_world_actor_frame(&ctx,record,256,&f));
 w32(rom+0x70010,0xFFFFFFFE);assert(!db1_world_actor_frame(&ctx,record,1,&f));
 w32(rom+0x70010,0x200);rom[0x310]=8;rom[0x311]=0;rom[0x312]=5;
 uint8_t rom_before[sizeof rom],vram_before[sizeof vram];
 memcpy(rom_before,rom,sizeof rom);memcpy(vram_before,vram,sizeof vram);
 uint8_t before[sizeof ram];memcpy(before,ram,sizeof ram);Db1WorldContext shortctx=ctx;shortctx.ram_size=4;
 assert(!db1_world_terrain_pixel(&shortctx,0,0,0,0,&p));assert(!memcmp(before,ram,sizeof ram));
 assert(db1_world_actor_frame(&ctx,record,1,&f));
 assert(db1_world_decompress(&ctx,0x400,out));
 assert(db1_world_terrain_pixel(&ctx,0,0,0,0,&p));
 assert(!memcmp(before,ram,sizeof ram));
 assert(!memcmp(rom_before,rom,sizeof rom)&&!memcmp(vram_before,vram,sizeof vram));
 puts("db1 world: PASS");return 0;
}
