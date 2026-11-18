#include "db2_video.h"
#include "db2_world.h"
#include "db2_actors.h"
#include "db2_markers.h"
#include "db2_effects.h"
#include "db2_visor.h"
#include "db2_bio_overlay.h"
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
static uint8_t native_valid[224],owner_kind[80];
static int rejected;
static int world_is_a;
static int visor_offset;
static int trace_enabled;
static unsigned frame_reasons,trace_seen,trace_signature,trace_budget;
static unsigned trace_ok_frames,trace_fallback_frames,trace_reason_frames[6];
/* Diagnostic-only producer epoch counter (evidence/db2-visor-research/coherence.md
 * step 1): 0x146C0 is the cache/gather producer's dirty entry point, 0x15220 its
 * completion point (MOVEM restore + RTS after the source-list terminator). This
 * only counts transitions and logs when a widened frame's capture() falls inside
 * a dirty span; it does not yet change frame accept/reject behavior (steps 2-4
 * of the proposed contract are not implemented). */
static unsigned producer_epoch,producer_dirty_since_epoch;
void db2_video_visor_offset(int tiles){visor_offset=tiles;}
int db2_video_visor_requested(void){return visor_offset;}
static int plane_layout(const GVDP *v){
 unsigned a=((unsigned)v->reg[2]&0x38)<<10,b=((unsigned)v->reg[4]&7)<<13;
 if(a==0xc000&&b==0xe000){world_is_a=1;return 1;}
 if(a==0xe000&&b==0xc000){world_is_a=0;return 1;}
 return 0;
}
static unsigned word(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
void db2_video_reset(void){db2_bio_overlay_reset();world_scene=ready=rejected=0;count=0;effect_count=0;canvas_top=0;canvas_height=224;memset(native_valid,0,sizeof native_valid);const char *t=getenv("DYNA_DB2_VIDEO_TRACE");trace_enabled=t&&!strcmp(t,"1");frame_reasons=trace_seen=0;trace_signature=~0u;trace_budget=32;producer_epoch=producer_dirty_since_epoch=0;}
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
  Db2Actor a;if(!(db2_actor_decode_cached(&ac,id,&a)||db2_actor_decode(&ac,id,&a))){ready=0;frame_reasons|=4;continue;}
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
 int wx=x-camera.anchor_x+(int)camera.cell_x*32;
 int wy=y-camera.anchor_y+(int)camera.cell_y*32;
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
  for(int x=0;x<320;x++){unsigned owner=r->sprite_owner[x];if(!owner)continue;
   if(owner>80||!owner_kind[owner-1]){rejected=1;frame_reasons|=8;continue;}
   if(owner_kind[owner-1]==3)continue;
   int dx=x+(owner_kind[owner-1]==2?layout.hud_shift:shift);
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
 if(supported&&ready){
  if(memcmp(ram,g_ram,sizeof ram))capture(v);else memcpy(&snapshot,v,sizeof snapshot);
  classify_sprites();
 }
 if(!supported||!ready||rejected){if(!supported)frame_reasons|=32;rejected=1;for(int x=0;x<w;x++)out[x]=background;if(w>=nw){memcpy(out+(w-nw)/2,p,(size_t)nw*sizeof *p);if(nw==320)db2_bio_overlay_row(v,y,out,w,(w-nw)/2);}return;}
 world_row(y,w,r,out);
}
static void complete(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,int nh){
 (void)v;if(!pixels||w<320||w>LIMIT||h>MAX_HEIGHT||stride<w||top<0||top+nh>h)return;
 for(int y=0;y<h;y++)if(y<top||y>=top+nh){if(ready)world_row(y-top,w,NULL,pixels+(size_t)y*stride);else for(int x=0;x<w;x++)pixels[(size_t)y*stride+x]=background;}
}
static void trace_totals(void){
 if(trace_enabled&&!((trace_ok_frames+trace_fallback_frames)%600))fprintf(stderr,"[db2-video] totals accepted=%u fallback=%u r01=%u r02=%u r04=%u r08=%u r10=%u r20=%u\n",trace_ok_frames,trace_fallback_frames,trace_reason_frames[0],trace_reason_frames[1],trace_reason_frames[2],trace_reason_frames[3],trace_reason_frames[4],trace_reason_frames[5]);
}
static void frame_complete(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,int nh){
 if(!pixels||w<320||w>LIMIT||stride<w||top<0||nh!=224||top+nh>h)return;
 if(!rejected&&ready){
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
 /* All rows are restored together, including rows drawn before an unknown
  * effect appeared. No callback/provider/runner pointer is retained. */
 for(int y=0;y<h;y++){
  uint32_t *out=pixels+(size_t)y*stride;for(int x=0;x<w;x++)out[x]=background;
  int source=y-top;if(source>=0&&source<224&&native_valid[source]){memcpy(out+(w-320)/2,native_image[source],sizeof native_image[source]);db2_bio_overlay_row(v,source,out,w,(w-320)/2);}
 }
}
const GameVideo db2_video={.configure=configure,.enabled=enabled,.width=width,.raster_enabled=raster_enabled,.scanline_presented=scanline,.integer_scale_supported=1,.canvas_geometry=geometry,.canvas_complete=complete,.frame_complete=frame_complete,.overlay_scanline=db2_bio_overlay_scanline};
