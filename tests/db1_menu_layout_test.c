#include "db1_menu_layout.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    uint32_t native[320],out[768],saved[320];
    for(int i=0;i<320;i++)native[i]=0xff000000u+(unsigned)i;
    memcpy(saved,native,sizeof saved);
    for(int scene=DB1_SCENE_OPTIONS;scene<=DB1_SCENE_PASSWORD;scene++) {
        int top=scene==DB1_SCENE_OPTIONS?56:72;
        for(int width=320;width<=768;width++) {
            for(int x=0;x<768;x++)out[x]=0x12345678u;
            assert(db1_menu_layout_row((Db1Scene)scene,top,native,out,width));
            int center=(width-320)/2;
            for(int x=0;x<40;x++)assert(out[x]==0x12345678u);
            for(int x=40;x<48;x++)assert(out[x]==native[x]);
            for(int x=48;x<272;x++)assert(out[center+x]==native[x]);
            for(int x=width-48;x<width-40;x++)
                assert(out[x]==native[272+x-(width-48)]);
            for(int x=width-40;x<768;x++)assert(out[x]==0x12345678u);
            assert(!memcmp(native,saved,sizeof saved));
        }
        assert(!db1_menu_layout_row((Db1Scene)scene,top-1,native,out,398));
        assert(!db1_menu_layout_row((Db1Scene)scene,200,native,out,398));
    }
    assert(!db1_menu_layout_row(DB1_SCENE_BATTLE,80,native,out,398));
    assert(!db1_menu_layout_row(DB1_SCENE_OPTIONS,80,0,out,398));
    assert(!db1_menu_layout_row(DB1_SCENE_OPTIONS,80,native,out,319));
    assert(!db1_menu_layout_row(DB1_SCENE_OPTIONS,80,native,out,769));
    memcpy(out,native,sizeof native);
    assert(db1_menu_layout_row(DB1_SCENE_OPTIONS,80,out,out,427));
    for(int x=48;x<272;x++)assert(out[x+53]==saved[x]);
    uint32_t background[768];
    for(int x=0;x<768;x++)background[x]=0xdead0000u+(unsigned)x;
    for(int width=320;width<=768;width++) {
        assert(db1_menu_layout_map_row(80,native,background,out,width));
        int extra=width-320;
        for(int x=0;x<16;x++)assert(out[x]==background[x]);
        for(int x=16;x<32;x++)assert(out[x]==native[x]);
        for(int x=32;x<192;x++)assert(out[x+extra/2]==native[x]);
        for(int x=192;x<208;x++)assert(out[x+extra]==native[x]);
        for(int x=216;x<304;x++)assert(out[x+extra]==native[x]);
        for(int x=32;x<32+extra/2;x++)assert(out[x]==background[x]);
        assert(db1_menu_layout_map_row(16,native,background,out,width));
        for(int x=32;x<width-128;x++)assert(out[x]==native[32+((x-32)&7)]);
        assert(db1_menu_layout_map_row(0,native,background,out,width));
        assert(!memcmp(out,background,(size_t)width*sizeof *out));
    }
    assert(!db1_menu_layout_map_row(224,native,background,out,398));
    memcpy(out,background,sizeof out);
    assert(db1_menu_layout_map_row(80,native,out,out,427));
    for(int x=32;x<192;x++)assert(out[x+53]==native[x]);
    for(int x=32;x<85;x++)assert(out[x]==background[x]);
    assert(!memcmp(native,saved,sizeof saved));
    puts("db1 authored submenu frame: PASS");
    return 0;
}
