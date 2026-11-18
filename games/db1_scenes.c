#include "db1_scenes.h"
#include "video/genesis_dac.h"
#include <string.h>
static Db1Scene scene;
void db1_scenes_reset(void) { scene=DB1_SCENE_UNKNOWN; }
Db1Scene db1_scenes_kind(void) { return scene; }
void db1_scenes_observe(unsigned pc) {
    switch(pc) {
    case 0x1c5a4: scene=DB1_SCENE_BRANDING;break;
    case 0x1c9ce: case 0x1caf6: case 0x1cb0c:
        scene=DB1_SCENE_TITLE;break;
    case 0x1e024: scene=DB1_SCENE_OPTIONS;break;
    case 0x1d7b6: scene=DB1_SCENE_PASSWORD;break;
    case 0x1bc04: scene=DB1_SCENE_GAMEOVER;break;
    case 0x1950: scene=DB1_SCENE_UNKNOWN;break;
    case 0x1b42: scene=DB1_SCENE_STAGE_INTRO;break;
    case 0x1c40: scene=DB1_SCENE_BATTLE;break;
    case 0x1c44: scene=DB1_SCENE_UNKNOWN;break;
    }
}
static unsigned word(const GVDP *v,unsigned a) {
    return ((unsigned)v->vram[a&65535]<<8)|v->vram[(a+1)&65535];
}
static int wrap(int p,int period) { int r=p%period;return r<0?r+period:r; }
uint8_t db1_scenes_plane_b(const GVDP *v,int x,int y) {
    /* DB1 uses normal (non-interlaced) pattern addressing. Reserved geometry
     * is refused, rather than reading an invented name-table layout. */
    static const int sizes[4]={32,64,0,128};
    int wt=sizes[v->reg[16]&3],ht=sizes[(v->reg[16]>>4)&3];
    if(!wt||!ht||(v->reg[12]&6)==6)return 0;
    unsigned mode=v->reg[11]&3;
    /* Extra rows continue the nearest native raster's scroll, never read
     * beyond the guest's 224-entry raster scroll table. */
    int raster=y<0?0:y>223?223:y;
    int hi=mode==0?0:mode==2?raster&~7:raster;
    unsigned hs=word(v,((v->reg[13]&63)<<10)+(unsigned)hi*4+2)&1023;
    int vs=v->vsram[1]&1023;
    if(v->reg[11]&4) {
        int col=x/16;if(x<0&&x%16)--col;
        /* Outside native columns, use the closest authored column scroll. */
        if(col<0)col=0;
        if(col>19)col=19;
        vs=v->vsram[col*2+1]&1023;
    }
    int px=wrap(x-(int)hs,wt*8),py=wrap(y+vs,ht*8);
    unsigned base=(v->reg[4]&7)<<13;
    unsigned attr=word(v,base+2*((unsigned)(py/8)*wt+(unsigned)px/8));
    unsigned tx=(unsigned)px&7,ty=(unsigned)py&7;
    if(attr&0x800)tx=7-tx;
    if(attr&0x1000)ty=7-ty;
    unsigned bits=v->vram[((attr&2047)*32+ty*4+tx/2)&65535];
    unsigned color=(bits>>(tx&1?0:4))&15;
    return (uint8_t)(color|((attr>>9)&48)|(color?64:0)|((attr>>8)&128));
}
uint32_t db1_scenes_background_for(Db1Scene kind,const GVDP *v,int x,int y) {
    if(kind==DB1_SCENE_BRANDING)return 0xffffffffu;
    unsigned p=db1_scenes_plane_b(v,x,y);
    unsigned color=p&64?p&63:v->reg[7]&63;
    int level=(v->reg[12]&8)&&!(p&128)?GENESIS_DAC_SHADOW:GENESIS_DAC_NORMAL;
    return genesis_dac_cram_to_argb(v->cram[color],level);
}
uint32_t db1_scenes_background(const GVDP *v,int x,int y) {
    return db1_scenes_background_for(scene,v,x,y);
}
int db1_scenes_row(const GVDP *v,int line,const uint32_t *native,int nw,
                   uint32_t *out,int width) {
    if(!((scene>=DB1_SCENE_BRANDING&&scene<=DB1_SCENE_PASSWORD)||
         scene==DB1_SCENE_GAMEOVER)||nw!=320||
       width<nw||!v||!native||!out)return 0;
    int left=(width-nw)/2;
    for(int x=0;x<width;x++)out[x]=db1_scenes_background(v,x-left,line);
    memcpy(out+left,native,(size_t)nw*sizeof *out);
    return 1;
}
