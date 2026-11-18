#include "progress_session.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t rom[0x80000],ram[65536];
static const char hash[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
int main(void) {
    DynaProgressStore store;DynaProgressSession s;uint32_t a[8]={0};char text[9];
    assert(dyna_progress_init(&store,1,hash));
    assert(dyna_progress_session_init(&s,&store,rom,sizeof rom));
    ram[0x35]=20;ram[0xadde + 20*0x4a]=1;a[1]=0x1e87e + 20*4;
    uint8_t code[4]={2,6,10,14};memcpy(rom+a[1],code,4);
    dyna_progress_session_before(&s,0x1becc,ram,sizeof ram,a);
    const DynaProgressCode *c=dyna_progress_session_current(&s);
    assert(c&&c->mode==1&&c->stage==1);assert(dyna_progress_session_text(&s,c,text));
    assert(!strcmp(text,"ABCD"));assert(dyna_progress_add(&store,c));
    assert(dyna_progress_session_count(&s)==1);assert(dyna_progress_session_item(&s,0));
    rom[a[1]]=0xff;assert(dyna_progress_session_count(&s)==0);
    dyna_progress_session_before(&s,0x1950,ram,sizeof ram,a);
    assert(!dyna_progress_session_current(&s));
    ram[0x35]=59;ram[0xadde + 60*0x4a]=1;
    const uint8_t secret[4]={0x32,0x1e,0x46,0x22};memcpy(rom+0x1e87e + 60*4,secret,4);
    dyna_progress_session_before(&s,0x28f1a,ram,sizeof ram,a);
    c=dyna_progress_session_current(&s);assert(c&&c->mode==3&&c->stage==1);
    assert(dyna_progress_session_text(&s,c,text));assert(!strcmp(text,"MHRI"));
    assert(dyna_progress_init(&store,2,hash));memset(ram,0,sizeof ram);memset(rom,0,sizeof rom);
    assert(dyna_progress_session_init(&s,&store,rom,sizeof rom));
    /* Mode1 stage2 is password table index9. Native stack target is next stage. */
    unsigned table=0x49f8a+9*2;rom[table]=2;rom[table+1]=0;
    unsigned p=0x49f8a+0x200;memcpy(rom+p,"native",7);a[0]=p;a[6]=0xff0100;
    ram[0x278]=0;ram[0x279]=0xff;ram[0x27a]=2;ram[0x27b]=0x92;
    ram[0x27c]=ram[0x292]=1;ram[0x27d]=ram[0x293]=1;ram[0x109]=2;
    dyna_progress_session_before(&s,0x58ef2,ram,sizeof ram,a);
    c=dyna_progress_session_current(&s);assert(c&&c->mode==1&&c->stage==2);
    assert(dyna_progress_session_text(&s,c,text));assert(!strcmp(text,"native"));
    dyna_progress_session_reset(&s);a[0]++;
    dyna_progress_session_before(&s,0x58ef2,ram,sizeof ram,a);assert(!dyna_progress_session_current(&s));
    puts("Native displayed-code capture, ROM revalidation and level enumeration passed");return 0;
}
