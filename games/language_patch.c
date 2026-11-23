#include "language_patch.h"
#include "state_crc.h"
#include <stdio.h>
#include <string.h>

uint32_t dyna_language_crc(const uint8_t *data,size_t size) {
    return ~genesis_state_crc32_update(UINT32_MAX,data,size);
}
static int fail(char *error,size_t cap,const char *message) {
    if(error&&cap)snprintf(error,cap,"%s",message);
    return 0;
}
int dyna_language_apply(uint8_t *rom,size_t size,const DynaLanguageImage *image,
                        int english,char *error,size_t cap) {
    if(error&&cap)error[0]=0;
    if(!rom||!image||size!=image->size||image->variants<2||image->variants>DYNA_LANGUAGE_VARIANTS||
       (image->count&&!image->spans))return fail(error,cap,"Invalid language image layout");
    uint32_t crc=dyna_language_crc(rom,size);
    unsigned source=image->variants,target=english?1u:0u;
    for(unsigned i=0;i<image->variants;++i)if(crc==image->crc[i]){source=i;break;}
    if(source==image->variants)return fail(error,cap,"Unrecognized language image CRC");
    size_t end=0;
    for(size_t i=0;i<image->count;++i) {
        const DynaLanguageSpan *span=&image->spans[i];
        if(!span->length||span->address<end||span->address>size||span->length>size-span->address||
           !span->bytes[source]||!span->bytes[target])return fail(error,cap,"Invalid language span");
        if(memcmp(rom+span->address,span->bytes[source],span->length))
            return fail(error,cap,"Language source byte guard failed");
        end=span->address+span->length;
    }
    /* The current image already passed both the CRC and byte guards. */
    if(source==target)return 1;
    for(size_t i=0;i<image->count;++i) {
        const DynaLanguageSpan *span=&image->spans[i];
        memcpy(rom+span->address,span->bytes[target],span->length);
    }
    if(dyna_language_crc(rom,size)!=image->crc[target]) {
        for(size_t i=0;i<image->count;++i) {
            const DynaLanguageSpan *span=&image->spans[i];
            memcpy(rom+span->address,span->bytes[source],span->length);
        }
        return fail(error,cap,"Language target CRC failed; original restored");
    }
    return 1;
}
