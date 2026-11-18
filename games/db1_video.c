#include "db1_video.h"
#include "db1_visible.h"
#include "genesis_runtime.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include "video/genesis_vdp_compose.h"
#include <stdio.h>
#include <string.h>
static int enabled=1,battle,map_overlay,canvas=398,adaptive;
static Db1VisibleActor actors[256];
static Db1WorldActorFrame frames[256];
static uint8_t valid[256];
static unsigned count;
static int center_supported;
static int configure(const char *mode) {
    int w,h; char tail;
    if(!strcmp(mode,"off")){enabled=0;return 1;}
    if(!strcmp(mode,"fit") || !strcmp(mode,"stage")){
        canvas=398;adaptive=!strcmp(mode,"fit");enabled=1;return 1;
    }
    if(sscanf(mode,"%d:%d%c",&w,&h,&tail)==2 && w>0 && h>0 && w<=10000 && h<=10000) {
        canvas=(int)((224LL*w+h/2)/h);
        if(canvas<320)canvas=320;
        if(canvas>768)canvas=768;
        adaptive=0;enabled=1;return 1;
    }
    return 0;
}
static int is_enabled(void){return enabled;}
static int width(int dw,int dh,int nw,int nh){
    (void)nw;
    if(adaptive && dw>0 && dh>0 && nh>0) {
        int64_t result=((int64_t)nh*dw+dh/2)/dh;
        return result<320?320:result>768?768:(int)result;
    }
    return canvas;
}
void db1_video_reset(void){battle=map_overlay=0;count=0;center_supported=0;}
void db1_video_observe(unsigned pc) {
    if(pc==0x1c40)battle=1;
    else if(pc==0x1c44)battle=0;
    else if(pc==0x19980)map_overlay=1;
    else if(pc==0x1b040)map_overlay=0;
}
static int word(const uint8_t *p){unsigned v=((unsigned)p[0]<<8)|p[1];return v<32768?(int)v:(int)v-65536;}
static int raster_enabled(const GVDP *vdp){(void)vdp;return battle&&!map_overlay;}
static int actor_owner(const GVDP *vdp,unsigned owner) {
    if(!owner)return 0;
    unsigned address=(((unsigned)vdp->reg[5]&127)<<9)+(owner-1)*8;
    unsigned tile=(((unsigned)vdp->vram[(address+4)&65535]<<8)|vdp->vram[(address+5)&65535])&2047;
    return vdp->vram[(address+2)&65535]==15 && tile>=0x2e6 && tile<=0x5d6 && !((tile-0x2e6)&15);
}
static unsigned owner_tile(const GVDP *vdp,unsigned owner) {
    unsigned address=(((unsigned)vdp->reg[5]&127)<<9)+(owner-1)*8;
    return (((unsigned)vdp->vram[(address+4)&65535]<<8)|vdp->vram[(address+5)&65535])&2047;
}
static int embedded_marker(unsigned tile) {
    return tile==0x2dd || tile==0x2de || tile==0x2e1 || tile==0x2e2 || tile==0x2e3;
}
static void marker(const GVDP *vdp,int line,int mx,int my,unsigned tile,int edge,
                   uint8_t *sprite,uint8_t *opaque,uint8_t *high,uint8_t *shadow,uint8_t *hilite) {
    if(line<my || line>=my+8)return;
    for(unsigned px=0;px<8;px++) {
        int x=mx+(int)px;
        unsigned address=tile*32+(unsigned)(line-my)*4+px/2;
        unsigned color=(vdp->vram[address&65535]>>(px&1?0:4))&15;
        if(x<16 || x>=edge || !color)continue;
        sprite[x]=(uint8_t)color;opaque[x]=1;high[x]=shadow[x]=hilite[x]=0;
    }
}
static void scanline(const GVDP *vdp,int line,int nw,uint32_t *out,int w,
                     GameVideoNativeRow native_row,void *native_ctx,const GVDPRasterRow *raster) {
    const uint32_t *native=native_row(native_ctx);
    uint32_t background=genesis_dac_cram_to_argb(vdp->cram[vdp->reg[7]&63],0);
    for(int x=0;x<w;x++)out[x]=background;
    if(nw!=320 || w<320 || w>768 || !battle || map_overlay || !raster || !raster->display) {
        int offset=(w-nw)/2;
        if(offset>=0)memcpy(out+offset,native,(size_t)nw*sizeof *out);
        return;
    }
    int extra=w-320,edge=208+extra;
    memcpy(out,native,208*sizeof *out);
    memcpy(out+edge,native+208,112*sizeof *out);
    Db1WorldContext c={g_rom,1048576,g_ram,65536,vdp->vram,sizeof vdp->vram};
    if(line==0) {
        center_supported=1;
        count=db1_visible_actors(&c,edge,208,actors);
        for(unsigned i=0;i<count;i++) {
            const uint8_t *record=g_ram+0x5f34+actors[i].id*16;
            valid[i]=(uint8_t)db1_world_actor_frame(&c,record,actors[i].id,&frames[i]);
            if(record[0]&&!valid[i])center_supported=0;
        }
    }
    if(line<16 || line>=208) {
        for(int x=208;x<edge;x++)out[x]=native[16+(x-208)%192];
        return;
    }
    uint8_t a[768]={0},b[768]={0},sprite[768]={0},opaque[768]={0},high[768]={0};
    uint8_t shadow[768]={0},hilite[768]={0},pixels[768];
    memcpy(a,raster->plane_a,208);
    memcpy(b,raster->plane_b,208);
    int sy=word(g_ram+0x206),sx=word(g_ram+0x204);
    int wy=line-16-sy+32*g_ram[0x7bd5];
    for(int x=208;x<edge;x++) {
        int wx=x-16-sx+32*g_ram[0x7bd4]; uint8_t p;
        if(wx>=0 && wy>=0 && db1_world_terrain_plane(&c,(unsigned)wx/32,(unsigned)wy/32,(unsigned)wx%32,(unsigned)wy%32,&p))
            b[x]=p;
    }
    /* Full host list, independent of the 36-entry guest drawing cache. */
    for(unsigned remaining=count;remaining;) {unsigned i=--remaining;
      if(valid[i] && line>=actors[i].y && line<actors[i].y+32) {
        for(int px=0;px<32;px++) {
            int x=actors[i].x+px;uint8_t p;
            if(x>=16 && x<edge && db1_world_actor_pixel(&frames[i],(unsigned)px,(unsigned)(line-actors[i].y),&p) && (p&15)) {
                sprite[x]=p;opaque[x]=1;high[x]=(frames[i].flags>>15)&1;
                shadow[x]=hilite[x]=0;
                if(raster->shadow_mode && p>=62) {
                    opaque[x]=0;
                    if(p==62)hilite[x]=1;else shadow[x]=1;
                }
            }
        }
      }
      /* 9114..9168 emits these within each actor's SAT group. They must
       * share its depth, so a foreground creature can cover a rear marker. */
      if(!(g_ram[0x168]&128)) {
        static const unsigned offsets[4]={0x2a,0x3c,0x2b,0x3d};
        static const unsigned tiles[4]={0x2e1,0x2e2,0x2dd,0x2de};
        static const int dx[4]={8,16,24,24};
        int mx=actors[i].x-sx,my=actors[i].y-sy;
        for(unsigned t=4;t;) {--t;
            if(g_ram[0x7bb4+offsets[t]]==actors[i].id)
                marker(vdp,line,mx+dx[t],my,tiles[t],edge,sprite,opaque,high,shadow,hilite);
        }
        if(g_ram[0x152]==actors[i].id)
            marker(vdp,line,mx,my,0x2e3,edge,sprite,opaque,high,shadow,hilite);
      }
    }
    /* Cursor, selection markers and other original sprites keep their original
     * priority. SAT entries are compacted: classify patterns, never slot count. */
    for(int x=16;x<208;x++)if(raster->sprite_owner[x] && !actor_owner(vdp,raster->sprite_owner[x]) &&
        !embedded_marker(owner_tile(vdp,raster->sprite_owner[x]))) {
        sprite[x]=raster->sprite[x];opaque[x]=raster->opaque[x];high[x]=raster->high[x];
        shadow[x]=raster->shadow_op[x];hilite[x]=raster->hilite_op[x];
    }
    gvdp_compose_span(pixels,a,b,sprite,opaque,high,shadow,hilite,edge,raster->backdrop,raster->shadow_mode);
    for(int x=16;x<edge;x++) {
        if(x<208&&!center_supported)continue;
        unsigned p=pixels[x],level=p>=128?GENESIS_DAC_HIGHLIGHT:p>=64?GENESIS_DAC_SHADOW:GENESIS_DAC_NORMAL;
        out[x]=genesis_dac_cram_to_argb(vdp->cram[p&63],(int)level);
    }
}
const GameVideo db1_video={.configure=configure,.enabled=is_enabled,.width=width,.raster_enabled=raster_enabled,.scanline_presented=scanline,.integer_scale_supported=1};
