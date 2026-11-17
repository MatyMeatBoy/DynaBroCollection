#include "language_patch.h"
#include <assert.h>
#include <string.h>

int main(void) {
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
    return 0;
}
