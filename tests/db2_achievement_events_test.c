#include "db2_achievement_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[65536];
static unsigned count; static uint32_t last;
static const uint8_t cases[][4]={
{0,1,0,51},
{0,2,0,53},
{0,3,0,52},
{0,4,0,54},
{0,5,0,55},
{0,6,0,57},
{0,7,0,58},
{0,8,255,56},
{1,1,0,1},
{1,2,0,21},
{1,3,0,22},
{1,4,0,23},
{1,5,0,2},
{1,6,0,24},
{1,7,0,3},
{1,8,0,25},
{1,9,0,4},
{1,10,0,5},
{1,11,0,26},
{1,12,0,28},
{1,13,0,7},
{1,14,0,8},
{1,15,0,33},
{1,16,0,29},
{1,17,0,34},
{1,18,0,30},
{1,19,0,11},
{1,20,0,35},
{1,21,0,27},
{1,22,0,9},
{1,23,0,36},
{1,24,0,44},
{1,25,0,16},
{1,26,0,13},
{1,27,0,42},
{1,28,0,40},
{1,29,0,39},
{1,30,0,43},
{1,31,0,14},
{1,32,0,15},
{1,33,0,12},
{1,34,0,31},
{1,35,0,32},
{1,36,0,17},
{1,37,0,6},
{1,38,0,10},
{1,39,0,18},
{1,40,0,38},
{1,41,0,45},
{1,42,0,41},
{1,43,0,19},
{1,44,0,37},
{1,45,255,20},
{3,1,0,80},
{3,2,0,81},
{3,3,0,67},
{3,4,0,66},
{3,5,0,68},
{3,6,0,70},
{3,7,0,73},
{3,8,0,76},
{3,9,0,72},
{3,10,255,71},
{4,1,0,69},
{4,2,0,82},
{4,3,0,74},
{4,4,0,85},
{4,5,0,84},
{4,6,0,78},
{4,7,0,88},
{4,8,0,86},
{4,9,0,89},
{4,10,255,112},
{5,1,0,128},
{5,2,0,114},
{5,3,0,131},
{5,4,0,113},
{5,5,0,119},
{5,6,0,120},
{5,7,0,130},
{5,8,0,127},
{5,9,0,123},
{5,10,255,124},
{6,1,0,138},
{6,2,0,140},
{6,3,0,135},
{6,4,0,141},
{6,5,0,146},
{6,6,0,144},
{6,7,0,145},
{6,8,0,149},
{6,9,0,147},
{6,10,255,150}
};
static void emit(void *user,uint32_t id) { (void)user; ++count; last=id; }
static void prepare(unsigned i) {
    memset(ram,0,sizeof(ram));count=0;last=0;
    ram[0x278]=0;ram[0x279]=255;ram[0x27a]=2;ram[0x27b]=146;
    ram[0x27c]=cases[i][0];ram[0x27d]=cases[i][1];
    memcpy(ram+0x292,cases[i],4);
}
static void step(Db2AchievementEvents *s,uint32_t pc) {
    db2_achievement_events_observe(s,pc,ram,sizeof(ram),1,emit,NULL);
}
static void finish(Db2AchievementEvents *s) {
    ram[0x9c86]=1;step(s,0x3bb4);step(s,0x3cb2);
}
static void win(Db2AchievementEvents *s) {step(s,0x3bb0);finish(s);}
int main(void) {
    Db2AchievementEvents s={0};unsigned i;
    for(i=0;i<sizeof(cases)/sizeof(cases[0]);++i) {
        prepare(i);win(&s);assert(count==1);
        assert(last==2000000u+cases[i][0]*100u+cases[i][1]);
        assert(last<2000900u); /* no final-stage-only completion awards */
        step(&s,0x3cb2);assert(count==1); /* consumed exactly once */
    }
    prepare(8);finish(&s);assert(!count); /* no gameplay arm */
    prepare(8);step(&s,0x3bb0);ram[0x9c86]=1;step(&s,0x3cb2);assert(!count);
    prepare(8);step(&s,0x3bb0);step(&s,0x3bb4);ram[0x9c86]=1;step(&s,0x3cb2);assert(!count);
    prepare(8);step(&s,0x3bb0);ram[0x9c86]=255;step(&s,0x3bb4);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);step(&s,0x3c70);finish(&s);assert(!count);
    for(i=0;i<4;++i) { /* descriptor identity/type/map corruption */
        prepare(8);step(&s,0x3bb0);ram[0x292+i]^=1;finish(&s);assert(!count);
    }
    prepare(8);ram[0x27b]++;win(&s);assert(!count);
    prepare(8);ram[0x27c]=2;win(&s);assert(!count); /* versus */
    prepare(8);ram[0x27c]=8;win(&s);assert(!count); /* demo/special */
    prepare(8);ram[0x27c]=7;win(&s);assert(!count); /* null mode */
    prepare(8);ram[0x27d]=0;win(&s);assert(!count);
    prepare(8);ram[0x27d]=46;win(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);ram[0x27d]++;finish(&s);assert(!count);
    prepare(8);ram[0x4c2]=0x55;ram[0x4c3]=0x48;ram[0x4c4]=0x59;ram[0x4c5]=0x6f;
    win(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);ram[0x4c2]=0x55;ram[0x4c3]=0x48;ram[0x4c4]=0x59;ram[0x4c5]=0x6f;
    step(&s,0x1234);memset(ram+0x4c2,0,4);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);
    db2_achievement_events_observe(&s,0x1234,ram,sizeof(ram),0,emit,NULL);
    finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);db2_achievement_events_reset(&s);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);step(&s,0x6f9a);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);step(&s,0x3a38);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);step(&s,0x3b1e);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);
    db2_achievement_events_observe(&s,0x3bb4,ram,100,1,emit,NULL);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);
    db2_achievement_events_observe(&s,0x3bb4,NULL,sizeof(ram),1,emit,NULL);finish(&s);assert(!count);
    prepare(8);step(&s,0x58918);step(&s,0x58078);step(&s,0x7342);finish(&s);assert(!count);
    prepare(8);step(&s,0x3bb0);ram[0x9c86]=1;step(&s,0x3bb4);
    db2_achievement_events_observe(&s,0x3cb2,ram,sizeof(ram),1,NULL,NULL);assert(!count&&!s.armed);
    db2_achievement_events_reset(NULL);
    db2_achievement_events_observe(NULL,0,NULL,0,0,NULL,NULL);
    puts("DB2 custom achievement contracts passed (93 stages; milestones aggregate saved goals)");
    return 0;
}
