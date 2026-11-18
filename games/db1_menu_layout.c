#include "db1_menu_layout.h"
#include <string.h>

int db1_menu_layout_row(Db1Scene scene, int line, const uint32_t *native,
                        uint32_t *out, int width) {
    int top;
    if(scene==DB1_SCENE_OPTIONS)top=56;
    else if(scene==DB1_SCENE_PASSWORD)top=72;
    else return 0;
    if(!native || !out || width<320 || width>768 || line<top || line>=200)
        return 0;
    /* A private row also permits native==out. The native frame has 8px caps
     * at40/272. Interior x48..55 is authored empty padding (plane B tile24
     * in options), including the cap's repeatable horizontal strokes.
     * Never sample a glyph or map cell to manufacture additional content. */
    uint32_t row[320];
    memcpy(row,native,sizeof row);
    int center=(width-320)/2;
    for(int x=40;x<width-40;x++) {
        int source;
        if(x<48)source=x;
        else if(x>=width-48)source=272+x-(width-48);
        else source=48+((x-48)&7);
        out[x]=row[source];
    }
    /* Includes all glyphs, selection cursor and dynamic values together.
     * This also preserves native identity at width320. */
    memcpy(out+center+48,row+48,224*sizeof *out);
    return 1;
}

int db1_menu_layout_map_row(int line, const uint32_t *native,
                            const uint32_t *background, uint32_t *out, int width) {
    if(!native || !background || !out || width<320 || width>768 ||
       line<0 || line>=224)return 0;
    uint32_t row[320];
    memcpy(row,native,sizeof row);
    memmove(out,background,(size_t)width*sizeof *out);
    if(line>=16 && line<208) {
        int right=width-112,extra=width-320;
        for(int x=16;x<32;x++)out[x]=row[x];
        for(int x=right-16;x<right;x++)out[x]=row[192+x-(right-16)];
        if(line<32 || line>=192) {
            /* Native frame cap repeats tile59C across the horizontal span;
             * the 16px corners retain both authored border tracks. */
            for(int x=32;x<right-16;x++)out[x]=row[32+((x-32)&7)];
        } else {
            memcpy(out+32+extra/2,row+32,160*sizeof *out);
        }
        memcpy(out+216+extra,row+216,88*sizeof *out);
    }
    return 1;
}
