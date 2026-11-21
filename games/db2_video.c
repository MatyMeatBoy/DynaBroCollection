#include "db2_video.h"
#include "db2_world.h"
#include "db2_actors.h"
#include "db2_markers.h"
#include "db2_effects.h"
#include "db2_visor.h"
#include "db2_bio_overlay.h"
#include "db2_wide_menu.h"
#include "db2_menu_labels.h"
#include "viewport.h"
#include "genesis_runtime.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include "video/genesis_vdp_compose.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum { LIMIT=768, MAX_HEIGHT=480 };
static DynaViewport view=DYNA_VIEWPORT_DEFAULT;
enum { WORLD_NONE, WORLD_BATTLE, WORLD_DEMO, WORLD_VERSUS };
static int world_scene,briefing_scene,results_scene,pause_active,ready,canvas_top,canvas_height=224;
static int actor_effect_active;
static int title_sky;
static int dialogue_hold;
static uint32_t sky_base;
static uint8_t ram[65536];
static GVDP snapshot;
static Db2WorldCamera camera;
static Db2Actor actors[254];
static unsigned count,rank[255];
static Db2WorldEffect effects[80];
static size_t effect_count;
static uint32_t background;
static uint32_t native_image[224][320];
/* Native row plus plane/sprite margins (db2_wide_menu): the presentation used
 * for every frame that is not an accepted widened battle frame. */
static uint32_t wide_image[224][LIMIT];
static uint8_t wide_valid[224];
static int wide_w;
static uint8_t native_valid[224],owner_kind[80];
static int rejected;
static int world_is_a;
static int hud_on_left;
/* Displayed-vs-modelled terrain alignment. During held camera pans the guest
 * advances the VDP scroll in 16-pixel steps while the RAM camera cell only
 * moves every 32 pixels, so half of the frames show the plane one sub-step
 * ahead of the RAM model. The displayed origin is VDP scroll + k (k fixed while
 * the nametable ring is coherent); off_x/off_y re-align the reconstructed
 * terrain with what the guest actually shows. Every widened row is still
 * verified against the native plane, so a wrong guess only rejects a frame. */
static int have_k,kx,ky,off_x,off_y,model_x,model_y,disp_x,disp_y,plane_w,plane_h,heal_pending,heal_dx,heal_dy;
/* Keep using the native raster through the RAM/SAT update after a scroll. */
enum { SCROLL_SETTLE_FRAMES=3 };
static int scroll_x,scroll_y,scroll_known;
static unsigned moving_frames;
static int visor_offset;
static int free_hud;
static uint8_t hud_edges[2][8][104];
static int trace_enabled,trace_frames;
static unsigned frame_reasons,trace_seen,trace_signature,trace_budget;
static unsigned trace_ok_frames,trace_fallback_frames,trace_reason_frames[6];
/* Diagnostic-only producer epoch counter (evidence/db2-visor-research/coherence.md
 * step 1): 0x146C0 is the cache/gather producer's dirty entry point, 0x15220 its
 * completion point (MOVEM restore + RTS after the source-list terminator). This
 * only counts transitions and logs when a widened frame's capture() falls inside
 * a dirty span; it does not yet change frame accept/reject behavior (steps 2-4
 * of the proposed contract are not implemented). */
static unsigned producer_epoch,producer_dirty_since_epoch;
/* Last accepted widened battle frame, re-presented for at most HOLD_FRAMES
 * consecutive rejected frames so a one-frame SAT/RAM incoherence or scene
 * hand-off never flashes the 4:3 native image. Presentation only. */
enum { HOLD_FRAMES=3 };
static uint32_t held[MAX_HEIGHT][LIMIT];
static int held_w,held_h,held_valid;
static unsigned held_misses;
void db2_video_visor_offset(int tiles){visor_offset=tiles;}
void db2_video_free_hud(int enabled){free_hud=enabled!=0;}
int db2_video_visor_requested(void){return visor_offset;}
static int plane_layout(const GVDP *v){
 unsigned a=((unsigned)v->reg[2]&0x38)<<10,b=((unsigned)v->reg[4]&7)<<13;
 if(a==0xc000&&b==0xe000){world_is_a=1;return 1;}
 if(a==0xe000&&b==0xc000){world_is_a=0;return 1;}
 return 0;
}
static unsigned word(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
void db2_video_reset(void){db2_bio_overlay_reset();world_scene=briefing_scene=results_scene=pause_active=actor_effect_active=title_sky=dialogue_hold=ready=rejected=0;count=0;effect_count=0;canvas_top=0;canvas_height=224;memset(native_valid,0,sizeof native_valid);const char *t=getenv("DYNA_DB2_VIDEO_TRACE");trace_enabled=t&&(!strcmp(t,"1")||!strcmp(t,"2"));trace_frames=t&&!strcmp(t,"2");frame_reasons=trace_seen=0;trace_signature=~0u;trace_budget=32;producer_epoch=producer_dirty_since_epoch=0;have_k=off_x=off_y=heal_pending=scroll_known=0;held_valid=0;held_misses=moving_frames=0;}
void db2_video_observe(unsigned pc){
 if(pc==0x3b64){briefing_scene=1;title_sky=0;}
 else if(pc==0x3b66)briefing_scene=0;
 else if(pc==0x3c70)results_scene=1;
 else if(pc==0x3c78)results_scene=0;
 else if(pc==0x3bb0){
  world_scene=WORLD_BATTLE;briefing_scene=results_scene=pause_active=actor_effect_active=title_sky=0;
  held_valid=have_k=heal_pending=scroll_known=0;moving_frames=0;
 }
 else if(pc==0x734e){world_scene=WORLD_VERSUS;held_valid=have_k=heal_pending=scroll_known=0;moving_frames=0;}
 else if(pc==0xf794)world_scene=WORLD_DEMO;
 else if(pc==0x3bb4||pc==0x7308||pc==0xf728)world_scene=WORLD_NONE;
 else if(pc==0x425c)pause_active=1;
 else if(pc==0x4422)pause_active=0;
 else if(pc==0xc158)actor_effect_active=!(g_ram[0x286]&0x88);
 else if(pc==0xc298)actor_effect_active=0;
 else if(pc==0x146c0)producer_dirty_since_epoch=1;
 else if(pc==0x15220){if(producer_dirty_since_epoch)producer_epoch++;producer_dirty_since_epoch=0;}
}
unsigned db2_video_producer_epoch(void){return producer_epoch;}
int db2_video_producer_dirty(void){return producer_dirty_since_epoch;}
static int configure(const char *s){return dyna_viewport_configure(&view,s);}
static int enabled(void){return view.enabled;}
static int width(int dw,int dh,int nw,int nh){(void)nw;canvas_top=0;canvas_height=nh;return dyna_viewport_width(&view,dw,dh,nh);}
static int native_pause(void){
 /* FF006C owns the native pause tick while the battlefield is still live.
  * Other users of bit 7 can replace the entire screen (Info View, effects). */
 return (g_ram[0x286]&0x80)&&word(g_ram+0x6c)==0&&word(g_ram+0x6e)==0x442c;
}
static int world_raster_enabled(void){
 unsigned modal_mask=world_scene==WORLD_VERSUS?0x80u:0x88u;
 /* The actor animation sets the modal bit while the stage stays visible. */
 return world_scene!=WORLD_NONE&&(!(g_ram[0x286]&modal_mask)||native_pause()||
        (world_scene==WORLD_BATTLE&&actor_effect_active&&!(g_ram[0x286]&8)));
}
static int raster_enabled(const GVDP *v){
 /* The guest can park the logo during this frame, after raster setup. */
 return world_raster_enabled()||db2_video_title_active(v);
}
static int geometry(int dw,int dh,int nw,int nh,int *w,int *h){
 if(!view.enabled||nw!=320||nh!=224)return 0;
 int scale=game_video_integer_canvas(dw,dh,dyna_viewport_width(&view,dw,dh,nh),nh,w,h);
 if(!scale||*w>LIMIT||*h>MAX_HEIGHT)return 0;
 canvas_top=(*h-nh)/2;canvas_height=*h;return scale;
}
static void capture(const GVDP *v){
 if(trace_enabled&&producer_dirty_since_epoch)
  fprintf(stderr,"[db2-video] capture during dirty producer epoch=%u pc=%06X\n",producer_epoch,g_cpu.PC);
 int left=world_scene==WORLD_VERSUS&&word(g_ram+0x2a5a)==0xe002&&
          word(g_ram+0x2a5c)==0xe01c&&word(g_ram+0x2a5e)==8&&word(g_ram+0x2a60)==112;
 if(left!=hud_on_left||(world_scene==WORLD_VERSUS&&memcmp(ram+0x2a56,g_ram+0x2a56,4)))
  held_valid=have_k=heal_pending=0;
 hud_on_left=left;
 memcpy(ram,g_ram,sizeof ram);memcpy(&snapshot,v,sizeof snapshot);
 Db2WorldContext c={g_rom,2621440,ram,sizeof ram,snapshot.vram,sizeof snapshot.vram};
 int right=word(ram+0x2a5a)==0xe036&&word(ram+0x2a5c)==0xe004&&
           word(ram+0x2a5e)==216&&word(ram+0x2a60)==16;
 int previous_anchor_x=camera.anchor_x,previous_anchor_y=camera.anchor_y;
 ready=db2_world_read_camera(&c,&camera)&&plane_layout(v)&&(left||right);
 off_x=off_y=0;
 if(ready){
  /* A native camera reset can move the anchor a large distance at once
   * (Practice moves from the cinematic's x=160 to the playfield's x=16).
   * The previous ring calibration is only valid for small scrolling steps. */
  if(abs(camera.anchor_x-previous_anchor_x)>32||abs(camera.anchor_y-previous_anchor_y)>32)
   have_k=heal_pending=0;
  static const int sizes[4]={256,512,0,1024};
  plane_w=sizes[snapshot.reg[16]&3];plane_h=sizes[(snapshot.reg[16]>>4)&3];
  unsigned hb=((unsigned)snapshot.reg[13]&63)<<10;
  int hs=(int)(word(snapshot.vram+((hb+(world_is_a?0:2))&65535))&1023);
  int vs=(int)(snapshot.vsram[world_is_a?0:1]&1023);
  model_x=(int)camera.cell_x*32-camera.anchor_x;model_y=(int)camera.cell_y*32-camera.anchor_y;
  disp_x=-hs;disp_y=vs;
  if(heal_pending&&plane_w&&plane_h){kx=model_x+heal_dx-disp_x;ky=model_y+heal_dy-disp_y;have_k=1;heal_pending=0;}
  if(have_k&&plane_w&&plane_h){
   off_x=((disp_x+kx-model_x)%plane_w+plane_w+plane_w/2)%plane_w-plane_w/2;
   off_y=((disp_y+ky-model_y)%plane_h+plane_h+plane_h/2)%plane_h-plane_h/2;
   if(off_x<-64||off_x>64||off_y<-64||off_y>64)off_x=off_y=0;
  }
 }
 if(!ready)frame_reasons|=1;
 Db2ActorsContext ac={g_rom,2621440,ram,sizeof ram};count=0;
 for(unsigned id=0;id<255;id++)rank[id]=72+id;
 unsigned off=word(ram+0x9d3a);uint8_t seen[72]={0};
 for(unsigned n=0;n<72&&off;n++){
  if(off<4||(off-4)%8||(off-4)/8>=72||seen[(off-4)/8]){ready=0;frame_reasons|=2;break;}
  seen[(off-4)/8]=1;const uint8_t *p=ram+0x9d3a+off;
  if(p[2]>0&&p[2]<255)rank[p[2]]=n;
  off=word(p);
 }
 for(unsigned id=1;id<=254;id++){
  const uint8_t *p=ram+0x5f58+48*id;
  if(!p[0])continue;
  /* A single actor mid-transition (e.g. the frame it dies on) can fail to
   * decode this frame without the OTHER actors (already correctly added
   * below) needing to be thrown away by a whole-frame fallback; the DB1
   * engine had the exact same pattern (games/db1_video.c prepare_actors)
   * and real play data here confirms reason 04 fires during ordinary
   * gameplay, not just the already-documented pre-battle window. */
  Db2Actor a;if(!(db2_actor_decode_cached(&ac,id,&a)||db2_actor_decode(&ac,id,&a)))continue;
  actors[count++]=a;
 }
 for(unsigned i=0;i<count;i++)for(unsigned j=i+1;j<count;j++)
  if(actors[i].depth<actors[j].depth||(actors[i].depth==actors[j].depth&&rank[actors[i].id]>rank[actors[j].id])){Db2Actor a=actors[i];actors[i]=actors[j];actors[j]=a;}
 effect_count=db2_effects_collect(&c,((unsigned)v->reg[5]&127)<<9,effects,80);
 for(size_t i=0;i<effect_count;i++)for(size_t j=i+1;j<effect_count;j++)
  if(effects[i].depth<effects[j].depth||(effects[i].depth==effects[j].depth&&effects[i].rank>effects[j].rank)){Db2WorldEffect e=effects[i];effects[i]=effects[j];effects[j]=e;}
}
/* Native 155CE/15648 compacts source records at FF2752 into linked SAT.
 * Recognize UI by exact source address, pattern, size AND coordinates.
 * A pixel's current X coordinate never establishes HUD ownership. */
static int ui_kind(unsigned attr,unsigned size,int x,int y){
 for(unsigned i=0;i<32;i++){
  const uint8_t *p=ram+0x2752+8*i;
  if(word(p+4)!=attr||(word(p+2)>>8)!=size||
     ((word(p+6)+128)&511)!=(unsigned)((x+128)&511)||
     ((word(p)+128)&1023)!=(unsigned)((y+128)&1023))continue;
  if(i<6&&attr==0x86e7&&size==0)return 2; /* HUD counters, 691A/6CBC */
  if(i==6&&attr==0x82f4&&size==10&&!(ram[0x2a89]&4)){
   /* Native input dispatch41DE: modes0/1/2 retain a HUD command hand;
    * modes3/5 project its shared coordinates into the world. */
   /* Native15B46/15CAC sets/clears bit7 while the same camera mode scrolls. */
   unsigned mode=ram[0x2a88]&0x7f;
   if(mode<=2)return 2;
   if(mode==3||mode==5){
    /* Command activation updates the mode before the hand's coordinates.
     * Keep the old SAT entry attached to the HUD until it reaches the map. */
    return (hud_on_left?x<104:x>=216)?2:1;
   }
  }
  if(i>=8&&i<=11&&(attr&0xe7ff)==0x86e8&&size==5)return 1;
  if((i==12&&attr==0x86e1&&size==5)||(i==13&&attr==0x8372&&size==5))return 2;
  if((i==14&&attr==0x836a&&size==13)||(i==15&&attr==0x833f&&size==13))return 2;
  if(i>=22&&i<=30&&((attr==0x8364&&size==9)||(attr==0x8360&&size==5)))return 2;
  if(i==22&&(attr&0x9fff)==0x87e9&&size==15)return 2;
  if(i==31&&attr==0x032f&&size==15)return 1;
 }
 return 0;
}
static int sprite_transparent(unsigned attr,unsigned size){
 unsigned tiles=(((size>>2)&3)+1)*((size&3)+1);
 for(unsigned t=0;t<tiles;t++){
  const uint8_t *p=snapshot.vram+((((attr&2047)+t)&2047)*32);
  for(unsigned i=0;i<32;i++)if(p[i])return 0;
 }
 return 1;
}
static uint8_t tile_pixel(unsigned attr,unsigned size,int px,int py);
static void classify_sprites(void){
 Db2ActorsContext ac={g_rom,2621440,ram,sizeof ram};
 unsigned base=((unsigned)snapshot.reg[5]&127)<<9,link=0;uint8_t seen[80]={0};
 memset(owner_kind,0,sizeof owner_kind);
 for(unsigned n=0;n<80;n++){
  if(link>=80||seen[link]){rejected=1;frame_reasons|=8;return;}seen[link]=1;
  unsigned p=base+link*8,attr=word(snapshot.vram+((p+4)&65535));
  unsigned size=snapshot.vram[(p+2)&65535],next=snapshot.vram[(p+3)&65535]&127;
  int x=(int)(word(snapshot.vram+((p+6)&65535))&511)-128;
  int y=(int)(word(snapshot.vram+(p&65535))&1023)-128;
  int kind=ui_kind(attr,size,x,y);
  if(!attr)kind=1;
  if(db2_actor_native_sat_body(&ac,attr)||db2_effects_is_marker((uint16_t)attr))kind=3;
  for(size_t i=0;i<effect_count;i++)if(effects[i].sat_index==link)kind=3;
  /* An unattributed entry whose every referenced pattern is fully transparent
   * contributes no pixel in either the native or widened raster (seen as a
   * parked 32x32 pattern-0 body at x=-32 during Normal2 play). Ownership is
   * irrelevant for it; any non-zero pattern byte still rejects the frame. */
  if(!kind&&sprite_transparent(attr,size))kind=3;
  /* Gameplay prompt banner ("HATCH EGGS FOR DINOS", written by the native
   * routine around 0CA8): a row of 32x32 palette-1 sprites using patterns
   * 370-47F (palette 1 or 2, either flip), slid in from the right and centered on the native 216-pixel
   * world view. It belongs to the world view, so it follows the world shift
   * instead of forcing the whole frame back to the native image. */
  if(!kind&&size==15&&(attr&0x6000)&&(attr&0x6000)!=0x6000&&(attr&0x7ff)>=0x370&&(attr&0x7ff)<0x480)kind=1;
  if(!kind){
   /* Unknown entry: not rejected here. world_row rejects the frame only if one
    * of its pixels is visible in the native raster (sprite_owner without a
    * kind); hidden, off-screen or limit-dropped entries contribute nothing. */
   if(trace_enabled){
    static unsigned unk_attr[16],unk_size[16],unk_count[16],unk_n;unsigned u=0;
    for(;u<unk_n;u++)if(unk_attr[u]==attr&&unk_size[u]==size)break;
    if(u==unk_n&&unk_n<16){unk_attr[u]=attr;unk_size[u]=size;unk_n++;fprintf(stderr,"[db2-video] unowned SAT link=%u attr=%04X size=%02X xy=%d,%d pc=%06X\n",link,attr,size,x,y,g_cpu.PC);}
    if(u<16&&++unk_count[u]%500==0)fprintf(stderr,"[db2-video] unowned SAT attr=%04X size=%02X seen=%u\n",attr,size,unk_count[u]);
   }
   owner_kind[link]=0;
   if(!next)return;
   link=next;continue;
  }
  owner_kind[link]=(uint8_t)kind;
  if(!next)return;
  link=next;
 }
 rejected=1;frame_reasons|=8;
}
static uint8_t tile_pixel(unsigned attr,unsigned size,int px,int py){
 unsigned hc=((size>>2)&3)+1,vc=(size&3)+1;
 unsigned x=(unsigned)px,y=(unsigned)py;
 if(attr&0x800)x=hc*8-1-x;
 if(attr&0x1000)y=vc*8-1-y;
 unsigned tile=((attr&2047)+(x/8)*vc+y/8)&2047;
 unsigned b=snapshot.vram[(tile*32+(y&7)*4+(x&7)/2)&65535];
 return (uint8_t)(((attr>>9)&48)|((b>>((x&1)?0:4))&15));
}
static void put(uint8_t *s,uint8_t *o,uint8_t *hi,int x,int w,uint8_t p,unsigned attr){
 if(x<0||x>=w||!(p&15))return;
 s[x]=p;o[x]=1;hi[x]=(uint8_t)(attr>>15);
}
static void effect_row(const Db2WorldEffect *e,int y,int w,int shift,uint8_t *s,uint8_t *o,uint8_t *hi){
 int h=((e->size&3)+1)*8,ww=(((e->size>>2)&3)+1)*8;
 if(y<e->y||y>=e->y+h)return;
 for(int x=0;x<ww;x++)put(s,o,hi,e->x+x+shift,w,tile_pixel(e->attr,e->size,x,y-e->y),e->attr);
}
static uint8_t terrain_pixel(const Db2WorldContext *c,int x,int y){
 uint8_t pixel=0;
 int wx=x-camera.anchor_x+(int)camera.cell_x*32+off_x;
 int wy=y-camera.anchor_y+(int)camera.cell_y*32+off_y;
 if(wx>=0&&wy>=0&&wx<1280&&wy<1280)
  db2_world_terrain_plane(c,(unsigned)wx/32,(unsigned)wy/32,(unsigned)wx%32,(unsigned)wy%32,&pixel);
 return pixel;
}
static void world_row(int y,int w,const GVDPRasterRow *r,uint32_t *out){
 uint8_t a[LIMIT]={0},b[LIMIT]={0},s[LIMIT]={0},o[LIMIT]={0},hi[LIMIT]={0};
 uint8_t sh[LIMIT]={0},hl[LIMIT]={0},pixels[LIMIT];
 Db2WorldContext c={g_rom,2621440,ram,sizeof ram,snapshot.vram,sizeof snapshot.vram};
 Db2VisorLayout layout;
 if(!db2_visor_layout(w,visor_offset,&layout)){rejected=1;return;}
 int native_hud=hud_on_left?0:216;
 int world_start=hud_on_left?104:0,world_end=world_start+216;
 if(hud_on_left){layout.hud_left=0;layout.hud_shift=0;}
 int shift=layout.world_shift_x;
 uint8_t *terrain=world_is_a?a:b,*ui=world_is_a?b:a;
 for(int x=0;x<w;x++){
  terrain[x]=terrain_pixel(&c,x-shift,y);
 }
 /* Verify the entire native world span before accepting this frame. Weather,
  * unknown scroll/raster effects and terrain mismatches trigger whole-frame
  * native rollback at frame_complete, never a cut at the old HUD boundary. */
 if(r){
  const uint8_t *native_terrain=world_is_a?r->plane_a:r->plane_b;
  const uint8_t *native_ui=world_is_a?r->plane_b:r->plane_a;
  /* A line the guest leaves completely blank (no plane pixel, native image is
   * pure backdrop across the world span) is blank in the widened view too. */
  int blank=y>=0&&y<224&&native_valid[y];
  for(int x=world_start;blank&&x<world_end;x++)if(native_terrain[x]||native_ui[x]||native_image[y][x]!=background)blank=0;
  if(blank)for(int x=0;x<w;x++)terrain[x]=0;
  for(int x=world_start;x<world_end;x++){
   if(blank)break;
   int dx=x+shift;
   uint8_t expected=dx>=0&&dx<w?terrain[dx]:terrain_pixel(&c,x,y);
   /* Terrain under an opaque high-priority UI pixel is invisible; the native
    * plane capture may omit it, so it cannot be compared. */
   if(expected!=native_terrain[x]&&!((native_ui[x]&0x80)&&(native_ui[x]&15))){
    if(trace_enabled&&!rejected){static unsigned n;if(n++%25==0)fprintf(stderr,"[db2-video] terrain mismatch y=%d x=%d expected=%02X native=%02X anchor=%d,%d cell=%d,%d shift=%d\n",y,x,expected,native_terrain[x],camera.anchor_x,camera.anchor_y,(int)camera.cell_x,(int)camera.cell_y,shift);}
    if(trace_enabled&&!rejected){static unsigned m;if(m++%25==0){unsigned bad=0,nz=0,bgc=0;for(int q=0;q<216;q++){bgc+=(y>=0&&y<224&&native_valid[y]&&native_image[y][q]==background);int dd=q+shift;uint8_t ex=dd>=0&&dd<w?terrain[dd]:terrain_pixel(&c,q,y);if(ex!=native_terrain[q])bad++;if(native_terrain[q])nz++;}fprintf(stderr,"[db2-video]   row y=%d mismatching=%u native_nonzero=%u native_img_bg=%u winH=%02X winV=%02X reg0=%02X reg11=%02X vs=%d ui=%02X,%02X,%02X,%02X spr=%02X,%02X,%02X,%02X img=%08X,%08X bg=%08X exp=%02X,%02X\n",y,bad,nz,bgc,snapshot.reg[17],snapshot.reg[18],snapshot.reg[0],snapshot.reg[11],world_is_a,native_ui[0],native_ui[50],native_ui[100],native_ui[200],r->sprite[0],r->sprite[50],r->sprite[100],r->sprite[200],native_image[0][0],native_image[0][100],background,terrain[shift],terrain[shift+100]);}}
    if(!rejected&&!heal_pending){
     /* Find the alignment the guest is actually showing on this row and use it
      * from the next frame on. Ambiguous rows (no unique full match) are left. */
     int found=0,fx=0,fy=0,sx=off_x,sy=off_y;
     for(int dy=-32;dy<=32&&found<2;dy+=4)for(int dx=-32;dx<=32&&found<2;dx+=4){
      off_x=sx+dx;off_y=sy+dy;int q=world_start;
      for(;q<world_end;q++)if(terrain_pixel(&c,q,y)!=native_terrain[q]&&!((native_ui[q]&0x80)&&(native_ui[q]&15)))break;
      if(q==world_end){found++;fx=off_x;fy=off_y;}
     }
     off_x=sx;off_y=sy;
     if(found==1){heal_pending=1;heal_dx=fx;heal_dy=fy;}
    }
    rejected=1;frame_reasons|=16;}
   if(dx>=0&&dx<w)ui[dx]=native_ui[x];
  }
  if(y>=0&&y<8)memcpy(hud_edges[0][y],native_ui+native_hud,104);
  if(y>=216&&y<224)memcpy(hud_edges[1][y-216],native_ui+native_hud,104);
  if(!free_hud)memcpy(ui+layout.hud_left,native_ui+native_hud,104);
  else if(y>=13&&y<=212){
   /* The bezel is narrower above its right-hand shadow. Follow that
    * silhouette while keeping the native minimap and controls in place. */
   int start=(hud_on_left?12:4)+(y>=211?y-208:0);
   int end=(hud_on_left?102:94)-(y<=15?2:0);
   memcpy(ui+layout.hud_left+start,native_ui+native_hud+start,(size_t)(end-start));
  }
 }else if(!free_hud&&(y<0||y>=224)){
  /* Continue the 8-pixel backing texture with the same vertical phase. */
  memcpy(ui+layout.hud_left,hud_edges[y>=224][(unsigned)y&7],104);
 }
 size_t ei=effect_count;
 /* RAM actors use the modelled origin. The native SAT, including effects
  * and the HUD, is already in display coordinates. */
 int actor_shift_x=shift-off_x;
 for(unsigned remaining=count;remaining;){const Db2Actor *ac=actors+--remaining;
  while(ei&&(effects[ei-1].depth<ac->depth||(effects[ei-1].depth==ac->depth&&effects[ei-1].rank>rank[ac->id])))effect_row(effects+--ei,y,w,shift,s,o,hi);
  for(unsigned part=ac->component_count;part;){const Db2ActorComponent *p=ac->part+--part;
   int xx=ac->x+p->dx+actor_shift_x,yy=ac->y+p->dy-off_y;if(y<yy||y>=yy+32)continue;
   for(int px=0;px<32;px++){uint8_t color;db2_actor_pixel(p,(unsigned)px,(unsigned)(y-yy),&color);put(s,o,hi,xx+px,w,color,p->flags);}
  }
  Db2WorldMarker m[6];unsigned n=db2_world_markers(ram[0x2a89],ram[0x5f58+48*ac->id+0x28],ac->x,ac->y-off_y,m);
  while(n){const Db2WorldMarker *p=m+--n;if(y<p->y||y>=p->y+8)continue;for(int px=0;px<8;px++)put(s,o,hi,p->x+px+actor_shift_x,w,tile_pixel(p->attribute,0,px,y-p->y),p->attribute);}
 }
 while(ei)effect_row(effects+--ei,y,w,shift,s,o,hi);
 if(r){
  int native_motion=moving_frames||off_x||off_y;
  if(native_motion){
   /* While the scroll and the actor producer publish at different times,
    * keep the native sprite raster authoritative over the native world. */
   int start=world_start+shift,end=world_end+shift;
   if(start<0)start=0;
   if(end>w)end=w;
   for(int x=start;x<end;x++)s[x]=o[x]=hi[x]=0;
  }
  int unknown_traced=0;
  for(int x=0;x<320;x++){unsigned owner=r->sprite_owner[x];if(!owner)continue;
   if(owner>80){rejected=1;frame_reasons|=8;continue;}
   /* A visible SAT entry without a recognized source is usually a HUD or unit
    * pattern whose SAT update lands one frame apart from its RAM source. Its
    * native pixels are kept, placed by the native region they occupy (HUD
    * column 216+ or world view), instead of rolling the whole frame back to
    * the native image and flickering between 16:9 and 4:3. */
   int kind=owner_kind[owner-1];
   if(!kind){
    if(trace_frames&&!(unknown_traced)){unsigned p=((((unsigned)snapshot.reg[5]&127)<<9)+(owner-1)*8)&65535;fprintf(stderr,"[db2-reject] y=%d x=%d link=%u attr=%04X size=%02X sx=%d sy=%d\n",y,x,owner-1,word(snapshot.vram+((p+4)&65535)),snapshot.vram[(p+2)&65535],(int)(word(snapshot.vram+((p+6)&65535))&511)-128,(int)(word(snapshot.vram+p)&1023)-128);}
    unknown_traced=1;
    unsigned address=(((unsigned)snapshot.reg[5]&127)<<9)+(owner-1)*8;
    int sx=(int)(word(snapshot.vram+((address+6)&65535))&511)-128;
    unsigned size=snapshot.vram[(address+2)&65535];
    int half_width=(int)(((size>>2)&3)+1)*4;
    int center=sx+half_width;
    kind=(hud_on_left?center<104:center>=216)?2:1;
   }
   if(kind==3){
    if(!native_motion)continue;
    kind=1;
   }
   int dx=x+(kind==2?layout.hud_shift:shift);
   if(dx<0||dx>=w)continue;
   s[dx]=r->sprite[x];o[dx]=r->opaque[x];hi[dx]=r->high[x];sh[dx]=r->shadow_op[x];hl[dx]=r->hilite_op[x];
  }
 }
 int shadow=r?r->shadow_mode:((snapshot.reg[12]&8)!=0);
 for(int x=0;x<w;x++)if(shadow&&o[x]&&s[x]>=62){o[x]=0;hl[x]=(s[x]==62);sh[x]=(s[x]==63);}
 gvdp_compose_span(pixels,a,b,s,o,hi,sh,hl,w,r?r->backdrop:(snapshot.reg[7]&63),shadow);
 for(int x=0;x<w;x++){unsigned p=pixels[x];out[x]=genesis_dac_cram_to_argb(snapshot.cram[p&63],p>=128?2:p>=64?1:0);}
}
static int title_sky_planes(const GVDP *v){
 if((v->reg[4]&7)!=7||(v->reg[16]&3)!=1||v->vram[0x75*32]!=0x44)return 0;
 /* A solid sky row on B survives the scroll from the title to the mode menu;
  * its tile's pixels select CRAM 20, including during palette fades. The
  * guest lays the title out on different name-table rows when the intro is
  * skipped (the sky row moves from 11 to 3 and the vertical scroll
  * compensates), so any row of the 32-row plane may hold it. */
 for(unsigned r=0;r<32;r++){
  const uint8_t *sky=v->vram+0xe000+r*128;
  if(word(sky)==0x2075&&word(sky+62)==0x2075&&word(sky+126)==0x2075)return 1;
 }
 return 0;
}
int db2_video_title_active(const GVDP *v){
 return (v->reg[1]&0x40)&&world_scene==WORLD_NONE&&!briefing_scene&&title_sky_planes(v);
}
static const uint32_t *remove_parked_title_logo(const GVDP *v,const GVDPRasterRow *r,
                                               const uint32_t *native,uint32_t *clean){
 static const uint8_t zero=0;
 if(!r||!r->display||!db2_video_title_active(v)||!db2_wide_menu_parked_title_logo(v))return native;
 memcpy(clean,native,320*sizeof *clean);
 for(int x=0;x<320;x++){
  if(!r->sprite_owner[x]||!db2_wide_menu_title_logo_sprite(v,r->sprite_owner[x]-1u))continue;
  uint8_t index;
  gvdp_compose_span(&index,r->plane_a+x,r->plane_b+x,&zero,&zero,&zero,&zero,&zero,
                    1,r->backdrop,r->shadow_mode);
  clean[x]=genesis_dac_cram_to_argb(v->cram[index&63],index>=128?2:index>=64?1:0);
 }
 return clean;
}
static void scanline(const GVDP *v,int y,int nw,uint32_t *out,int w,GameVideoNativeRow native,void *ctx,const GVDPRasterRow *r){
 background=genesis_dac_cram_to_argb(v->cram[v->reg[7]&63],0);
 int supported=nw==320&&w>=320&&w<=LIMIT&&r&&r->display&&world_raster_enabled();
 if(y==0){ready=0;rejected=0;frame_reasons=0;memset(native_valid,0,sizeof native_valid);memset(hud_edges,0,sizeof hud_edges);if(supported)capture(v);
  if(moving_frames)--moving_frames;
  if(ready&&scroll_known&&(disp_x!=scroll_x||disp_y!=scroll_y))moving_frames=SCROLL_SETTLE_FRAMES;
  if(!ready)moving_frames=0;
  scroll_known=ready;scroll_x=disp_x;scroll_y=disp_y;
 }
 const uint32_t *p=native(ctx);
 uint32_t cleaned[320];
 if(nw==320&&world_scene==WORLD_NONE)p=remove_parked_title_logo(v,r,p,cleaned);
 if(y>=0&&y<224&&nw==320){memcpy(native_image[y],p,sizeof native_image[y]);native_valid[y]=1;}
 if(y==0){memset(wide_valid,0,sizeof wide_valid);wide_w=w;
  title_sky=nw==320&&db2_video_title_active(v);
  if(title_sky)sky_base=genesis_dac_cram_to_argb(v->cram[20],0);
  db2_wide_menu_set_sprite_gate(title_sky);
  db2_wide_menu_frame(v);db2_menu_labels_frame(v);}
 if(y>=0&&y<224&&nw==320&&w>320&&w<=LIMIT&&w==wide_w){
  uint32_t *row=wide_image[y];int left=(w-320)/2;
  for(int x=0;x<w;x++)row[x]=background;
  memcpy(row+left,p,320*sizeof *p);
  db2_wide_menu_row(v,y,row,w,left,1);
  db2_bio_overlay_row(v,y,row,w,left);
  db2_menu_labels_row(v,y,row,w,left);
  wide_valid[y]=1;
 }
 if(supported&&ready){
  if(memcmp(ram,g_ram,sizeof ram))capture(v);else memcpy(&snapshot,v,sizeof snapshot);
  classify_sprites();
 }
 if(trace_frames&&(y==100)&&world_scene!=WORLD_BATTLE){
  static unsigned probe_n;if(probe_n++%30==0){
  unsigned hb=((unsigned)v->reg[13]&63)<<10;int wt=(v->reg[16]&3)==1?64:(v->reg[16]&3)==3?128:32;int ht=((v->reg[16]>>4)&3)==1?64:((v->reg[16]>>4)&3)==3?128:32;
  char colsA[129]={0},colsB[129]={0};
  for(int pl=0;pl<2;pl++){unsigned base=pl?((unsigned)v->reg[4]&7)<<13:((unsigned)v->reg[2]&0x38)<<10;char *cs=pl?colsB:colsA;
   for(int cx=0;cx<wt&&cx<128;cx++){int n=0;for(int cy=0;cy<ht;cy++){unsigned a2=(base+((unsigned)(cy*wt+cx)<<1))&0xffff;if(v->vram[a2]|v->vram[(a2+1)&0xffff])n++;}cs[cx]=n==0?'.':n<ht/2?'+':'#';}}
  fprintf(stderr,"[db2-introprobe] size=%dx%d hsA=%d hsB=%d vsA=%d vsB=%d hmode=%d\n A:%s\n B:%s\n",wt,ht,(int)(((v->vram[hb]<<8)|v->vram[hb+1])&1023),(int)(((v->vram[hb+2]<<8)|v->vram[hb+3])&1023),v->vsram[0]&1023,v->vsram[1]&1023,v->reg[11]&3,colsA,colsB);}
 }
 if(!supported||!ready||rejected){if(!supported)frame_reasons|=32;rejected=1;
  if(y>=0&&y<224&&wide_valid[y]&&w==wide_w)memcpy(out,wide_image[y],(size_t)w*sizeof *out);
  else{
   for(int x=0;x<w;x++)out[x]=background;
   if(w>=nw)memcpy(out+(w-nw)/2,p,(size_t)nw*sizeof *p);
   if(w>=nw&&nw==320){db2_bio_overlay_row(v,y,out,w,(w-nw)/2);db2_menu_labels_row(v,y,out,w,(w-nw)/2);}
  }
  return;}
 world_row(y,w,r,out);
}
static void complete(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,int nh){
if(!pixels||w<320||w>LIMIT||h>MAX_HEIGHT||stride<w||top<0||top+nh>h)return;
 for(int y=0;y<h;y++)if(y<top||y>=top+nh){if(ready)world_row(y-top,w,NULL,pixels+(size_t)y*stride);else{uint32_t *o=pixels+(size_t)y*stride;for(int x=0;x<w;x++)o[x]=background;if(w>320)db2_wide_menu_row(v,y-top,o,w,(w-320)/2,0);}}
}
static void trace_totals(void){
 if(trace_enabled&&!((trace_ok_frames+trace_fallback_frames)%600))fprintf(stderr,"[db2-video] totals accepted=%u fallback=%u r01=%u r02=%u r04=%u r08=%u r10=%u r20=%u\n",trace_ok_frames,trace_fallback_frames,trace_reason_frames[0],trace_reason_frames[1],trace_reason_frames[2],trace_reason_frames[3],trace_reason_frames[4],trace_reason_frames[5]);
}
/* Percentage of margin pixels that carry real plane/sprite art this frame. */
static int margin_art(int w){
 int margin=(w-320)/2;unsigned art=0,total=0;
 if(margin<=0||w!=wide_w)return 0;
 for(int y=0;y<224;y++)if(wide_valid[y])for(int x=0;x<margin;x++){total+=2;art+=(wide_image[y][x]!=background)+(wide_image[y][w-1-x]!=background);}
 return total?(int)(art*100u/total):0;
}
/* A flat, confetti-dotted backdrop touching both native edges (stage
 * briefing): the same colour fills at least 70% of the 12 outer pixels per side. */
static int flat_edges(void){
 uint32_t c=0;unsigned best=0;
 for(int y=0;y<224;y++)if(!native_valid[y])return 0;
 for(int y=0;y<224;y+=7){uint32_t k=native_image[y][2];unsigned n=0;
  for(int yy=0;yy<224;yy++)for(int x=0;x<12;x++)n+=native_image[yy][x]==k;
  if(n>best){best=n;c=k;}}
 unsigned l=0,r=0;
 for(int y=0;y<224;y++)for(int x=0;x<12;x++){l+=native_image[y][x]==c;r+=native_image[y][319-x]==c;}
 return l*100u>=224u*12u*70u&&r*100u>=224u*12u*70u;
}
static int extend_solid_backdrop(uint32_t *pixels,int stride,int w,int h,int top){
 int left=(w-320)/2;
 for(int y=0;y<224;y++){
  if(!native_valid[y])return 0;
  const uint32_t *native=native_image[y];
  for(int x=0;x<8;x++)
   if(native[x]!=native[0]||native[319-x]!=native[0])return 0;
 }
 /* The logos' backdrop comes from the native image during the whole fade.
  * Use its rendered color on every row, including raster palette changes. */
 for(int y=0;y<h;y++){
  int source=y-top;
  uint32_t *row=pixels+(size_t)y*stride;
  uint32_t color=native_image[source<0?0:source>=224?223:source][0];
  if(source<0||source>=224){for(int x=0;x<w;x++)row[x]=color;}
  else{
   for(int x=0;x<left;x++)row[x]=color;
   for(int x=left+320;x<w;x++)row[x]=color;
  }
 }
 return 1;
}
static int special_stage(void){
 return word(g_ram+0x278)==0xff&&word(g_ram+0x27a)==0x292&&g_ram[0x292]==6;
}
static int dialogue_over_black(void){
 if(world_scene!=WORLD_NONE||briefing_scene||title_sky||!native_valid[0]||
    native_image[0][0]!=0xff000000u||native_image[0][319]!=0xff000000u){
  dialogue_hold=0;return 0;
 }
 /* Sega Channel Special sets its stage descriptor before the heading appears. */
 if(special_stage())return 1;
 /* The animated introductions use a single parchment box near the top.
  * A stretch seam through that box also stretches the heading below it. */
 for(int y=18;y<36;y++){
  if(!native_valid[y])continue;
  const uint32_t *row=native_image[y];
  uint32_t fill=row[274];
  if(row[7]!=0xff000000u||row[312]!=0xff000000u||fill==0xff000000u)continue;
  int matching=0;
  for(int x=22;x<298;x++)matching+=row[x]==fill;
  if(matching>=256){dialogue_hold=120;return 1;}
 }
 if(dialogue_hold){--dialogue_hold;return 1;}
 return 0;
}
static uint32_t patterned_pixel(int x,int y){
 int column=(x+(((unsigned)y&16u)?80:0))%112;
 if(column<0)column+=112;
 return native_image[(unsigned)y&15u][column];
}
static int extend_menu_pattern(uint32_t *pixels,int stride,int w,int h,int top){
 enum { PERIOD=112, EDGE=12 };
 int has_pattern=0;
 for(int y=0;y<224;y++)if(!native_valid[y])return 0;
 for(int y=0;y<16;y++)for(int x=0;x<320-PERIOD;x++){
  if(native_image[y][x]!=native_image[y][x+PERIOD])return 0;
  if(native_image[y][x]!=native_image[y][0])has_pattern=1;
 }
 if(!has_pattern)return 0;
 unsigned matches=0;
 for(int y=0;y<224;y++)for(int x=0;x<EDGE;x++){
  matches+=native_image[y][x]==patterned_pixel(x,y);
  matches+=native_image[y][319-x]==patterned_pixel(319-x,y);
 }
 if(matches<224u*EDGE*2*9/10)return 0;
 int left=(w-320)/2;
 for(int y=0;y<h;y++){
  int source=y-top;
  uint32_t *row=pixels+(size_t)y*stride;
  for(int x=0;x<w;x++)
   if(source<0||source>=224||x<left||x>=left+320)
    row[x]=patterned_pixel(x-left,source);
 }
 return 1;
}
static int framed_submenu(void){
 int frame_edges=0;
 for(int edge=0;edge<2;edge++){
  int first=edge?198:14;
  for(int y=first;y<first+14;y++){
   if(!native_valid[y])continue;
   const uint32_t *row=native_image[y];
   unsigned matches=0;
   for(int x=20;x<300;x++)matches+=row[x]==row[160];
   if(matches>=274){++frame_edges;break;}
  }
 }
 return frame_edges==2;
}
static void flatten_versus_panel(uint32_t *pixels,int stride,int left,int h,int top){
 const uint32_t green=0xff005731u;
 /* Both Versus selectors share a yellow frame, two greens and an olive
  * lower panel. The other framed menus use different interior palettes. */
 if(!native_valid[22]||!native_valid[27]||!native_valid[140]||
    native_image[22][100]!=0xffffff00u||native_image[27][27]!=0xff003131u||
    native_image[27][287]!=green||native_image[140][27]!=0xff575731u)return;
 for(int y=24;y<200;y++){
  if(top+y<0||top+y>=h)continue;
  uint32_t *row=pixels+(size_t)(top+y)*stride+left;
  for(int x=24;x<296;x++){
   switch(row[x]){
   case 0xff003131u:case 0xff575731u:case 0xff777757u:case 0xff929257u:
    row[x]=green;break;
   }
  }
 }
}
static int reflect_edge(int distance,int size){
 int phase=distance%(size*2);
 return phase<size?phase:size*2-1-phase;
}
static void extend_missing_menu_background(uint32_t *pixels,int stride,int w,int h,int top){
 int left=(w-320)/2;
 /* Some menus author the landscape on the same plane as their controls.
  * Continue its outer strips where the separate backing plane is empty. */
 for(int y=0;y<h;y++){
  int source=y-top,vertical=source<0||source>=224;
  int sy=source<0?reflect_edge(-source-1,8):
         source>=224?223-reflect_edge(source-224,8):source;
  if(!native_valid[sy])continue;
  uint32_t *row=pixels+(size_t)y*stride;
  for(int x=0;x<w;x++){
   int sx=x-left;
   if(!vertical&&sx>=0&&sx<320)continue;
   if(row[x]!=background&&row[x]!=0xff000000u)continue;
   if(sx<0)sx=reflect_edge(-sx-1,16);
   else if(sx>=320)sx=319-reflect_edge(sx-320,16);
   row[x]=native_image[sy][sx];
  }
 }
}
static void fill_menu_margins(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,
                              Db2MenuMargins mode){
 db2_wide_menu_margins(v,mode);
 for(int y=0;y<h;y++){
  int source=y-top;
  db2_wide_menu_row(v,source,pixels+(size_t)y*stride,w,(w-320)/2,source>=0&&source<224);
 }
 db2_wide_menu_margins(v,DB2_MENU_PLANES);
 if(mode==DB2_MENU_BACKGROUND)extend_missing_menu_background(pixels,stride,w,h,top);
}
static uint32_t results_pattern(int x,int y){
 unsigned phase=(unsigned)y&63u;
 return native_image[phase<16?phase:phase+128][(unsigned)x&63u];
}
static int special_panorama(const GVDP *v){
 if(world_scene!=WORLD_BATTLE||ready||!special_stage()||
    (v->reg[4]&7)!=7||(v->reg[16]&0x33)!=1||(v->reg[11]&7)||
    (v->vsram[1]&1023))return 0;
 unsigned scroll=((unsigned)v->reg[13]&63)<<10;
 if(word(v->vram+scroll+2)&1023)return 0;
 /* The introductory landscape is authored on B, at zero scroll, in exactly
  * 40 by 28 cells. A holds dialogue and the actors are independent sprites. */
 const uint8_t *map=v->vram+0xe000;
 if(word(map)!=0x6075||word(map+2)!=0x6076||word(map+4)!=0x6075)return 0;
 for(int y=0;y<32;y++)for(int x=0;x<64;x++)
  if((word(map+2*(y*64+x))!=0)!=(y<28&&x<40))return 0;
 return 1;
}
static int briefing_planes(const GVDP *v){
 if((v->reg[2]&0x38)!=0x30||(v->reg[4]&7)!=7||(v->reg[16]&3)!=1)return 0;
 /* The Story route builds its briefing outside the tracked call pair.
  * Match its logo on A and the confetti backdrop on B independently. */
 const uint8_t *logo=v->vram+0xc000+3*128+4;
 const uint8_t *confetti=v->vram+0xe000;
 return word(logo)==0x87cb&&word(logo+2)==0x87cc&&word(logo+4)==0x87cd&&
        word(confetti)==0x87eb&&word(confetti+10)==0x87ec&&
        word(confetti+3*128+28)==0x87f3&&word(confetti+27*128+54)==0x87ee;
}
static int stage_picker_planes(const GVDP *v){
 if(!v||(v->reg[4]&7)!=7||(v->reg[16]&0x33)!=1)return 0;
 /* Classic stage selection keeps this frame and bank of backdrop tiles
  * through the fade. Its colors alone cannot identify the dark frames. */
 const uint8_t *map=v->vram+0xe000;
 return word(map)==0x607d&&word(map+2*(2*64+2))==0x6082&&
        word(map+2*(2*64+3))==0x6083&&word(map+2*(25*64+2))==0x7082;
}
static int extend_results_pattern(uint32_t *pixels,int stride,int w,int h,int top){
 for(int y=0;y<224;y++){
  if(!native_valid[y])return 0;
  for(int x=0;x<8;x++)
   if(native_image[y][x]!=results_pattern(x,y)||
      native_image[y][319-x]!=results_pattern(319-x,y))return 0;
 }
 int left=(w-320)/2;
 for(int y=0;y<h;y++){
  int source=y-top;
  uint32_t *row=pixels+(size_t)y*stride;
  for(int x=0;x<w;x++)
   if(source<0||source>=224||x<left||x>=left+320)
    row[x]=results_pattern(x-left,source);
 }
 return 1;
}
static void fade_menu_edges(uint32_t *pixels,int stride,int w,int h){
 int left=(w-320)/2;
 for(int y=0;y<h;y++){
  uint32_t *row=pixels+(size_t)y*stride;
  for(int x=0;x<left;x++)row[x]=background;
  for(int x=left+320;x<w;x++)row[x]=background;
  for(int i=0;i<16;i++){unsigned wgt=(unsigned)(i*i*(45-2*i));row[left+i]=db2_wide_menu_blend(row[left+i],background,wgt);row[left+319-i]=db2_wide_menu_blend(row[left+319-i],background,wgt);}
 }
}
static int story_intro_landscape(const GVDP *v){
 if(world_scene!=WORLD_NONE||briefing_scene||(v->reg[16]&0x33)!=1)return 0;
 unsigned a=((unsigned)v->reg[2]&0x38)<<10;
 unsigned b=((unsigned)v->reg[4]&7)<<13;
 for(int plane=0;plane<2;plane++){
  const uint8_t *map=v->vram+(plane?b:a);
  if(word(map)==0x6075&&word(map+2)==0x6076&&
     word(map+4)==0x6075&&word(map+6)==0x6076&&
     word(map+86)==0x6076&&word(map+88)==0&&word(map+28*128)==0)return 1;
 }
 return 0;
}
static void vertical_finish(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top){
 int bottom=h-(top+224);
 if(top<0||bottom<0)return;
 /* Results already have independently composed margins. Keep both upper
  * plates and the result box in their native positions over that background. */
 if(results_scene){
  if(v&&w>320&&!extend_results_pattern(pixels,stride,w,h,top))
   fill_menu_margins(v,pixels,stride,w,h,top,DB2_MENU_BACKGROUND);
  return;
 }
 if(v&&w>320&&story_intro_landscape(v)){
  fade_menu_edges(pixels,stride,w,h);
  return;
 }
 if(v&&w>320&&dialogue_over_black()){
  fill_menu_margins(v,pixels,stride,w,h,top,DB2_MENU_SPRITES);
  return;
 }
 if(v&&w>320&&special_panorama(v)){
  fill_menu_margins(v,pixels,stride,w,h,top,DB2_MENU_PANORAMA);
  return;
 }
 if(v&&w>320&&(briefing_scene||briefing_planes(v))){
  if(trace_enabled&&!(trace_seen&128)){
   fprintf(stderr,"[db2-video] briefing margins use native sprites\n");
   trace_seen|=128;
  }
  fill_menu_margins(v,pixels,stride,w,h,top,DB2_MENU_BRIEFING);
  return;
 }
 int stage_picker=world_scene==WORLD_NONE&&stage_picker_planes(v);
 if(world_scene==WORLD_NONE&&!briefing_scene&&!title_sky&&!stage_picker&&w>320&&
    extend_solid_backdrop(pixels,stride,w,h,top))return;
 /* The blue Options palette also matches the title's sky heuristic. The
  * full-period and edge checks identify the menu independently of color. */
 if(world_scene==WORLD_NONE&&!briefing_scene&&w>320&&
    extend_menu_pattern(pixels,stride,w,h,top))return;
 if(v&&w>320&&world_scene==WORLD_NONE&&!briefing_scene&&!title_sky&&(stage_picker||framed_submenu())){
  fill_menu_margins(v,pixels,stride,w,h,top,DB2_MENU_BACKGROUND);
  /* The silhouette reaches the right native column above the ground bands.
   * Extend the clear sky color there. For the ground, sample the unobstructed
   * strip left of the frame so the bands continue without stretching specks. */
  int left=(w-320)/2,right=left+320;
  flatten_versus_panel(pixels,stride,left,h,top);
  /* Keep the foliage texture from the background plane. Lower its edge
   * evenly, tapering only the last few pixels into the native image. */
  for(int y=top+127;y>=top&&y>=0;y--){
   if(y>=h)continue;
   uint32_t *row=pixels+(size_t)y*stride;
   for(int x=0;x<left;x++){
    int distance=left-x;
    int shift=distance>=8?4:distance/2;
    if(y>=shift)row[x]=pixels[(size_t)(y-shift)*stride+x];
   }
  }
  for(int y=0;y<h;y++){
   int line=y-top;
   int source=line<0?0:line>=224?223:line;
   if(!native_valid[source])continue;
   uint32_t *row=pixels+(size_t)y*stride;
   uint32_t color=source<128&&native_valid[0]?native_image[0][319]:native_image[source][7];
   int canopy=0;
   if(source>=72&&source<128&&native_valid[0]&&
      native_image[source][319]!=color){
    int depth=source-72;
    canopy=48*depth*(56-depth)/(56*56);
    canopy+=(int)(((unsigned)source*13u^((unsigned)source>>2))&3u);
   }
   for(int x=right;x<w;x++)
    row[x]=x-right<canopy?native_image[source][319]:color;
  }
  return;
 }
 if(v&&w>320&&!title_sky&&!db2_wide_menu_extended()&&flat_edges()){
  fill_menu_margins(v,pixels,stride,w,h,top,DB2_MENU_FLAT);
  return;
 }
 if(top<=0)return;
 if(title_sky&&native_valid[0]&&native_valid[223]){
  /* Anchor the native scene at the bottom. The added band is only sky;
   * native foreground and sprites leave the screen at the original edge. */
  for(int y=h-1;y>=top+bottom;y--)
   memcpy(pixels+(size_t)y*stride,pixels+(size_t)(y-bottom)*stride,(size_t)w*sizeof *pixels);
  for(int y=0;y<top+bottom;y++)
   for(int x=0;x<w;x++)pixels[(size_t)y*stride+x]=sky_base;
  return;
 }
 /* Intro (art authored past the 4:3 window): like the DB1 intro, the sides
  * stay backdrop and the original left/right borders fade into it over
  * sixteen native pixels; the continued art is kept only above and below. */
 /* Map scenes with nothing authored outside 4:3 (attract demo, cutscenes):
  * the same treatment, so stray off-screen sprites never float on black. */
 if(margin_art(w)>=30)return;
 fade_menu_edges(pixels,stride,w,h);
}

static void frame_complete(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,int nh){
 if(!pixels||w<320||w>LIMIT||stride<w||top<0||nh!=224||top+nh>h)return;
 /* Leaving a native modal screen (pause, Info View): the first frames can still
  * be rejected while the guest restores its camera state; the frame from before
  * the modal is re-presented for those instead of dropping the wide view. */
 {static int prev_modal;int modal=(g_ram[0x286]&0x88)!=0;if(prev_modal&&!modal)held_misses=0;prev_modal=modal;}
 if(trace_frames&&v){
  unsigned hb=((unsigned)snapshot.reg[13]&63)<<10;
  fprintf(stderr,"[db2-frame] g286=%02X n=%u ok=%d reasons=%02X ready=%d scene=%d ramscroll=%d,%d vdp_hs=%d,%d vs=%d,%d ring=%u,%u cell=%u,%u offset=%d,%d\n",g_ram[0x286],trace_ok_frames+trace_fallback_frames,!rejected&&ready,frame_reasons,ready,world_scene,(int)(int16_t)word(g_ram+0x2bf8),(int)(int16_t)word(g_ram+0x2bfa),(int)(word(snapshot.vram+hb)&1023),(int)(word(snapshot.vram+hb+2)&1023),snapshot.vsram[0]&1023,snapshot.vsram[1]&1023,camera.ring_x,camera.ring_y,camera.cell_x,camera.cell_y,off_x,off_y);
 }
 if(!rejected&&ready){
  for(int y=0;y<h;y++)memcpy(held[y],pixels+(size_t)y*stride,(size_t)w*sizeof *pixels);
  held_w=w;held_h=h;held_valid=1;held_misses=0;
  if(plane_w&&plane_h){kx=model_x+off_x-disp_x;ky=model_y+off_y-disp_y;have_k=1;}
  ++trace_ok_frames;trace_totals();
  if(trace_enabled&&!(trace_seen&64)){
   Db2VisorLayout layout={0};db2_visor_layout(w,visor_offset,&layout);
   fprintf(stderr,"[db2-video] accepted canvas=%dx%d scene=%s terrain=%c actors=%u frame=%d,%d,%d,%d\n",w,h,world_scene==WORLD_DEMO?"demo":"battle",world_is_a?'A':'B',count,layout.frame.x,layout.frame.y,layout.frame.w,layout.frame.h);trace_seen|=64;
  }
  return;
 }
 ++trace_fallback_frames;if(trace_enabled)for(unsigned b=0;b<6;b++)if(frame_reasons&(1u<<b))++trace_reason_frames[b];
 trace_totals();
 if(trace_enabled&&trace_budget){
  unsigned pa=v?v->reg[2]:0,pb=v?v->reg[4]:0;
  unsigned signature=((unsigned)world_scene<<24)|((unsigned)(g_ram[0x286]&0x88)<<16)|(pa<<8)|pb;
  if((frame_reasons&~trace_seen)||signature!=trace_signature){
   fprintf(stderr,"[db2-video] native fallback reasons=%02X scene=%d flags=%02X planes=%02X/%02X ready=%d actors=%u\n",frame_reasons,world_scene,g_ram[0x286],pa,pb,ready,count);
   --trace_budget;trace_signature=signature;trace_seen|=frame_reasons;
  }
 }
 int recovering_battle=world_scene==WORLD_BATTLE&&!(g_ram[0x286]&0x88);
 int pausing_battle=(world_scene==WORLD_BATTLE||world_scene==WORLD_VERSUS)&&
                    (pause_active||native_pause())&&
                    !memcmp(ram+0x2a56,g_ram+0x2a56,12);
 if(held_valid&&held_w==w&&held_h==h&&(recovering_battle||pausing_battle)&&++held_misses<=HOLD_FRAMES+2){
  for(int y=0;y<h;y++)memcpy(pixels+(size_t)y*stride,held[y],(size_t)w*sizeof *pixels);
  return;
 }
 if(!(g_ram[0x286]&0x88)&&held_misses>HOLD_FRAMES+2)held_valid=0;
 /* All rows are restored together, including rows drawn before an unknown
  * effect appeared. No callback/provider/runner pointer is retained. */
 for(int y=0;y<h;y++){
  uint32_t *out=pixels+(size_t)y*stride;int source=y-top;
  if(source>=0&&source<224&&wide_valid[source]&&w==wide_w){memcpy(out,wide_image[source],(size_t)w*sizeof *out);continue;}
  for(int x=0;x<w;x++)out[x]=background;
  if(source>=0&&source<224&&native_valid[source]){memcpy(out+(w-320)/2,native_image[source],sizeof native_image[source]);{db2_bio_overlay_row(v,source,out,w,(w-320)/2);db2_menu_labels_row(v,source,out,w,(w-320)/2);}}
  else if(w>320)db2_wide_menu_row(v,source,out,w,(w-320)/2,0);
 }
 vertical_finish(v,pixels,stride,w,h,top);
}
static void overlay_scanline(const GVDP *v,int line,uint32_t *out,int width){
 db2_bio_overlay_scanline(v,line,out,width);
 if(line==0)db2_menu_labels_frame(v);
 db2_menu_labels_row(v,line,out,width,width>=320?(width-320)/2:0);
}
const GameVideo db2_video={.configure=configure,.enabled=enabled,.width=width,.raster_enabled=raster_enabled,.scanline_presented=scanline,.integer_scale_supported=1,.canvas_geometry=geometry,.canvas_complete=complete,.frame_complete=frame_complete,.overlay_scanline=overlay_scanline};
