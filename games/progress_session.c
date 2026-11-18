#include "progress_session.h"
#include <string.h>
static unsigned be16(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
static uint32_t be32(const uint8_t *p){return ((uint32_t)be16(p)<<16)|be16(p+2);}
static int db1_index(const DynaProgressCode *c) {
    if(c->mode==0&&c->stage>=1&&c->stage<=8)return c->stage-1;
    if(c->mode>=1&&c->mode<=3&&c->stage>=1&&c->stage<=20)
        return c->mode*20+c->stage-1;
    if(c->mode==4&&c->stage>=1&&c->stage<=17)return 79+c->stage;
    return -1;
}
static int db2_index(unsigned mode,unsigned stage) {
    static const unsigned bases[7]={0,8,0,53,63,73,83};
    static const unsigned counts[7]={8,45,0,10,10,10,10};
    if(mode>6||!stage||stage>counts[mode])return -1;
    return (int)(bases[mode]+stage-1);
}
static int expected(const DynaProgressSession *s,const DynaProgressCode *c) {
    if(!s||!s->store||!s->rom||!c)return 0;
    if(s->store->game==1) {
        int index=db1_index(c);if(index<0||c->length!=4)return 0;
        size_t p=0x1e87e + (unsigned)index*4;
        if(p+4>s->rom_size||memcmp(s->rom+p,c->code,4))return 0;
        for(unsigned i=0;i<4;i++)if((c->code[i]>>2)>=26&&
            !(index==95&&i==3&&c->code[i]==0x6a))return 0;
        return 1;
    }
    int index=db2_index(c->mode,c->stage);if(index<0||!c->length||c->length>6)return 0;
    size_t offset=0x49f8a+(unsigned)index*2;
    if(offset+2>s->rom_size)return 0;
    size_t p=0x49f8a+be16(s->rom+offset);
    return p+c->length<s->rom_size&&!s->rom[p+c->length]&&
        !memcmp(s->rom+p,c->code,c->length);
}
int dyna_progress_session_init(DynaProgressSession *s,DynaProgressStore *store,
                              const uint8_t *rom,size_t n) {
    if(!s||!store||!rom||(store->game!=1&&store->game!=2))return 0;
    memset(s,0,sizeof *s);s->store=store;s->rom=rom;s->rom_size=n;return 1;
}
void dyna_progress_session_reset(DynaProgressSession *s) {
    if(s){s->has_current=0;memset(&s->current,0,sizeof s->current);}
}
void dyna_progress_session_before(DynaProgressSession *s,uint32_t pc,
    const uint8_t *ram,size_t n,const uint32_t a[8]) {
    if(!s||!s->store||!ram||n<65536||!a)return;
    DynaProgressCode c={0};size_t p;
    if(s->store->game==1) {
        if(pc==0x1950||pc==0x1c5a4||pc==0x1d7b6){dyna_progress_session_reset(s);return;}
        if(pc!=0x1becc&&pc!=0x1dd90&&pc!=0x28f1a)return;
        unsigned index=be16(ram+0x34);
        /* Hard-ending message shows table60 (MHRI), completed stage is59. */
        if(pc==0x28f1a){if(index!=59)return;index=60;}
        if(index>=97||(index>=8&&index<20))return;
        p=0x1e87e + index*4;
        if(pc==0x1becc&&(a[1]&0xffffffu)!=p)return;
        if(pc==0x1dd90&&(be32(ram+0x2e)!=0xffadde + index*0x4a||
                           p+4>s->rom_size||memcmp(ram+0x36,s->rom+p,4)))return;
        unsigned ordinal=index>=80?0:index<8?index+1:index%20+1;
        if(ram[0xadde + index*0x4a]!=ordinal||p+4>s->rom_size)return;
        c.mode=(uint16_t)(index>=80?4:index/20);
        c.stage=(uint16_t)(index>=80?index-79:ordinal);c.length=4;
        memcpy(c.code,s->rom+p,4);
    } else {
        if(pc==0x3a38||pc==0x6f9a||pc==0x4913e){dyna_progress_session_reset(s);return;}
        if(pc!=0x58ef2)return;
        uint32_t frame=a[6]&0xffffffu;
        if(frame<0xff0000||frame>0xfffff6||be32(ram+0x278)!=0xff0292)return;
        c.mode=ram[0x292];c.stage=(uint16_t)be16(ram+frame-0xff0000+8);
        if(c.mode!=ram[0x27c]||ram[0x293]!=ram[0x27d]||
           (c.stage!=ram[0x293]&&c.stage!=(unsigned)ram[0x293]+1))return;
        int index=db2_index(c.mode,c.stage);if(index<0)return;
        size_t table=0x49f8a+(unsigned)index*2;
        if(table+2>s->rom_size)return;
        p=0x49f8a+be16(s->rom+table);
        if((a[0]&0xffffffu)!=p||p+7>s->rom_size)return;
        while(c.length<6&&s->rom[p+c.length]){c.code[c.length]=s->rom[p+c.length];++c.length;}
    }
    c.source_pc=pc;
    if(expected(s,&c)){s->current=c;s->has_current=1;}
}
const DynaProgressCode *dyna_progress_session_current(const DynaProgressSession *s) {
    return s&&s->has_current&&expected(s,&s->current)?&s->current:NULL;
}
int dyna_progress_session_save(DynaProgressSession *s,const char *path) {
    const DynaProgressCode *c=dyna_progress_session_current(s);if(!c)return 0;
    DynaProgressStore copy=*s->store;
    if(!dyna_progress_add(&copy,c)||!dyna_progress_save(&copy,path))return 0;
    *s->store=copy;return 1;
}
unsigned dyna_progress_session_count(const DynaProgressSession *s) {
    unsigned n=0;if(!s||!s->store)return 0;
    for(unsigned i=0;i<s->store->count&&i<DYNA_PROGRESS_LIMIT;i++)
        if(expected(s,&s->store->codes[i]))++n;
    return n;
}
const DynaProgressCode *dyna_progress_session_item(const DynaProgressSession *s,unsigned index) {
    if(!s||!s->store)return NULL;
    for(unsigned i=0;i<s->store->count&&i<DYNA_PROGRESS_LIMIT;i++)
        if(expected(s,&s->store->codes[i])){if(!index)return &s->store->codes[i];--index;}
    return NULL;
}
int dyna_progress_session_text(const DynaProgressSession *s,const DynaProgressCode *c,char text[9]) {
    if(!text||!expected(s,c))return 0;
    for(unsigned i=0;i<c->length;i++)text[i]=s->store->game==1?(char)('A'+(c->code[i]>>2)):(char)c->code[i];
    /* Native Alien16 stores4 glyphs, but its last0x6A is a display blank. */
    text[s->store->game==1&&c->mode==4&&c->stage==16&&c->code[3]==0x6a?3:c->length]=0;return 1;
}
