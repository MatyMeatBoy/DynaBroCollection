#include "genesis_vdp.h"
#include <assert.h>
#include <string.h>
static GVDP v;
static GVDPRasterRow row;
static void word(unsigned address,unsigned value){v.vram[address]=(uint8_t)(value>>8);v.vram[address+1]=(uint8_t)value;}
int main(void){
    gvdp_init(&v);v.reg[1]=64;v.reg[2]=0x30;v.reg[4]=7;
    v.reg[5]=0x7c;v.reg[12]=1;v.reg[13]=0x3f;
    memset(v.vram+32,0x11,32);
    /* A sparse linked list: provenance is physical SAT index, not ordinal. */
    word(0xf800,128);v.vram[0xf803]=7;word(0xf804,1);word(0xf806,144);
    word(0xf838,128);word(0xf83c,1);word(0xf83e,148);
    GVDP before=v;
    uint8_t eager[320],captured[320];
    gvdp_render_scanline(&before,0,eager);
    assert(gvdp_capture_scanline(&v,0,&row)==320);
    assert(gvdp_compose_captured(&row,captured)==320);
    assert(!memcmp(eager,captured,sizeof eager));
    assert(!memcmp(&before,&v,sizeof v));
    assert(row.sprite_owner[16]==1 && row.sprite_owner[20]==1);
    assert(row.sprite_owner[24]==8 && !row.sprite_owner[28]);
    /* A subsequent empty row cannot retain the previous provenance. */
    assert(gvdp_capture_scanline(&v,16,&row)==320);
    for(unsigned x=0;x<320;x++)assert(!row.sprite_owner[x]);
    return 0;
}
