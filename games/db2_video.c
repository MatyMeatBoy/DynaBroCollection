#include "db2_video.h"
#include "db2_world.h"
#include "db2_actors.h"
#include "db2_markers.h"
#include "db2_effects.h"
#include "viewport.h"
#include "genesis_runtime.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include "video/genesis_vdp_compose.h"
#include <string.h>

enum { LIMIT=768, MAX_HEIGHT=480 };
static DynaViewport view=DYNA_VIEWPORT_DEFAULT;
static int battle,ready,canvas_top,canvas_height=224;
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
static unsigned word(const uint8_t *p){return ((unsigned)p[0]<<8)|p[1];}
void db2_video_reset(void){battle=ready=rejected=0;count=0;effect_count=0;canvas_top=0;canvas_height=224;memset(native_valid,0,sizeof native_valid);}
void db2_video_observe(unsigned pc){if(pc==0x3bb0)battle=1;else if(pc==0x3bb4||pc==0x734e)battle=0;}
static int configure(const char *s){return dyna_viewport_configure(&view,s);}
static int enabled(void){return view.enabled;}
static int width(int dw,int dh,int nw,int nh){(void)nw;canvas_top=0;canvas_height=nh;return dyna_viewport_width(&view,dw,dh,nh);}
static int raster_enabled(const GVDP *v){(void)v;return battle&&!(g_ram[0x286]&0x88);}
static int geometry(int dw,int dh,int nw,int nh,int *w,int *h){
 if(!view.enabled||nw!=320||nh!=224)return 0;
 int scale=game_video_integer_canvas(dw,dh,dyna_viewport_width(&view,dw,dh,nh),nh,w,h);
 if(!scale||*w>LIMIT||*h>MAX_HEIGHT)return 0;
 canvas_top=(*h-nh)/2;canvas_height=*h;return scale;
}
static void capture(const GVDP *v){
 memcpy(ram,g_ram,sizeof ram);memcpy(&snapshot,v,sizeof snapshot);
 Db2WorldContext c={g_rom,2621440,ram,sizeof ram,snapshot.vram,sizeof snapshot.vram};
 ready=db2_world_read_camera(&c,&camera);
 Db2ActorsContext ac={g_rom,2621440,ram,sizeof ram};count=0;
 for(unsigned id=0;id<255;id++)rank[id]=72+id;
 unsigned off=word(ram+0x9d3a);uint8_t seen[72]={0};
 for(unsigned n=0;n<72&&off;n++){
  if(off<4||(off-4)%8||(off-4)/8>=72||seen[(off-4)/8]){ready=0;break;}
  seen[(off-4)/8]=1;const uint8_t *p=ram+0x9d3a+off;
  if(p[2]>0&&p[2]<255)rank[p[2]]=n;
  off=word(p);
 }
 for(unsigned id=1;id<=254;id++){
  const uint8_t *p=ram+0x5f58+48*id;
  if(!p[0])continue;
  Db2Actor a;if(!(db2_actor_decode_cached(&ac,id,&a)||db2_actor_decode(&ac,id,&a))){ready=0;continue;}
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
  if((i==6||i==7)&&(attr==0x82f4||attr==0x8376)&&(size==10||size==5))return 1;
  if(i>=8&&i<=11&&(attr&0xe7ff)==0x86e8&&size==5)return i>=10?2:1;
  if(i==31&&attr==0x032f&&size==15)return 1;
 }
 return 0;
}
static void classify_sprites(void){
 Db2ActorsContext ac={g_rom,2621440,ram,sizeof ram};
 unsigned base=((unsigned)snapshot.reg[5]&127)<<9,link=0;uint8_t seen[80]={0};
 memset(owner_kind,0,sizeof owner_kind);
 for(unsigned n=0;n<80;n++){
  if(link>=80||seen[link]){rejected=1;return;}seen[link]=1;
  unsigned p=base+link*8,attr=word(snapshot.vram+((p+4)&65535));
  unsigned size=snapshot.vram[(p+2)&65535],next=snapshot.vram[(p+3)&65535]&127;
  int x=(int)(word(snapshot.vram+((p+6)&65535))&511)-128;
  int y=(int)(word(snapshot.vram+(p&65535))&1023)-128;
  int kind=ui_kind(attr,size,x,y);
  if(!attr)kind=1;
  if(db2_actor_native_sat_body(&ac,attr)||db2_effects_is_marker((uint16_t)attr))kind=3;
  for(size_t i=0;i<effect_count;i++)if(effects[i].sat_index==link)kind=3;
  if(!kind){rejected=1;return;}
  owner_kind[link]=(uint8_t)kind;
  if(!next)return;
  link=next;
 }
 rejected=1;
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
static void effect_row(const Db2WorldEffect *e,int y,int w,uint8_t *s,uint8_t *o,uint8_t *hi){
 int h=((e->size&3)+1)*8,ww=(((e->size>>2)&3)+1)*8;
 if(y<e->y||y>=e->y+h)return;
 for(int x=0;x<ww;x++)put(s,o,hi,e->x+x,w,tile_pixel(e->attr,e->size,x,y-e->y),e->attr);
}
static void world_row(int y,int w,const GVDPRasterRow *r,uint32_t *out){
 uint8_t a[LIMIT]={0},b[LIMIT]={0},s[LIMIT]={0},o[LIMIT]={0},hi[LIMIT]={0};
 uint8_t sh[LIMIT]={0},hl[LIMIT]={0},pixels[LIMIT];
 Db2WorldContext c={g_rom,2621440,ram,sizeof ram,snapshot.vram,sizeof snapshot.vram};
 int wy=y-camera.anchor_y+(int)camera.cell_y*32;
 for(int x=0;x<w;x++){
  int wx=x-camera.anchor_x+(int)camera.cell_x*32;
  if(wx>=0&&wy>=0&&wx<1280&&wy<1280)db2_world_terrain_plane(&c,(unsigned)wx/32,(unsigned)wy/32,(unsigned)wx%32,(unsigned)wy%32,b+x);
 }
 /* Verify the entire native world span before accepting this frame. Weather,
  * unknown scroll/raster effects and terrain mismatches trigger whole-frame
  * native rollback at frame_complete, never a cut at the old HUD boundary. */
 if(r){if(memcmp(b,r->plane_b,216))rejected=1;memcpy(a+w-104,r->plane_a+216,104);}
 size_t ei=effect_count;
 for(unsigned remaining=count;remaining;){const Db2Actor *ac=actors+--remaining;
  while(ei&&(effects[ei-1].depth<ac->depth||(effects[ei-1].depth==ac->depth&&effects[ei-1].rank>rank[ac->id])))effect_row(effects+--ei,y,w,s,o,hi);
  for(unsigned part=ac->component_count;part;){const Db2ActorComponent *p=ac->part+--part;
   int xx=ac->x+p->dx,yy=ac->y+p->dy;if(y<yy||y>=yy+32)continue;
   for(int px=0;px<32;px++){uint8_t color;db2_actor_pixel(p,(unsigned)px,(unsigned)(y-yy),&color);put(s,o,hi,xx+px,w,color,p->flags);}
  }
  Db2WorldMarker m[6];unsigned n=db2_world_markers(ram[0x2a89],ram[0x5f58+48*ac->id+0x28],ac->x,ac->y,m);
  while(n){const Db2WorldMarker *p=m+--n;if(y<p->y||y>=p->y+8)continue;for(int px=0;px<8;px++)put(s,o,hi,p->x+px,w,tile_pixel(p->attribute,0,px,y-p->y),p->attribute);}
 }
 while(ei)effect_row(effects+--ei,y,w,s,o,hi);
 if(r){
  for(int x=0;x<320;x++){unsigned owner=r->sprite_owner[x];if(!owner)continue;
   if(owner>80||!owner_kind[owner-1]){rejected=1;continue;}
   if(owner_kind[owner-1]==3)continue;
   int dx=x+(owner_kind[owner-1]==2?w-320:0);
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
 if(y==0){ready=0;rejected=0;memset(native_valid,0,sizeof native_valid);if(supported)capture(v);}
 const uint32_t *p=native(ctx);
 if(y>=0&&y<224&&nw==320){memcpy(native_image[y],p,sizeof native_image[y]);native_valid[y]=1;}
 if(supported&&ready){
  if(memcmp(ram,g_ram,sizeof ram))capture(v);else memcpy(&snapshot,v,sizeof snapshot);
  classify_sprites();
  for(int x=0;x<216;x++)if(r->plane_a[x]&64){rejected=1;break;}
 }
 if(!supported||!ready||rejected){rejected=1;for(int x=0;x<w;x++)out[x]=background;if(w>=nw)memcpy(out+(w-nw)/2,p,(size_t)nw*sizeof *p);return;}
 world_row(y,w,r,out);
}
static void complete(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,int nh){
 (void)v;if(!pixels||w<320||w>LIMIT||h>MAX_HEIGHT||stride<w||top<0||top+nh>h)return;
 for(int y=0;y<h;y++)if(y<top||y>=top+nh){if(ready)world_row(y-top,w,NULL,pixels+(size_t)y*stride);else for(int x=0;x<w;x++)pixels[(size_t)y*stride+x]=background;}
}
static void frame_complete(const GVDP *v,uint32_t *pixels,int stride,int w,int h,int top,int nh){
 (void)v;if(!pixels||w<320||w>LIMIT||stride<w||top<0||nh!=224||top+nh>h)return;
 if(!rejected&&ready)return;
 /* All rows are restored together, including rows drawn before an unknown
  * effect appeared. No callback/provider/runner pointer is retained. */
 for(int y=0;y<h;y++){
  uint32_t *out=pixels+(size_t)y*stride;for(int x=0;x<w;x++)out[x]=background;
  int source=y-top;if(source>=0&&source<224&&native_valid[source])memcpy(out+(w-320)/2,native_image[source],sizeof native_image[source]);
 }
}
const GameVideo db2_video={.configure=configure,.enabled=enabled,.width=width,.raster_enabled=raster_enabled,.scanline_presented=scanline,.integer_scale_supported=1,.canvas_geometry=geometry,.canvas_complete=complete,.frame_complete=frame_complete};
