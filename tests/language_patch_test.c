#include "language_patch.h"
#include <assert.h>
#include <string.h>

static void expanded(unsigned count) {
    assert(DYNA_LANGUAGE_VARIANTS==8);
    uint8_t variants[8][4]={{0}};
    DynaLanguageSpan span={1,2,{0}};
    DynaLanguageImage image={4,count,{0},&span,1};
    for(unsigned i=0;i<count;i++) {
        variants[i][1]=(uint8_t)(i+1);variants[i][2]=(uint8_t)(i*3);
        span.bytes[i]=variants[i]+1;
        image.crc[i]=dyna_language_crc(variants[i],4);
    }
    char error[100];uint8_t rom[4];
    for(unsigned source=0;source<count;source++)for(int english=0;english<2;english++) {
        memcpy(rom,variants[source],4);
        assert(dyna_language_apply(rom,4,&image,english,error,sizeof error));
        assert(!memcmp(rom,variants[english?1:0],4));
    }
    image.variants=9;memcpy(rom,variants[0],4);
    assert(!dyna_language_apply(rom,4,&image,1,error,sizeof error));
    assert(!memcmp(rom,variants[0],4));
}
int main(void) {
    expanded(5);expanded(8);
    const uint8_t jp[]={1,2}, en[]={3,4}, old[]={3,2};
    DynaLanguageSpan span={1,2,{jp,en,old}};
    const uint8_t original[]={0,1,2,0}, translated[]={0,3,4,0}, previous[]={0,3,2,0};
    DynaLanguageImage image={4,3,{0,0,0},&span,1};
    image.crc[0]=dyna_language_crc(original,4);
    image.crc[1]=dyna_language_crc(translated,4);
    image.crc[2]=dyna_language_crc(previous,4);
    char error[100]; uint8_t rom[4];
    memcpy(rom,original,4);
    assert(dyna_language_apply(rom,4,&image,1,error,sizeof error));
    assert(!memcmp(rom,translated,4));
    assert(dyna_language_apply(rom,4,&image,0,error,sizeof error));
    assert(!memcmp(rom,original,4));
    memcpy(rom,previous,4);
    assert(dyna_language_apply(rom,4,&image,1,error,sizeof error));
    assert(!memcmp(rom,translated,4));
    rom[3]=9;
    uint8_t bad[4];memcpy(bad,rom,4);
    assert(!dyna_language_apply(rom,4,&image,0,error,sizeof error));
    assert(!memcmp(rom,bad,4));
    memcpy(rom,original,4);
    assert(!dyna_language_apply(rom,3,&image,1,error,sizeof error));
    span.address=3;
    assert(!dyna_language_apply(rom,4,&image,1,error,sizeof error));
    assert(!memcmp(rom,original,4));
    span.address=1;
    span.bytes[0]=en; /* qualified CRC alone does not bypass byte guards */
    assert(!dyna_language_apply(rom,4,&image,1,error,sizeof error));
    assert(!memcmp(rom,original,4));
    span.bytes[0]=jp;image.crc[1]^=1; /* broken target metadata rolls back */
    assert(!dyna_language_apply(rom,4,&image,1,error,sizeof error));
    assert(!memcmp(rom,original,4));
    const uint8_t older[]={2,3}, older_rom[]={0,2,3,0};
    span.bytes[3]=older;image.variants=4;image.crc[1]^=1;
    image.crc[3]=dyna_language_crc(older_rom,4);
    memcpy(rom,older_rom,4);
    assert(dyna_language_apply(rom,4,&image,1,error,sizeof error));
    assert(!memcmp(rom,translated,4));
    memcpy(rom,older_rom,4);
    assert(dyna_language_apply(rom,4,&image,0,error,sizeof error));
    assert(!memcmp(rom,original,4));
    return 0;
}
