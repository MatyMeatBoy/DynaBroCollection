#include "db1_achievement_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[65536];
static uint32_t ids[8]; static unsigned count;
static void emit(void *u,uint32_t id) { (void)u; assert(count<8); ids[count++]=id; }
static void prepare(unsigned stage) {
    uint32_t p=0xffaddeu+stage*74u;
    memset(ram,0,sizeof(ram)); count=0;
    ram[0x2e]=(uint8_t)(p>>24);ram[0x2f]=(uint8_t)(p>>16);
    ram[0x30]=(uint8_t)(p>>8);ram[0x31]=(uint8_t)p;
    ram[p-0xff0000u]=(uint8_t)(stage>=80?0:stage<8?stage+1:stage%20+1);
    ram[0x35]=(uint8_t)(stage<8?0:stage);
}
static void step(Db1AchievementEvents *s,uint32_t pc) {
    db1_achievement_events_observe(s,pc,ram,sizeof(ram),1,emit,NULL);
}
static void expire(Db1AchievementEvents *s) {
    ram[0x7bb7]=1;step(s,0xa10e);ram[0x7bb7]=0;step(s,0xa13c);
    ram[0x7bc9]=255;step(s,0xa162);
}
static void win(Db1AchievementEvents *s) {
    step(s,0x1c40);ram[0x3d]=1;step(s,0x28ae);step(s,0x1c44);
}
int main(void) {
    Db1AchievementEvents s={0}; unsigned n;
    /* Every supported descriptor, including all final-stage paths. */
    for(n=0;n<80;n++) {
        if(n==8)n=20;
        prepare(n);win(&s);
        assert(count==(n>=40&&n<60?2u:1u));
        if(n<8)assert(ids[0]==183303+n);
        if(n>=60)assert(ids[0]==183283+n-60);
        step(&s,0x1c44);assert(count==(n>=40&&n<60?2u:1u));
    }
    prepare(20);win(&s);assert(ids[0]==183311);
    prepare(40);win(&s);assert(ids[0]==183311&&ids[1]==183313);
    prepare(59);win(&s);assert(ids[0]==183264&&ids[1]==183282);
    prepare(20);ram[0x3d]=1;step(&s,0x1c44);assert(!count); /* no arm */
    step(&s,0x1c40);step(&s,0x1c44);assert(!count); /* no battle RTS */
    prepare(20);step(&s,0x1c40);ram[0x3d]=255;step(&s,0x28ae);step(&s,0x1c44);assert(!count);
    prepare(20);step(&s,0x1c40);ram[0x31]++;ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(!count);
    prepare(20);ram[0x35]=40;win(&s);assert(!count); /* mismatched stageword */
    prepare(20);ram[0xb3a6]=2;win(&s);assert(!count); /* bad ordinal */
    prepare(8);win(&s);assert(!count); /* special descriptor is not practice9 */
    prepare(80);win(&s);assert(!count);
    prepare(20);step(&s,0x1c40);
    db1_achievement_events_observe(&s,0x28ae,ram,sizeof(ram),0,emit,NULL);
    ram[0x3d]=1;step(&s,0x1c44);assert(!count);
    prepare(20);step(&s,0x1c40);
    db1_achievement_events_observe(&s,0x28ae,ram,100,1,emit,NULL);
    ram[0x3d]=1;step(&s,0x1c44);assert(!count);
    prepare(20);step(&s,0x1c40);expire(&s);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);
    assert(count==2&&ids[0]==193203&&ids[1]==183311);
    prepare(40);step(&s,0x1c40);expire(&s);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==3&&ids[0]==193203);
    prepare(60);step(&s,0x1c40);expire(&s);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==2&&ids[0]==193203);
    prepare(0);step(&s,0x1c40);expire(&s);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1&&ids[0]==183303);
    prepare(20);step(&s,0x1c40);step(&s,0xa10e);step(&s,0xa13c);
    ram[0x7bc9]=255;step(&s,0xa162);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1&&ids[0]==183311);
    prepare(20);step(&s,0x1c40);ram[0x7bc9]=255;expire(&s);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1&&ids[0]==183311);
    prepare(20);step(&s,0x1c40);expire(&s);
    ram[0x3d]=255;step(&s,0x28ae);step(&s,0x1c44);assert(!count);
    prepare(87);win(&s);assert(count==1&&ids[0]==205061);
    prepare(92);win(&s);assert(count==1&&ids[0]==193204);
    prepare(87);step(&s,0x1c40);ram[0x3d]=255;step(&s,0x28ae);step(&s,0x1c44);assert(!count);
    prepare(92);ram[0x35]=87;win(&s);assert(!count);
    prepare(20);step(&s,0x1c40);for(n=0;n<99;n++)step(&s,0x11ef4);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1&&ids[0]==183311);
    prepare(20);step(&s,0x1c40);for(n=0;n<101;n++)step(&s,0x11ef4);
    assert(s.rain_casts==100);ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);
    assert(count==2&&ids[0]==193201);
    prepare(20);step(&s,0x1c40);for(n=0;n<100;n++)step(&s,0x11ef4);
    ram[0x3d]=255;step(&s,0x28ae);step(&s,0x1c44);assert(!count);
    prepare(87);step(&s,0x1c40);for(n=0;n<100;n++)step(&s,0x11ef4);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==2&&ids[0]==193201&&ids[1]==205061);
    prepare(20);step(&s,0x1c40);for(n=0;n<100;n++)step(&s,0x11f6c); /* failed casts */
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1);
    prepare(20);step(&s,0x1c40);step(&s,0x11ff6);step(&s,0x12224);step(&s,0x12194);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==2&&ids[0]==205018);
    prepare(87);step(&s,0x1c40);step(&s,0x11ff6);step(&s,0x12224);step(&s,0x12194);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==2&&ids[0]==205018);
    prepare(20);step(&s,0x1c40);step(&s,0x11ff6);step(&s,0x12224);step(&s,0x12194);
    ram[0x3d]=255;step(&s,0x28ae);step(&s,0x1c44);assert(!count);
    for(n=0;n<3;n++) {
        static const unsigned hooks[3]={0x11ff6,0x12224,0x12194};unsigned k;
        prepare(20);step(&s,0x1c40);for(k=0;k<3;k++)if(k!=n)step(&s,hooks[k]);
        ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1&&ids[0]==183311);
    }
    prepare(20);step(&s,0x1c40);step(&s,0x12058);step(&s,0x12254);step(&s,0x121ee);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1);
    prepare(20);step(&s,0x1c40);for(n=1;n<=100;n++){ram[0x258f]=(uint8_t)n;step(&s,0xd37a);}
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==2&&ids[0]==193202);
    prepare(20);step(&s,0x1c40);ram[0x258f]=100;step(&s,0xd37a);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1&&ids[0]==183311);
    prepare(20);ram[0x258f]=100;step(&s,0x1c40);
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1);
    prepare(0);step(&s,0x1c40);for(n=1;n<=100;n++){ram[0x258f]=(uint8_t)n;step(&s,0xd37a);}
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1&&ids[0]==183303);
    prepare(20);step(&s,0x1c40);for(n=0;n<100;n++)step(&s,0xd37a); /* player removals */
    ram[0x3d]=1;step(&s,0x28ae);step(&s,0x1c44);assert(count==1);
    prepare(20);step(&s,0x1c40);step(&s,0x12224);step(&s,0x1429e);
    assert(!count);step(&s,0x14566);assert(count==1&&ids[0]==193205);
    step(&s,0x14566);assert(count==1); /* multiple animation frames */
    prepare(20);step(&s,0x1c40);step(&s,0x1429e);step(&s,0x14566);assert(!count);
    prepare(20);step(&s,0x1c40);step(&s,0x12224);step(&s,0x14070);
    step(&s,0x1429e);step(&s,0x14566);assert(!count); /* units occupied wave */
    db1_achievement_events_reset(&s);prepare(20);step(&s,0x12224);step(&s,0x1429e);step(&s,0x14566);assert(!count);
    prepare(20);step(&s,0x1c40);ram[0x5f44]=0x33;ram[0x5f53]=4;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff5f44,emit,NULL);
    ram[0x5f52]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),1,0xff5f44,emit,NULL);
    assert(count==1&&ids[0]==205017);
    prepare(20);step(&s,0x1c40);ram[0x6734]=0x43;ram[0x6743]=4;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff6734,emit,NULL);
    ram[0x6742]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),1,0xff6734,emit,NULL);assert(!count);
    prepare(87);step(&s,0x1c40);ram[0x5f44]=0x33;ram[0x5f53]=4;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff5f44,emit,NULL);
    ram[0x5f52]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),1,0xff5f44,emit,NULL);assert(!count);
    prepare(20);step(&s,0x1c40);ram[0x5f44]=0x33;ram[0x5f53]=4;ram[0x5f52]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff5f44,emit,NULL);
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),1,0xff5f44,emit,NULL);assert(!count);
    prepare(20);step(&s,0x1c40);ram[0x5f44]=0x33;ram[0x5f53]=3;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff5f44,emit,NULL);
    ram[0x5f52]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),1,0xff5f44,emit,NULL);assert(!count);
    prepare(20);step(&s,0x1c40);ram[0x5f44]=0x33;ram[0x5f53]=4;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff5f45,emit,NULL);
    ram[0x5f52]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),1,0xff5f45,emit,NULL);assert(!count);
    prepare(20);step(&s,0x1c40);ram[0x5f44]=0x33;ram[0x5f53]=4;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff5f44,emit,NULL);
    ram[0x5f52]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),1,0xff5f54,emit,NULL);assert(!count);
    prepare(20);step(&s,0x1c40);ram[0x5f44]=0x33;ram[0x5f53]=4;
    db1_achievement_events_observe_cpu(&s,0xc7a6,ram,sizeof(ram),1,0xff5f44,emit,NULL);
    ram[0x5f52]=0x4b;
    db1_achievement_events_observe_cpu(&s,0xc7b8,ram,sizeof(ram),0,0xff5f44,emit,NULL);assert(!count);
    puts("DB1 local achievement contracts passed (76 goals)");return 0;
}
