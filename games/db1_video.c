#include "db1_video.h"
#include "db1_visible.h"
#include "db1_scenes.h"
#include "viewport.h"
#include "genesis_runtime.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include "video/genesis_vdp_compose.h"
#include <stdio.h>
#include <string.h>
static int battle,map_overlay,spell_effect,frame_spell,fullplane_effect,frame_fullplane;
static DynaViewport viewport=DYNA_VIEWPORT_DEFAULT;
static Db1VisibleActor actors[256];
static Db1WorldActorFrame frames[256];
static uint8_t valid[256];
static unsigned count;
static int center_supported;
static int canvas_height=224,canvas_top;
static int frame_world;
static uint8_t frame_ram[65536];
static GVDP frame_vdp;
static uint32_t frame_background;
static Db1Scene frame_scene;
/* Canvas callbacks use owned bytes only. Scanline providers never escape. */
static int canvas_geometry(int dw,int dh,int nw,int nh,int *w,int *h) {
    if(!viewport.enabled || nw!=320 || nh!=224)return 0;
    int base=dyna_viewport_width(&viewport,dw,dh,nh);
    int scale=game_video_integer_canvas(dw,dh,base,nh,w,h);
    if(!scale || *w>768 || *h>480)return 0;
    canvas_height=*h;canvas_top=(*h-nh)/2;return scale;
}
static int configure(const char *mode) {
    return dyna_viewport_configure(&viewport,mode);
}
static int is_enabled(void){return viewport.enabled;}
static int width(int dw,int dh,int nw,int nh){
    (void)nw;
    return dyna_viewport_width(&viewport,dw,dh,nh);
}
void db1_video_reset(void){battle=map_overlay=spell_effect=fullplane_effect=0;count=0;center_supported=0;frame_world=0;canvas_height=224;canvas_top=0;db1_scenes_reset();}
void db1_video_observe(unsigned pc) {
    db1_scenes_observe(pc);
    if(pc==0x1c40)battle=1;
    else if(pc==0x1c44)battle=0;
    else if(pc==0x19980)map_overlay=1;
    else if(pc==0x1b040)map_overlay=0;
    else if(pc==0x1b42)map_overlay=0;
    else if(pc==0x5ff6)spell_effect=1;
    else if(pc==0x62fc)spell_effect=0;
    else if(pc==0x132d8)fullplane_effect=1;
    else if(pc==0x13630)fullplane_effect=0;
}
static int word(const uint8_t *p){unsigned v=((unsigned)p[0]<<8)|p[1];return v<32768?(int)v:(int)v-65536;}
static int world_active(void){return (battle || db1_scenes_kind()==DB1_SCENE_STAGE_INTRO)&&!map_overlay;}
static int raster_enabled(const GVDP *vdp){(void)vdp;return world_active();}
static int actor_owner(const GVDP *vdp,unsigned owner) {
    if(!owner)return 0;
    unsigned address=(((unsigned)vdp->reg[5]&127)<<9)+(owner-1)*8;
    unsigned tile=(((unsigned)vdp->vram[(address+4)&65535]<<8)|vdp->vram[(address+5)&65535])&2047;
    /* 1B70..1BF6 temporarily uses the last cache tile for the first egg's
     * falling spawn animation, before ordinary actor rendering is initialized. */
    if(db1_scenes_kind()==DB1_SCENE_STAGE_INTRO && tile==0x5d6)return 0;
    return vdp->vram[(address+2)&65535]==15 && tile>=0x2e6 && tile<=0x5d6 && !((tile-0x2e6)&15);
}
static unsigned owner_tile(const GVDP *vdp,unsigned owner) {
    unsigned address=(((unsigned)vdp->reg[5]&127)<<9)+(owner-1)*8;
    return (((unsigned)vdp->vram[(address+4)&65535]<<8)|vdp->vram[(address+5)&65535])&2047;
}
static int embedded_marker(unsigned tile) {
    return tile==0x2dd || tile==0x2de || tile==0x2e1 || tile==0x2e2 || tile==0x2e3;
}
static int hud_sprite(const GVDP *vdp,unsigned owner,unsigned tile,int spell) {
    /* 3CF4 HUD graphic pieces; 9216 cursor/minimap; 3EAC HUD indicator.
     * World markers/effects keep world coordinates even beneath the old HUD. */
    unsigned address=(((unsigned)vdp->reg[5]&127)<<9)+(owner-1)*8;
    int source_x=(int)((((unsigned)vdp->vram[(address+6)&65535]<<8)|vdp->vram[(address+7)&65535])&1023)-128;
    return !spell && source_x>=208 && ((tile>=0x152 && tile<=0x1b2) ||
                            tile==0x2af || tile==0x2b8 || tile==0x2da);
}
static unsigned vword(const GVDP *v,unsigned p) {
    return ((unsigned)v->vram[p&65535]<<8)|v->vram[(p+1)&65535];
}
static int wrap_coord(int value,int period){int r=value%period;return r<0?r+period:r;}
/* Lightning 132D8..13630 writes authored clouds/bolts across plane A and uses
 * cell-row Hscroll. Sample that plane directly; none of it is a relocated HUD. */
static uint8_t authored_plane_a(const GVDP *v,int x,int y) {
    static const int dimensions[4]={32,64,0,128};
    int wt=dimensions[v->reg[16]&3],ht=dimensions[(v->reg[16]>>4)&3];
    if(!wt || !ht)return 0;
    int row=y<0?0:y>223?223:y,mode=v->reg[11]&3;
    int hi=mode==0?0:mode==2?(row&~7):row;
    unsigned hs=vword(v,((v->reg[13]&63)<<10)+(unsigned)hi*4)&1023;
    int vs=v->vsram[0]&1023;
    if(v->reg[11]&4) {
        int col=x/16-(x<0 && x%16!=0);if(col<0)col=0;if(col>19)col=19;
        vs=v->vsram[col*2]&1023;
    }
    int xx=wrap_coord(x-(int)hs,wt*8),yy=wrap_coord(y+vs,ht*8);
    unsigned base=(v->reg[2]&0x38)<<10;
    unsigned attr=vword(v,base+2*((unsigned)(yy/8)*wt+(unsigned)xx/8));
    unsigned px=(unsigned)xx&7,py=(unsigned)yy&7;
    if(attr&0x800)px=7-px;
    if(attr&0x1000)py=7-py;
    unsigned bits=v->vram[((attr&2047)*32+py*4+px/2)&65535];
    unsigned color=(bits>>(px&1?0:4))&15;
    return (uint8_t)(color|((attr>>9)&48)|(color?64:0)|((attr>>8)&128));
}
/* 6372/65F2 cull these absolute weather cells to a six-by-six native mask.
 * Read the complete bounded lists instead; do not extend the mask by repeating it. */
static uint8_t weather_plane(const GVDP *v,const uint8_t *ram,int x,int y) {
    int dx=x-16,dy=y-16;
    int cx=ram[0x7bd4]+dx/32-(dx<0 && dx%32!=0);
    int cy=ram[0x7bd5]+dy/32-(dy<0 && dy%32!=0);
    if(cx<0 || cy<0 || cx>=40 || cy>=40)return 0;
    int covered=0;
    for(unsigned type=0;type<2;type++)if(ram[0x168]&(4u<<type)) {
        unsigned base=type?0x1864:0x181a;
        for(unsigned i=0;i<37;i++) {
            unsigned px=ram[base+2*i];if(px==255)break;
            if(px==(unsigned)cx && ram[base+2*i+1]==(unsigned)cy){covered=1;break;}
        }
    }
    if(!covered)return 0;
    unsigned tx=(unsigned)dx&31,ty=(unsigned)dy&31;
    unsigned table=(ram[0x168]&64)?0x6514:0x64f4;
    unsigned p=table+2*((ty/8)*4+tx/8);
    unsigned attr=0x8000+(((unsigned)g_rom[p]<<8)|g_rom[p+1]);
    tx&=7;ty&=7;if(attr&0x800)tx=7-tx;if(attr&0x1000)ty=7-ty;
    unsigned bits=v->vram[(((attr&2047)*32)+ty*4+tx/2)&65535];
    unsigned color=(bits>>(tx&1?0:4))&15;
    return (uint8_t)(color|((attr>>9)&48)|(color?64:0)|((attr>>8)&128));
}
/* Presentation-only SAT walk: preserve linked first-wins order and masking,
 * but do not clip effect geometry at raster column320 or run VDP evaluation. */
static void effect_sprites(const GVDP *v,int line,int w,int spell,int sh,
                          uint8_t *sprite,uint8_t *opaque,uint8_t *high,
                          uint8_t *shadow,uint8_t *hilite) {
    uint8_t claimed[768]={0},seen[128]={0};unsigned link=0,on_line=0;
    unsigned base=((unsigned)v->reg[5]&127)<<9;
    for(unsigned n=0;n<80;n++) {
        if(seen[link])break;
        seen[link]=1;
        unsigned p=base+link*8,size=v->vram[(p+2)&65535],attr=vword(v,p+4),tile=attr&2047;
        unsigned next=v->vram[(p+3)&65535]&127;
        int yy=(int)(vword(v,p)&1023)-128,xx=(int)(vword(v,p+6)&1023)-128;
        unsigned hcells=(size>>2&3)+1,vcells=(size&3)+1;
        int height=(int)vcells*8,width=(int)hcells*8;
        if(line>=yy && line<yy+height) {
            if(++on_line>1 && xx==-128)break;
            if(!actor_owner(v,link+1) && !embedded_marker(tile) && !hud_sprite(v,link+1,tile,spell)) {
                unsigned py=(unsigned)(line-yy);if(attr&0x1000)py=(unsigned)height-1-py;
                for(int px=0;px<width;px++) {
                    int x=xx+px;if(x<0 || x>=w || claimed[x])continue;
                    unsigned ix=(unsigned)px;if(attr&0x800)ix=(unsigned)width-1-ix;
                    unsigned t=(tile+(ix/8)*vcells+py/8)&2047;
                    unsigned bits=v->vram[(t*32+(py&7)*4+(ix&7)/2)&65535];
                    unsigned color=(bits>>((ix&1)?0:4))&15;if(!color)continue;
                    unsigned pal=(attr>>13)&3;
                    claimed[x]=1;sprite[x]=(uint8_t)(pal*16+color);opaque[x]=1;
                    high[x]=(uint8_t)(attr>>15);shadow[x]=hilite[x]=0;
                    if(sh && pal==3 && color>=14){opaque[x]=0;if(color==14)hilite[x]=1;else shadow[x]=1;}
                }
            }
        }
        if(!next)break;
        link=next;
    }
}
static void marker(const GVDP *vdp,int line,int mx,int my,unsigned tile,int edge,
                   uint8_t *sprite,uint8_t *opaque,uint8_t *high,uint8_t *shadow,uint8_t *hilite) {
    if(line<my || line>=my+8)return;
    for(unsigned px=0;px<8;px++) {
        int x=mx+(int)px;
        unsigned address=tile*32+(unsigned)(line-my)*4+px/2;
        unsigned color=(vdp->vram[address&65535]>>(px&1?0:4))&15;
        if(x<0 || x>=edge || !color)continue;
        sprite[x]=(uint8_t)color;opaque[x]=1;high[x]=shadow[x]=hilite[x]=0;
    }
}
/* The entire canvas has one continuous world coordinate system, even behind
 * transparent HUD pixels. Native ARGB is used only for complete fallback scenes. */
static void world_row(const GVDP *vdp,const uint8_t *ram,int line,int w,
                      const GVDPRasterRow *raster,int spell,int fullplane,uint32_t *out) {
    if(w<320 || w>768)return;
    uint8_t a[768]={0},b[768]={0},sprite[768]={0},opaque[768]={0},high[768]={0};
    uint8_t shadow[768]={0},hilite[768]={0},pixels[768];
    Db1WorldContext c={g_rom,1048576,ram,65536,vdp->vram,sizeof vdp->vram};
    int edge=w-112,sx=word(ram+0x204),sy=word(ram+0x206);
    int shake_x=0,shake_y=0;
    /* 5CCC/61DA temporarily displace B without changing the camera fields.
     * Continue that same raster displacement instead of exposing a rectangle. */
    if((vdp->reg[13]&63)==0x34 && (vdp->reg[4]&7)==7) {
        int row=line<0?0:line>223?223:line,mode=vdp->reg[11]&3;
        int index=mode==0?0:mode==2?(row&~7):row;
        unsigned actual=vword(vdp,0xd002+(unsigned)index*4);
        shake_x=(int)(int16_t)(actual-(unsigned)(uint16_t)word(ram+0x200));
        shake_y=(int)((vdp->vsram[1]-(unsigned)(uint16_t)word(ram+0x202)+512)&1023)-512;
    }
    int wy=line-16-sy+32*ram[0x7bd5]+shake_y;
    for(int x=0;x<w;x++) {
        int wx=x-16-sx+32*ram[0x7bd4]-shake_x;uint8_t p;
        if(wx>=0 && wy>=0 && db1_world_terrain_plane(&c,(unsigned)wx/32,(unsigned)wy/32,
                                                    (unsigned)wx%32,(unsigned)wy%32,&p))b[x]=p;
    }
    if(fullplane) {
        if(raster)memcpy(b,raster->plane_b,208);
        for(int x=0;x<w;x++)a[x]=authored_plane_a(vdp,x,line);
    }else if(raster) {
        /* Keep authentic center planes, but never reuse the wrapped B plane
         * at old HUD coordinates. Only A carries the relocated HUD and frame. */
        memcpy(b,raster->plane_b,208);
        memcpy(a,raster->plane_a,208);
        for(unsigned x=208;x<768;x++)if((int)x>=edge && (int)x<w)
            a[x]=raster->plane_a[208+x-(unsigned)edge];
        if(line<16 || line>=208)
            for(int x=208;x<edge;x++)a[x]=raster->plane_a[16+(x-208)%192];
    }
    for(int x=0;x<w;x++)if(!fullplane && (!raster || x>=208)) {
        uint8_t cloud=weather_plane(vdp,ram,x,line);
        if((cloud&64) && !(a[x]&64))a[x]=cloud;
    }
    /* Full host list, independent of the 36-entry guest drawing cache. */
    for(unsigned remaining=count;remaining;) {unsigned i=--remaining;
      int actor_x=actors[i].x+shake_x,actor_y=actors[i].y-shake_y;
      if(valid[i] && line>=actor_y && line<actor_y+32) {
        for(int px=0;px<32;px++) {
            int x=actor_x+px;uint8_t p;
            if(x>=0 && x<w && db1_world_actor_pixel(&frames[i],(unsigned)px,(unsigned)(line-actor_y),&p) && (p&15)) {
                sprite[x]=p;opaque[x]=1;high[x]=(frames[i].flags>>15)&1;
                shadow[x]=hilite[x]=0;
                if((raster?raster->shadow_mode:(vdp->reg[12]&8)) && p>=62) {
                    opaque[x]=0;
                    if(p==62)hilite[x]=1;else shadow[x]=1;
                }
            }
        }
      }
      /* 9114..9168 emits these within each actor's SAT group. They must
       * share its depth, so a foreground creature can cover a rear marker. */
      if(!(ram[0x168]&128)) {
        static const unsigned offsets[4]={0x2a,0x3c,0x2b,0x3d};
        static const unsigned tiles[4]={0x2e1,0x2e2,0x2dd,0x2de};
        static const int dx[4]={8,16,24,24};
        int mx=actor_x-sx,my=actor_y-sy;
        for(unsigned t=4;t;) {--t;
            if(ram[0x7bb4+offsets[t]]==actors[i].id)
                marker(vdp,line,mx+dx[t],my,tiles[t],w,sprite,opaque,high,shadow,hilite);
        }
        if(ram[0x152]==actors[i].id)
            marker(vdp,line,mx,my,0x2e3,w,sprite,opaque,high,shadow,hilite);
      }
    }
    effect_sprites(vdp,line,w,spell,raster?raster->shadow_mode:((vdp->reg[12]&8)!=0),
                   sprite,opaque,high,shadow,hilite);
    if(raster)for(int source=0;source<320;source++) {
        unsigned owner=raster->sprite_owner[source];
        if(!owner || actor_owner(vdp,owner) || embedded_marker(owner_tile(vdp,owner)))continue;
        int x=hud_sprite(vdp,owner,owner_tile(vdp,owner),spell)?source+(w-320):source;
        sprite[x]=raster->sprite[source];opaque[x]=raster->opaque[source];high[x]=raster->high[source];
        shadow[x]=raster->shadow_op[source];hilite[x]=raster->hilite_op[source];
    }
    unsigned backdrop=raster?raster->backdrop:(vdp->reg[7]&63);
    int shadow_mode=raster?raster->shadow_mode:((vdp->reg[12]&8)!=0);
    gvdp_compose_span(pixels,a,b,sprite,opaque,high,shadow,hilite,w,(uint8_t)backdrop,shadow_mode);
    for(int x=0;x<w;x++) {
        unsigned p=pixels[x],level=p>=128?GENESIS_DAC_HIGHLIGHT:p>=64?GENESIS_DAC_SHADOW:GENESIS_DAC_NORMAL;
        out[x]=genesis_dac_cram_to_argb(vdp->cram[p&63],(int)level);
    }
}
static void scanline(const GVDP *vdp,int line,int nw,uint32_t *out,int w,
                     GameVideoNativeRow native_row,void *native_ctx,const GVDPRasterRow *raster) {
    const uint32_t *native=native_row(native_ctx);
    uint32_t background=genesis_dac_cram_to_argb(vdp->cram[vdp->reg[7]&63],0);
    int supported=nw==320 && w>=320 && w<=768 && world_active() && raster && raster->display;
    if(line==0) {
        frame_world=0;frame_background=background;
        frame_scene=db1_scenes_kind();
        frame_spell=spell_effect;
        frame_fullplane=fullplane_effect;
        memcpy(frame_vdp.vram,vdp->vram,sizeof frame_vdp.vram);
        memcpy(frame_vdp.cram,vdp->cram,sizeof frame_vdp.cram);
        memcpy(frame_vdp.reg,vdp->reg,sizeof frame_vdp.reg);
        memcpy(frame_vdp.vsram,vdp->vsram,sizeof frame_vdp.vsram);
        if(supported) {
            memcpy(frame_ram,g_ram,sizeof frame_ram);
            Db1WorldContext c={g_rom,1048576,frame_ram,sizeof frame_ram,frame_vdp.vram,sizeof frame_vdp.vram};
            center_supported=1;
            /* Native spell quakes move B and sprites by up to eight pixels.
             * Keep read-only overscan actors so those crossing an edge exist. */
            count=db1_visible_actors_rect(&c,-64,-canvas_top-64,w+64,canvas_height-canvas_top+64,actors);
            for(unsigned i=0;i<count;i++) {
                const uint8_t *record=frame_ram+0x5f34+actors[i].id*16;
                valid[i]=(uint8_t)db1_world_actor_frame(&c,record,actors[i].id,&frames[i]);
                if(record[0]&&!valid[i])center_supported=0;
            }
            frame_world=center_supported;
        }
    }
    if(!supported || !center_supported) {
        if(db1_scenes_row(vdp,line,native,nw,out,w))return;
        for(int x=0;x<w;x++)out[x]=background;
        int offset=(w-nw)/2;if(offset>=0)memcpy(out+offset,native,(size_t)nw*sizeof *out);
        return;
    }
    world_row(vdp,g_ram,line,w,raster,spell_effect||fullplane_effect,fullplane_effect,out);
}
static void canvas_complete(const GVDP *vdp,uint32_t *pixels,int stride,int w,int h,int top,int native_h) {
    (void)vdp;
    if(w<320 || w>768 || h>480 || stride<w || top<0 || top+native_h>h)return;
    for(int y=0;y<h;y++)if(y<top || y>=top+native_h) {
        uint32_t *row=pixels+(size_t)y*stride;
        if(frame_world)world_row(&frame_vdp,frame_ram,y-top,w,NULL,frame_spell||frame_fullplane,frame_fullplane,row);
        else for(int x=0;x<w;x++) {
            if((frame_scene>=DB1_SCENE_BRANDING && frame_scene<=DB1_SCENE_PASSWORD) || frame_scene==DB1_SCENE_GAMEOVER)
                row[x]=db1_scenes_background_for(frame_scene,&frame_vdp,x-(w-320)/2,y-top);
            else row[x]=frame_background;
        }
    }
}
const GameVideo db1_video={.configure=configure,.enabled=is_enabled,.width=width,
    .raster_enabled=raster_enabled,.scanline_presented=scanline,.integer_scale_supported=1,
    .canvas_geometry=canvas_geometry,.canvas_complete=canvas_complete};
