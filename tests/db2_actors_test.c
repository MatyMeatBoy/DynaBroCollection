#include "db2_actors.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned char rom[0x79000],ram[65536];
static Db2Actor visible[254];
static Db2ActorsContext c={rom,sizeof rom,ram,sizeof ram};
static void w16(unsigned char*p,unsigned x){p[0]=x>>8;p[1]=x;}
static void w32(unsigned char*p,unsigned x){w16(p,x>>16);w16(p+2,x);}
int main(void){Db2Actor a;unsigned char p=0;unsigned char *rec=ram+0x5f88;
 w32(rom+0x78810,0x1000);w32(rom+0x78814,0x1100);w32(rom+0x78804,0x1200);w32(rom+0x1000,0x1300);w32(rom+0x1100,0x1200);
 w16(rom+0x1200,0x20);w32(rom+0x1300,0x1400);w16(rom+0x1400,512);rom[0x1403]=0;rom[0x1404]=0xab;unsigned s=0x1405;for(unsigned i=0;i<3;i++){rom[s++]=255;rom[s++]=0;}rom[s++]=254;rom[s++]=0;
 w32(ram+0x2a56,0xff9c8e);rec[0]=1;rec[2]=3;rec[3]=4;rec[4]=0xfc;rec[5]=2;rec[6]=0xf0;rom[0x1220]=8;rom[0x1221]=0;rom[0x1222]=5;
 assert(db2_actor_decode(&c,1,&a));assert(a.depth==130&&a.component_count==1&&a.part[0].frame==0x800);assert(a.x==92&&a.y==118);assert(db2_actor_pixel(&a.part[0],0,0,&p));assert(p==27);
 assert(!db2_actor_decode(&c,0,&a));assert(!db2_actor_decode(&c,255,&a));rec[0]=93;assert(!db2_actor_decode(&c,1,&a));rec[0]=1;
 /* FB carries two independent frames plus displacement selector. */
 rom[0x1220]=0xfa;rom[0x1221]=3;rom[0x1222]=0xf9;rom[0x1223]=0xfe;rom[0x1224]=0xfb;rom[0x1225]=1;w16(rom+0x1226,0);w16(rom+0x1228,0x800);rom[0x122a]=5;
 assert(db2_actor_decode(&c,1,&a)&&a.component_count==2);assert(a.x==95&&a.y==116);assert(a.part[1].dx==0&&a.part[1].dy==-32);
 rom[0x1220]=0xfe;rom[0x1221]=0xfe;assert(!db2_actor_decode(&c,1,&a));rom[0x1220]=0;rom[0x1221]=0;rom[0x1222]=5;rom[0x1403]=0x80;rom[0x1404]=0;assert(!db2_actor_decode(&c,1,&a));
 /* A live cache preserves its own frame and FA/F9 offsets, including FB. */
 w32(ram+0x2a56,0xff9c8e);rom[0x1403]=0;rom[0x1404]=0xab;
 w16(ram+0x20b2,0);unsigned char *cache=ram+0x1bb2;memset(cache,0,32);w16(cache,65535);w16(cache+2,0x800);w16(cache+6,0);w16(cache+10,0x100);cache[0x14]=1;cache[0x18]=rec[7];cache[0x19]=rec[1];w32(cache+0x1a,0xff5f88);cache[0x1e]=5;cache[0x1f]=0xfd;cache[0x17]=2;
 assert(db2_actor_decode_cached(&c,1,&a)&&a.part[0].frame==0x800&&a.component_count==2);assert(a.x==97&&a.y==115&&a.part[1].dx==32);
 cache[0x19]=2;assert(!db2_actor_decode_cached(&c,1,&a));cache[0x19]=rec[1];
 w32(ram+0x2a56,0);assert(!db2_actor_decode(&c,1,&a));
 w16(ram+0x20b2,0);w16(ram+0x1bb2,65535);w16(ram+0x1bb2+4,0x20);w16(ram+0x1bb2+10,0x100);assert(db2_actor_native_sat_body(&c,0x239e));assert(!db2_actor_native_sat_body(&c,0x6ec));ram[0x1bb2+4]=255;assert(!db2_actor_native_sat_body(&c,0x239e));
 w32(ram+0x2a56,0xff9c8e);memcpy(rec+48,rec,48);w16(ram+0x9d3a,4);w16(ram+0x9d3a+4,12);ram[0x9d3a+6]=2;w16(ram+0x9d3a+12,0);ram[0x9d3a+14]=1;
 assert(db2_actors_visible(&c,1000,1000,visible)==2);assert(visible[0].id==2&&visible[1].id==1);
 for(unsigned id=3;id<=41;id++)memcpy(ram+0x5f58+48*id,rec,48);
 assert(db2_actors_visible(&c,1000,1000,visible)==41);
 puts("db2 actors: ok");return 0;}





