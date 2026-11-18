#include "db1_visible.h"
static int signed_word(const uint8_t *p) {
    unsigned v=((unsigned)p[0]<<8)|p[1];
    return v<32768?(int)v:(int)v-65536;
}
unsigned db1_visible_actors_rect(const Db1WorldContext *c,int left,int top,int width,int height,
                           Db1VisibleActor out[256]) {
    unsigned count=0;
    uint8_t seen[256]={0};
    if(!c || !c->ram || c->ram_size<65536 || !out || width<=left || height<=top)return 0;
    const uint8_t *r=c->ram;
    int sx=signed_word(r+0x204),sy=signed_word(r+0x206);
    for(unsigned y=0;y<40;y++)for(unsigned x=0;x<40;x++)for(unsigned slot=0;slot<2;slot++) {
        unsigned id=r[0x6f34+y*80+x*2+slot];
        if(!id || id>=254 || seen[id])continue;
        const uint8_t *a=r+0x5f34+16*id;
        int ax=16+32*((int)x-r[0x7bd4])+(int)(int8_t)a[4]+sx;
        int sort_y=8+32*((int)y-r[0x7bd5])+(int)(int8_t)a[5];
        int ay=sort_y+(int)(int8_t)a[6]+sy;
        int body_visible=ax+32>left && ax<width && ay+32>top && ay<height;
        int marker_visible=ax-sx+32>left && ax-sx<width && ay-sy+8>top && ay-sy<height;
        if(!body_visible&&!marker_visible)continue;
        seen[id]=1;
        out[count++]=(Db1VisibleActor){id,ax,ay,sort_y};
    }
    /* Exact exchange order from 8D7A..8DA6. SAT is first-wins, so consumers
     * paint in reverse order. Ties are intentionally not a stable sort. */
    for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++)if(out[i].sort_y<out[j].sort_y) {
        Db1VisibleActor swap=out[i];out[i]=out[j];out[j]=swap;
    }
    return count;
}
unsigned db1_visible_actors(const Db1WorldContext *c,int width,int height,
                           Db1VisibleActor out[256]) {
    return db1_visible_actors_rect(c,16,16,width,height,out);
}
