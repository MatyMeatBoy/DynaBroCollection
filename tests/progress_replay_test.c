#include "progress_replay.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x80000],ram[65536],snapshot[65536];
static void put(unsigned p,uint32_t x) {
    ram[p]=(uint8_t)(x>>24);ram[p+1]=(uint8_t)(x>>16);ram[p+2]=(uint8_t)(x>>8);ram[p+3]=(uint8_t)x;
}
int main(void) {
    DynaProgressReplay s;uint32_t d[8]={0},a[8]={0};
    DynaProgressCode c={0};c.mode=1;c.stage=1;c.length=4;
    uint8_t code[4]={2,6,10,14};memcpy(c.code,code,4);memcpy(rom+0x1e87e + 20*4,code,4);
    assert(dyna_progress_replay_init(&s,1,rom,sizeof rom));
    assert(dyna_progress_replay_request(&s,&c));memcpy(snapshot,ram,sizeof ram);
    dyna_progress_replay_before(&s,0x1c40,ram,sizeof ram,d,a);
    assert(!memcmp(snapshot,ram,sizeof ram));assert(s.status==DYNA_REPLAY_WAIT_PASSWORD);
    ram[0x28]=3;ram[0x29]=1;ram[0x2a]=2;d[2]=7;d[3]=3;
    dyna_progress_replay_before(&s,0x1db7e,ram,sizeof ram,d,a);
    assert(s.status==DYNA_REPLAY_PREFILLED&&d[4]==4&&d[2]==0&&d[3]==0);
    assert(!memcmp(ram+0x36,code,4));
    for(unsigned frame=0;frame<9;frame++) {
        int override;unsigned buttons;dyna_progress_replay_frame(&s,&override,&buttons);
        assert(override);assert(buttons==(frame>=3&&frame<6?DYNA_REPLAY_PAD_B:0));
    }
    int override;unsigned buttons;dyna_progress_replay_frame(&s,&override,&buttons);assert(!override);
    dyna_progress_replay_before(&s,0x1dcec,ram,sizeof ram,d,a);assert(s.status==DYNA_REPLAY_SUBMITTED);
    ram[0x35]=20;put(0x2e,0xffadde + 20*0x4a);
    dyna_progress_replay_before(&s,0x1dd90,ram,sizeof ram,d,a);assert(s.status==DYNA_REPLAY_ACCEPTED);
    dyna_progress_replay_before(&s,0x1950,ram,sizeof ram,d,a);assert(s.status==DYNA_REPLAY_LOADED);
    assert(dyna_progress_replay_request(&s,&c));
    dyna_progress_replay_before(&s,0x1db88,ram,sizeof ram,d,a);
    memset(ram+0x36,0,4); /* Later guest/user edit must not be silently replaced. */
    dyna_progress_replay_before(&s,0x1dcec,ram,sizeof ram,d,a);assert(s.status==DYNA_REPLAY_FAILED);
    assert(dyna_progress_replay_request(&s,&c));
    for(unsigned i=0;i<1801;i++)dyna_progress_replay_frame(&s,&override,&buttons);
    assert(s.status==DYNA_REPLAY_FAILED&&!override);
    assert(dyna_progress_replay_request(&s,&c));dyna_progress_replay_cancel(&s);
    assert(s.status==DYNA_REPLAY_CANCELED);
    /* DB2 requires the user's native submission; it never guesses a pad map. */
    memset(rom,0,sizeof rom);memset(ram,0,sizeof ram);c.mode=1;c.stage=2;c.length=6;
    memcpy(c.code,"native",6);unsigned table=0x49f8a+9*2;
    rom[table]=2;rom[table+1]=0;memcpy(rom+0x49f8a+0x200,"native",7);
    assert(dyna_progress_replay_init(&s,2,rom,sizeof rom));assert(dyna_progress_replay_request(&s,&c));
    dyna_progress_replay_frame(&s,&override,&buttons);assert(!override);
    dyna_progress_replay_before(&s,0x49708,ram,sizeof ram,d,a);
    assert(ram[0xe45e]==6&&!memcmp(ram+0xe458,"native",6));assert(s.status==DYNA_REPLAY_SUBMITTED);
    ram[0xe463]=2;ram[0xe464]=10;
    dyna_progress_replay_before(&s,0x49908,ram,sizeof ram,d,a);assert(s.status==DYNA_REPLAY_ACCEPTED);
    d[0]=1;d[1]=2;dyna_progress_replay_before(&s,0x6f9a,ram,sizeof ram,d,a);
    assert(s.status==DYNA_REPLAY_LOADED);
    puts("Native parser replay queue, bounded pad pulse, acceptance and timeout passed");return 0;
}
