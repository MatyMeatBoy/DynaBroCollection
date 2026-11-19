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
enum { WORLD_NONE, WORLD_BATTLE, WORLD_DEMO };
static int world_scene,ready,canvas_top,canvas_height=224;
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
/* Displayed-vs-modelled terrain alignment. During held camera pans the guest
 * advances the VDP scroll in 16-pixel steps while the RAM camera cell only
 * moves every 32 pixels, so half of the frames show the plane one sub-step
 * ahead of the RAM model. The displayed origin is VDP scroll + k (k fixed while
 * the nametable ring is coherent); off_x/off_y re-align the reconstructed
 * terrain with what the guest actually shows. Every widened row is still
 * verified against the native plane, so a wrong guess only rejects a frame. */
static int have_k,kx,ky,off_x,off_y,model_x,model_y,disp_x,disp_y,plane_w,plane_h,heal_pending,heal_dx,heal_dy;
static int visor_offset;
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
int db2_video_visor_requested(void){return visor_offset;}
static int plane_layout(const GVDP *v){
 unsigned a=((unsigned)v->reg[2]&0x38)<<10,b=((unsigned)v->reg[4]&7)<<13;
 if(a==0xc000&&b==0xe000){world_is_a=1;return 1;}
 if(a==0xe000&&b==0xc000){world_is_a=0;return 1;}
 return 0;
}
static unsigned word(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
void db2_video_reset(void){db2_bio_overlay_reset();world_scene=ready=rejected=0;count=0;effect_count=0;canvas_top=0;canvas_height=224;memset(native_valid,0,sizeof native_valid);const char *t=getenv("DYNA_DB2_VIDEO_TRACE");trace_enabled=t&&(!strcmp(t,"1")||!strcmp(t,"2"));trace_frames=t&&!strcmp(t,"2");frame_reasons=trace_seen=0;trace_signature=~0u;trace_budget=32;producer_epoch=producer_dirty_since_epoch=0;have_k=off_x=off_y=heal_pending=0;held_valid=0;held_misses=0;}
void db2_video_observe(unsigned pc){
 if(pc==0x3bb0)world_scene=WORLD_BATTLE;
 else if(pc==0xf794)world_scene=WORLD_DEMO;
 else if(pc==0x3bb4||pc==0x734e||pc==0xf728)world_scene=WORLD_NONE;
 else if(pc==0x146c0)producer_dirty_since_epoch=1;
 else if(pc==0x15220){if(producer_dirty_since_epoch)producer_epoch++;producer_dirty_since_epoch=0;}
}
unsigned db2_video_producer_epoch(void){return producer_epoch;}
int db2_video_producer_dirty(void){return producer_dirty_since_epoch;}
static int configure(const char *s){return dyna_viewport_configure(&view,s);}
static int enabled(void){return view.enabled;}
static int width(int dw,int dh,int nw,int nh){(void)nw;canvas_top=0;canvas_height=nh;return dyna_viewport_width(&view,dw,dh,nh);}
static int raster_enabled(const GVDP *v){(void)v;return world_scene!=WORLD_NONE&&!(g_ram[0x286]&0x88);}
static int geometry(int dw,int dh,int nw,int nh,int *w,int *h){
 if(!view.enabled||nw!=320||nh!=224)return 0;
 int scale=game_video_integer_canvas(dw,dh,dyna_viewport_width(&view,dw,dh,nh),nh,w,h);
 if(!scale||*w>LIMIT||*h>MAX_HEIGHT)return 0;
 canvas_top=(*h-nh)/2;canvas_height=*h;return scale;
}
static void capture(const GVDP *v){
 if(trace_enabled&&producer_dirty_since_epoch)
  fprintf(stderr,"[db2-video] capture during dirty producer epoch=%u pc=%06X\n",producer_epoch,g_cpu.PC);
 memcpy(ram,g_ram,sizeof ram);memcpy(&snapshot,v,sizeof snapshot);
 Db2WorldContext c={g_rom,2621440,ram,sizeof ram,snapshot.vram,sizeof snapshot.vram};
 ready=db2_world_read_camera(&c,&camera)&&plane_layout(v)&&
       word(ram+0x2a5a)==0xe036&&word(ram+0x2a5c)==0xe004&&
       word(ram+0x2a5e)==216&&word(ram+0x2a60)==16;
 off_x=off_y=0;
 if(ready){
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
   if(mode==3||mode==5)return 1;
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
  for(int x=0;blank&&x<216;x++)if(native_terrain[x]||native_ui[x]||native_image[y][x]!=background)blank=0;
  if(blank)for(int x=0;x<w;x++)terrain[x]=0;
  for(int x=0;x<216;x++){
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
      off_x=sx+dx;off_y=sy+dy;int q=0;
      for(;q<216;q++)if(terrain_pixel(&c,q,y)!=native_terrain[q]&&!((native_ui[q]&0x80)&&(native_ui[q]&15)))break;
      if(q==216){found++;fx=off_x;fy=off_y;}
     }
     off_x=sx;off_y=sy;
     if(found==1){heal_pending=1;heal_dx=fx;heal_dy=fy;}
    }
    rejected=1;frame_reasons|=16;}
   if(dx>=0&&dx<w)ui[dx]=native_ui[x];
  }
  memcpy(ui+layout.hud_left,native_ui+216,104);
 }
 size_t ei=effect_count;
 for(unsigned remaining=count;remaining;){const Db2Actor *ac=actors+--remaining;
  while(ei&&(effects[ei-1].depth<ac->depth||(effects[ei-1].depth==ac->depth&&effects[ei-1].rank>rank[ac->id])))effect_row(effects+--ei,y,w,shift,s,o,hi);
  for(unsigned part=ac->component_count;part;){const Db2ActorComponent *p=ac->part+--part;
   int xx=ac->x+p->dx+shift,yy=ac->y+p->dy;if(y<yy||y>=yy+32)continue;
   for(int px=0;px<32;px++){uint8_t color;db2_actor_pixel(p,(unsigned)px,(unsigned)(y-yy),&color);put(s,o,hi,xx+px,w,color,p->flags);}
  }
  Db2WorldMarker m[6];unsigned n=db2_world_markers(ram[0x2a89],ram[0x5f58+48*ac->id+0x28],ac->x,ac->y,m);
  while(n){const Db2WorldMarker *p=m+--n;if(y<p->y||y>=p->y+8)continue;for(int px=0;px<8;px++)put(s,o,hi,p->x+px+shift,w,tile_pixel(p->attribute,0,px,y-p->y),p->attribute);}
 }
 while(ei)effect_row(effects+--ei,y,w,shift,s,o,hi);
 if(r){
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
    unknown_traced=1;kind=x>=216?2:1;
   }
   if(kind==3)continue;
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
static void scanline(const GVDP *v,int y,int nw,uint32_t *out,int w,GameVideoNativeRow native,void *ctx,const GVDPRasterRow *r){
 background=genesis_dac_cram_to_argb(v->cram[v->reg[7]&63],0);
 int supported=nw==320&&w>=320&&w<=LIMIT&&r&&r->display&&raster_enabled(v);
 if(y==0){ready=0;rejected=0;frame_reasons=0;memset(native_valid,0,sizeof native_valid);if(supported)capture(v);}
 const uint32_t *p=native(ctx);
 if(y>=0&&y<224&&nw==320){memcpy(native_image[y],p,sizeof native_image[y]);native_valid[y]=1;}
 if(y==0){memset(wide_valid,0,sizeof wide_valid);wide_w=w;db2_wide_menu_frame(v);db2_menu_labels_frame(v);}
 if(y>=0&&y<224&&nw==320&&w>320&&w<=LIMIT&&w==wide_w){
  uint32_t *row=wide_image[y];int left=(w-320)/2;
  for(int x=0;x<w;x++)row[x]=background;
  memcpy(row+left,p,320*sizeof *p);
  db2_wide_menu_row(v,y,row,w,left,1);
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
  else{for(int x=0;x<w;x++)out[x]=background;if(w>=nw)memcpy(out+(w-nw)/2,p,(size_t)nw*sizeof *p);}
  if(w>=nw&&nw==320){db2_bio_overlay_row(v,y,out,w,(w-nw)/2);db2_menu_labels_row(v,y,out,w,(w-nw)/2);}
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
/* DB1-style panel stretch for menu screens whose art is exactly 4:3: find a
 * pixel column near each side that is horizontally flat on nearly every line
 * (a panel interior or frame run), move everything outside it outward by the
 * margin and repeat that column across the gap. The lettering/cursor between
 * the two columns keep their exact native pixels and position. */
static int flat_rows(int x,int dir){
 int n=0;for(int y=0;y<224;y++)n+=native_image[y][x]==native_image[y][x+dir];
 return n;
}
/* Percentage of margin pixels that carry real plane/sprite art this frame. */
static int margin_art(int w){
 int margin=(w-320)/2;unsigned art=0,total=0;
 if(margin<=0||w!=wide_w)return 0;
 for(int y=0;y<224;y++)if(wide_valid[y])for(int x=0;x<margin;x++){total+=2;art+=(wide_image[y][x]!=background)+(wide_image[y][w-1-x]!=background);}
 return total?(int)(art*100u/total):0;
}
static int same_rows(int x,int d){
 int n=0;for(int y=0;y<224;y++)n+=native_image[y][x]==native_image[y][x+d];
 return n;
}
static int panel_stretch(uint32_t *pixels,int stride,int w,int top){
 int margin=(w-320)/2;
 if(margin<=0)return 0;
 for(int y=0;y<224;y++)if(!native_valid[y])return 0;
 int xl=-1,xr=-1;
 /* Sticky: while the columns chosen for this screen stay valid, keep them,
  * so an animated background never makes the layout jump between frames. */
 static int last_xl=-1,last_xr=-1;
 if(last_xl>=0&&flat_rows(last_xl,1)>=150&&same_rows(last_xl,8)>=150&&
    flat_rows(last_xr,-1)>=150&&same_rows(last_xr,-8)>=150){xl=last_xl;xr=last_xr;}
 else{
 last_xl=last_xr=-1;
 /* Screens whose own planes already continue past 4:3 (main menu, intro)
  * keep that real art instead. */
 if(margin_art(w)>=30)return 0;
 /* The repeated column must be a wide interior (same colour eight pixels
  * further in), with a frame (busy columns) between it and the screen edge. */
 int busy_l=0,busy_r=0;
 for(int x=4;x<=64&&xl<0;x++){
  if(flat_rows(x,1)<112)busy_l=1;
  if(busy_l&&flat_rows(x,1)>=168&&same_rows(x,8)>=168)xl=x;
 }
 for(int x=315;x>=255&&xr<0;x--){
  if(flat_rows(x,-1)<112)busy_r=1;
  if(busy_r&&flat_rows(x,-1)>=168&&same_rows(x,-8)>=168)xr=x;
 }
 if(xl<16||xr<0||xr>303||xr-xl<160)return 0;
 last_xl=xl;last_xr=xr;
 }
 for(int y=0;y<224;y++){
  uint32_t *row=pixels+(size_t)(y+top)*stride;const uint32_t *n=native_image[y];
  /* One column for every line: the gap is that column's colour, never a
   * copied block (which would echo borders, cursors or HUD pieces). A line
   * where that column touches an outline/shadow (not wide interior) takes the
   * colour of the nearest interior line instead. */
  int a=xl,b=xr;
  uint32_t gl=n[xl],gr=n[xr];
  if(!(n[xl]==n[xl+1]&&n[xl]==n[xl+8]))for(int d=1;d<224;d++){
   int yy=y-d;if(yy>=0){const uint32_t *m=native_image[yy];if(m[xl]==m[xl+1]&&m[xl]==m[xl+8]){gl=m[xl];break;}}
   yy=y+d;if(yy<224){const uint32_t *m=native_image[yy];if(m[xl]==m[xl+1]&&m[xl]==m[xl+8]){gl=m[xl];break;}}
  }
  if(!(n[xr]==n[xr-1]&&n[xr]==n[xr-8]))for(int d=1;d<224;d++){
   int yy=y-d;if(yy>=0){const uint32_t *m=native_image[yy];if(m[xr]==m[xr-1]&&m[xr]==m[xr-8]){gr=m[xr];break;}}
   yy=y+d;if(yy<224){const uint32_t *m=native_image[yy];if(m[xr]==m[xr-1]&&m[xr]==m[xr-8]){gr=m[xr];break;}}
  }
  for(int dx=0;dx<w;dx++){
   int x=dx-margin;uint32_t c;
   if(x<a+1-margin)c=n[x+margin];
   else if(x<a+1)c=gl;
   else if(x<b)c=n[x];
   else if(x<b+margin)c=gr;
   else c=n[x-margin];
   row[dx]=c;
  }
 }
 return 1;
}
/* Non-battle frames: what goes above/below the 224 native lines.
 * A plain sky on the first native line (title/main menu beach) anchors the
 * image to the bottom of the canvas and continues that sky upward, as DB1's
 * title does; otherwise rows without art fade the native edge into the
 * backdrop over eight lines (DB1 intro fade, vertical). */
static int sky_row(const uint32_t *row){
 uint32_t c=row[160];unsigned same=0;
 for(int x=0;x<320;x++)same+=row[x]==c;
 unsigned r=(c>>16)&255,g=(c>>8)&255,b=c&255;
 return same>=280&&b>=96&&b>=r&&r+g+b>=200;
}
static void vertical_finish(uint32_t *pixels,int stride,int w,int h,int top){
 int bottom=h-(top+224);
 if(top<0||bottom<0)return;
 if(!db2_wide_menu_extended()&&panel_stretch(pixels,stride,w,top)){
  /* Lines above/below continue the stretched first/last line. */
  for(int y=0;y<top;y++)memcpy(pixels+(size_t)y*stride,pixels+(size_t)top*stride,(size_t)w*sizeof *pixels);
  for(int y=top+224;y<h;y++)memcpy(pixels+(size_t)y*stride,pixels+(size_t)(top+223)*stride,(size_t)w*sizeof *pixels);
  return;
 }
 if(top<=0)return;
 if(native_valid[0]&&native_valid[223]&&sky_row(native_image[0])){
  for(int y=h-1;y>=top+bottom;y--)memcpy(pixels+(size_t)y*stride,pixels+(size_t)(y-bottom)*stride,(size_t)w*sizeof *pixels);
  for(int y=0;y<top+bottom;y++)memcpy(pixels+(size_t)y*stride,pixels+(size_t)(top+bottom)*stride,(size_t)w*sizeof *pixels);
  return;
 }
 /* Intro (art authored past the 4:3 window): like the DB1 intro, the sides
  * stay backdrop and the original left/right borders fade into it over
  * sixteen native pixels; the continued art is kept only above and below. */
 /* Map scenes with nothing authored outside 4:3 (attract demo, cutscenes):
  * the same treatment, so stray off-screen sprites never float on black. */
 if(!db2_wide_menu_extended()&&margin_art(w)>=30)return;
 int left=(w-320)/2;
 for(int y=0;y<h;y++){
  uint32_t *row=pixels+(size_t)y*stride;
  for(int x=0;x<left;x++)row[x]=background;
  for(int x=left+320;x<w;x++)row[x]=background;
  for(int i=0;i<16;i++){unsigned wgt=(unsigned)(i*i*(45-2*i));row[left+i]=db2_wide_menu_blend(row[left+i],background,wgt);row[left+319-i]=db2_wide_menu_blend(row[left+319-i],background,wgt);}
 }
}
static void frame_complete(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,int nh){
 if(!pixels||w<320||w>LIMIT||stride<w||top<0||nh!=224||top+nh>h)return;
 if(trace_frames&&v){
  unsigned hb=((unsigned)snapshot.reg[13]&63)<<10;
  fprintf(stderr,"[db2-frame] n=%u ok=%d reasons=%02X ready=%d scene=%d ramscroll=%d,%d vdp_hs=%d,%d vs=%d,%d ring=%u,%u cell=%u,%u\n",trace_ok_frames+trace_fallback_frames,!rejected&&ready,frame_reasons,ready,world_scene,(int)(int16_t)word(g_ram+0x2bf8),(int)(int16_t)word(g_ram+0x2bfa),(int)(word(snapshot.vram+hb)&1023),(int)(word(snapshot.vram+hb+2)&1023),snapshot.vsram[0]&1023,snapshot.vsram[1]&1023,camera.ring_x,camera.ring_y,camera.cell_x,camera.cell_y,off_x,off_y);
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
 if(held_valid&&held_w==w&&held_h==h&&world_scene==WORLD_BATTLE&&++held_misses<=HOLD_FRAMES){
  for(int y=0;y<h;y++)memcpy(pixels+(size_t)y*stride,held[y],(size_t)w*sizeof *pixels);
  return;
 }
 held_valid=0;
 /* All rows are restored together, including rows drawn before an unknown
  * effect appeared. No callback/provider/runner pointer is retained. */
 for(int y=0;y<h;y++){
  uint32_t *out=pixels+(size_t)y*stride;int source=y-top;
  if(source>=0&&source<224&&wide_valid[source]&&w==wide_w){memcpy(out,wide_image[source],(size_t)w*sizeof *out);{db2_bio_overlay_row(v,source,out,w,(w-320)/2);db2_menu_labels_row(v,source,out,w,(w-320)/2);}continue;}
  for(int x=0;x<w;x++)out[x]=background;
  if(source>=0&&source<224&&native_valid[source]){memcpy(out+(w-320)/2,native_image[source],sizeof native_image[source]);{db2_bio_overlay_row(v,source,out,w,(w-320)/2);db2_menu_labels_row(v,source,out,w,(w-320)/2);}}
  else if(w>320)db2_wide_menu_row(v,source,out,w,(w-320)/2,0);
 }
 vertical_finish(pixels,stride,w,h,top);
}
static void overlay_scanline(const GVDP *v,int line,uint32_t *out,int width){
 db2_bio_overlay_scanline(v,line,out,width);
 if(line==0)db2_menu_labels_frame(v);
 db2_menu_labels_row(v,line,out,width,width>=320?(width-320)/2:0);
}
const GameVideo db2_video={.configure=configure,.enabled=enabled,.width=width,.raster_enabled=raster_enabled,.scanline_presented=scanline,.integer_scale_supported=1,.canvas_geometry=geometry,.canvas_complete=complete,.frame_complete=frame_complete,.overlay_scanline=overlay_scanline};
