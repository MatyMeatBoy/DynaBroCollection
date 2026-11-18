#include "progress_replay.h"
#include <string.h>
static unsigned word(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
static uint32_t longword(const uint8_t *p){return ((uint32_t)word(p)<<16)|word(p+2);}
static int index_for(unsigned game,const DynaProgressCode *c) {
    if(game==1) {
        if(c->mode==0&&c->stage>=1&&c->stage<=8)return c->stage-1;
        if(c->mode>=1&&c->mode<=3&&c->stage>=1&&c->stage<=20)return c->mode*20+c->stage-1;
        if(c->mode==4&&c->stage>=1&&c->stage<=17)return c->stage+79;
    } else {
        static const unsigned base[7]={0,8,0,53,63,73,83},count[7]={8,45,0,10,10,10,10};
        if(c->mode<=6&&c->stage>=1&&c->stage<=count[c->mode])return (int)(base[c->mode]+c->stage-1);
    }
    return -1;
}
static int valid(const DynaProgressReplay *s,const DynaProgressCode *c) {
    if(!s||!s->rom||!c)return 0;
    int index=index_for(s->game,c);if(index<0)return 0;
    size_t p;
    if(s->game==1) {
        if(c->length!=4)return 0;
        p=0x1e87e + (unsigned)index*4;
        for(unsigned i=0;i<4;i++)if((c->code[i]>>2)>=26)return 0;
        return p+4<=s->rom_size&&!memcmp(c->code,s->rom+p,4);
    }
    if(!c->length||c->length>6)return 0;
    p=0x49f8a+(unsigned)index*2;if(p+2>s->rom_size)return 0;
    p=0x49f8a+word(s->rom+p);
    return p+c->length<s->rom_size&&!s->rom[p+c->length]&&
        !memcmp(c->code,s->rom+p,c->length);
}
int dyna_progress_replay_init(DynaProgressReplay *s,unsigned game,const uint8_t *rom,size_t n) {
    if(!s||!rom||(game!=1&&game!=2))return 0;
    memset(s,0,sizeof *s);s->game=game;s->rom=rom;s->rom_size=n;return 1;
}
int dyna_progress_replay_request(DynaProgressReplay *s,const DynaProgressCode *c) {
    if(!valid(s,c))return 0;
    s->requested=*c;s->status=DYNA_REPLAY_WAIT_PASSWORD;s->elapsed=0;
    s->pulse_frame=s->confirm_button=0;s->supplied=0;return 1;
}
void dyna_progress_replay_cancel(DynaProgressReplay *s) {
    if(s){s->status=DYNA_REPLAY_CANCELED;s->confirm_button=0;}
}
static int active(const DynaProgressReplay *s) {
    return s&&s->status>=DYNA_REPLAY_WAIT_PASSWORD&&s->status<=DYNA_REPLAY_ACCEPTED;
}
void dyna_progress_replay_before(DynaProgressReplay *s,uint32_t pc,
    uint8_t *ram,size_t n,uint32_t d[8],const uint32_t a[8]) {
    if(!active(s)||!ram||n<65536||!d||!a)return;
    if(!valid(s,&s->requested)){s->status=DYNA_REPLAY_FAILED;return;}
    int index=index_for(s->game,&s->requested);
    if(s->game==1) {
        if(!s->supplied&&(pc==0x1db7e||pc==0x1db88)) {
            /* Existing native password screen owns FF0036 and D4 here.
             * 1DB7E refreshes the original display;1DB88 is its input loop. */
            if((d[4]&0xffffu)>4)return;
            unsigned button=0;
            for(unsigned i=0;i<3;i++)if(ram[0x28+i]==1){button=1u<<i;break;}
            /* A corrupt/unqualified remap must not receive a guessed press. */
            if(!button){s->status=DYNA_REPLAY_FAILED;return;}
            memcpy(ram+0x36,s->requested.code,4);d[4]=(d[4]&0xffff0000u)|4u;
            /* Choose the first ordinary letter, never an existing DELETE/
             * CANCEL cursor. With length4, accepting this cell submits before
             * the native handler writes a fifth character (1DBE2..1DBE6). */
            d[2]&=0xffff0000u;d[3]&=0xffff0000u;
            s->supplied=1;s->status=DYNA_REPLAY_PREFILLED;
            s->confirm_button=button;s->pulse_frame=0;
        }
        if(s->supplied&&pc==0x1dcec) {
            if(memcmp(ram+0x36,s->requested.code,4)){s->status=DYNA_REPLAY_FAILED;return;}
            s->status=DYNA_REPLAY_SUBMITTED;
        }
        if(s->supplied&&pc==0x1dd1a)s->status=DYNA_REPLAY_FAILED;
        if(s->supplied&&pc==0x1dd90) {
            if(word(ram+0x34)!=(unsigned)index||
               longword(ram+0x2e)!=0xffadde + (unsigned)index*0x4a)
                s->status=DYNA_REPLAY_FAILED;
            else s->status=DYNA_REPLAY_ACCEPTED;
        }
        if(s->status==DYNA_REPLAY_ACCEPTED&&pc==0x1950)
            s->status=word(ram+0x34)==(unsigned)index&&
                longword(ram+0x2e)==0xffadde + (unsigned)index*0x4a?
                DYNA_REPLAY_LOADED:DYNA_REPLAY_FAILED;
        if(s->supplied&&(pc==0x1dce4||pc==0x1c5a4))dyna_progress_replay_cancel(s);
    } else {
        /* Parser49708 is called only by the original password input screen.
         * It immediately compares FFE458[FFE45E] against native strings. */
        if(!s->supplied&&pc==0x49708) {
            memset(ram+0xe458,0,6);memcpy(ram+0xe458,s->requested.code,s->requested.length);
            ram[0xe45e]=s->requested.length;s->supplied=1;s->status=DYNA_REPLAY_SUBMITTED;
        }
        if(s->supplied&&pc==0x49926)s->status=DYNA_REPLAY_FAILED;
        if(s->supplied&&pc==0x49908) {
            if(ram[0xe463]!=2||ram[0xe464]!=(unsigned)index+1)
                s->status=DYNA_REPLAY_FAILED;
            else s->status=DYNA_REPLAY_ACCEPTED;
        }
        if(s->status==DYNA_REPLAY_ACCEPTED&&pc==0x6f9a) {
            s->status=((d[0]&255)==s->requested.mode&&(d[1]&255)==s->requested.stage)?
                DYNA_REPLAY_LOADED:DYNA_REPLAY_FAILED;
        }
        if(s->supplied&&pc==0x3a38)dyna_progress_replay_cancel(s);
    }
}
void dyna_progress_replay_frame(DynaProgressReplay *s,int *override,unsigned *buttons) {
    if(override)*override=0;
    if(buttons)*buttons=0;
    if(!active(s))return;
    if(++s->elapsed>1800){s->status=DYNA_REPLAY_FAILED;s->confirm_button=0;return;}
    if(s->game!=1||!s->supplied||!s->confirm_button)return;
    if(s->pulse_frame<9) {
        if(override)*override=1;
        if(buttons&&s->pulse_frame>=3&&s->pulse_frame<6)*buttons=s->confirm_button;
        ++s->pulse_frame;
    }
}
const char *dyna_progress_replay_status_text(const DynaProgressReplay *s) {
    if(!s)return "No level selected";
    switch(s->status) {
    case DYNA_REPLAY_WAIT_PASSWORD:return s->game==1?"Open the native Password screen":"Open Password and submit to load the selected level";
    case DYNA_REPLAY_PREFILLED:return "Confirming the saved password";
    case DYNA_REPLAY_SUBMITTED:return "Validating the native password";
    case DYNA_REPLAY_ACCEPTED:return "Password accepted; loading level";
    case DYNA_REPLAY_LOADED:return "Native level loading started";
    case DYNA_REPLAY_FAILED:return "Level loading stopped; use the native Password screen";
    case DYNA_REPLAY_CANCELED:return "Level loading canceled";
    default:return "No level selected";
    }
}
