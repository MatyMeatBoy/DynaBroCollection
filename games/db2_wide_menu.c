#include "db2_wide_menu.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include "video/genesis_vdp_compose.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum { MAX_W=768 };
static unsigned rd(const GVDP *v,unsigned a){a&=0xffff;return ((unsigned)v->vram[a]<<8)|v->vram[(a+1)&0xffff];}
uint32_t db2_wide_menu_blend(uint32_t pixel,uint32_t backdrop,unsigned weight){
 const unsigned denominator=3375;   /* smoothstep over 16 pixels, 15^3 */
 if(weight>denominator)weight=denominator;
 uint32_t result=pixel&0xff000000u;
 for(unsigned shift=0;shift<24;shift+=8){
  unsigned x=(pixel>>shift)&255,y=(backdrop>>shift)&255;
  result|=((x*weight+y*(denominator-weight)+denominator/2)/denominator)<<shift;
 }
 return result;
}
static int cells(unsigned bits){return bits==0?32:bits==1?64:bits==3?128:32;}
static int reflect(int coordinate,int size){
 int phase=coordinate%(2*size);
 if(phase<0)phase+=2*size;
 return phase<size?phase:2*size-1-phase;
}

/* Per-frame column occupancy (whole plane height). A margin column whose name
 * entries are all zero has no art; it repeats the nearest edge block of the
 * authored run instead (EDGE_PERIOD cells, the floor/background texture
 * period), so backgrounds authored only a little wider than 320 continue. */
enum { EDGE_PERIOD=4 };
/* 2 = dense art, 1 = sparse art, 0 = empty; per plane column and row. */
static uint8_t col_art[2][128],row_art[2][128];
static int occupancy_ready,extended_used;

/* Title/main-menu clouds are sprites that respawn beyond the 4:3 window and sit
 * there until they start drifting; on a wide canvas that showed them popping in
 * at the margins. While the gate is on, a margin sprite is drawn only once it
 * has actually moved in the last few frames. */
static int sprite_gate;
static int16_t prev_sx[80],prev_sy[80];
static int16_t right_spawn[80];
static uint16_t prev_shape[80];
static uint8_t prev_live[80];
static uint8_t moved_recent[80];
static uint8_t moved_x_recent[80];
void db2_wide_menu_set_sprite_gate(int on){sprite_gate=on!=0;}
static void track_sprites(const GVDP *v){
 unsigned base=((unsigned)v->reg[5]&0x7f)<<9,link=0;uint8_t seen[80]={0},live[80]={0};
 for(int n=0;n<80;n++){
  if(link>=80||seen[link])break;seen[link]=1;live[link]=1;
  unsigned p=base+link*8;
  int sy=(int)(rd(v,p)&511)-128,sx=(int)(rd(v,p+6)&511)-128;
  unsigned shape=(rd(v,p+4)&2047)|((v->vram[(p+2)&0xffff]&15)<<11);
  int dx=sx-prev_sx[link],dy=sy-prev_sy[link];
  if(!prev_live[link]||dx<-8||dx>8)moved_x_recent[link]=0;
  else if(dx)moved_x_recent[link]=8;
  else if(moved_x_recent[link])--moved_x_recent[link];
  int recycled=!prev_live[link]||shape!=prev_shape[link]||dx<-8||dx>8||dy<-8||dy>8;
  if(recycled){
   moved_recent[link]=0;
   right_spawn[link]=sprite_gate&&sx>320&&sx<=383?(int16_t)sx:0;
  }else if(dx||dy)moved_recent[link]=12;
  else if(moved_recent[link])--moved_recent[link];
  prev_shape[link]=(uint16_t)shape;
  prev_sx[link]=(int16_t)sx;prev_sy[link]=(int16_t)sy;
  unsigned next=v->vram[(p+3)&0xffff]&127;if(!next)break;link=next;
 }
 memcpy(prev_live,live,sizeof prev_live);
 for(int i=0;i<80;i++)if(!live[i]||!sprite_gate)right_spawn[i]=0;
}
void db2_wide_menu_frame(const GVDP *v){
 occupancy_ready=0;extended_used=0;if(!v)return;
 track_sprites(v);
 int wt=cells(v->reg[16]&3),ht=cells((v->reg[16]>>4)&3);if(wt*ht>4096)ht=4096/wt;
 for(int plane=0;plane<2;plane++){
  unsigned base=plane?((unsigned)v->reg[4]&7)<<13:((unsigned)v->reg[2]&0x38)<<10;
  /* Only dense background columns/rows (art on at least 3/4 of the visible
   * 28 rows / 40 columns) count; sparse overlays such as dialogue boxes are
   * never repeated. */
  int need_c=(ht<28?ht:28)*3/4,need_r=(wt<40?wt:40)*3/4;
  int rows[128]={0};
  for(int cx=0;cx<wt;cx++){int n=0;for(int cy=0;cy<ht;cy++){int on=rd(v,base+((unsigned)(cy*wt+cx)<<1))!=0;n+=on;rows[cy]+=on;}col_art[plane][cx]=(uint8_t)(n>=need_c?2:n?1:0);}
  for(int cy=0;cy<ht;cy++)row_art[plane][cy]=(uint8_t)(rows[cy]>=need_r?2:rows[cy]?1:0);
 }
 occupancy_ready=1;
}
/* An empty cell outside the window repeats the nearest edge block of the dense
 * run around the visible centre, but only when the guest authored that run
 * wider/taller than the visible window (a scrolling background); a panel
 * exactly the size of the 4:3 window keeps its frame inside it. */
static int remap(const uint8_t *art,int pc,int n,int center,int visible){
 if(!occupancy_ready||art[pc]||art[center]!=2)return pc;
 int s=center,e=center,k=0;
 while(k<n&&art[(s-1+n)%n]==2){s=(s-1+n)%n;k++;}
 k=0;while(k<n&&art[(e+1)%n]==2){e=(e+1)%n;k++;}
 int len=(e-s+n)%n+1;
 if(len<=visible||len>=n)return pc;
 int left=(s-pc+n)%n,right=(pc-e+n)%n;
 extended_used=1;
 if(left<=right){int q=(EDGE_PERIOD-left%EDGE_PERIOD)%EDGE_PERIOD;return (s+q)%n;}
 int q=(right-1)%EDGE_PERIOD;return (e-EDGE_PERIOD+1+q+n)%n;
}

static Db2MenuMargins margins;
static unsigned pool[4*28],pool_n;
void db2_wide_menu_margins(const GVDP *v,Db2MenuMargins mode){
 margins=mode;pool_n=0;
 if((mode!=DB2_MENU_FLAT&&mode!=DB2_MENU_BRIEFING)||!v)return;
 int wt=cells(v->reg[16]&3);unsigned base=((unsigned)v->reg[4]&7)<<13;
 static const int cols[4]={0,1,38,39};
 for(int i=0;i<4;i++)for(int cy=0;cy<28;cy++)pool[pool_n++]=rd(v,base+((unsigned)(cy*wt+cols[i])<<1));
 /* The user-approved look: flat backdrop colour only. Keep the single most
  * frequent tile (the empty sky tile) and drop the confetti pieces. */
 unsigned best=0,bn=0;
 for(unsigned i=0;i<pool_n;i++){unsigned n=0;for(unsigned j=0;j<pool_n;j++)n+=pool[j]==pool[i];if(n>bn){bn=n;best=pool[i];}}
 pool[0]=best;pool_n=1;
}
uint8_t db2_wide_menu_plane_pixel(const GVDP *v,int plane,int x,int line){
 if(margins==DB2_MENU_PANORAMA&&plane==1){
  x=reflect(x,320);
  line=reflect(line,224);
 }
 int wt=cells(v->reg[16]&3),ht=cells((v->reg[16]>>4)&3);
 /* 128-cell planes are only valid with 32-cell height and vice versa. */
 if(wt*ht>4096)ht=4096/wt;
 int pw=wt*8,ph=ht*8;
 /* Confetti backdrop (stage briefing): outside the window every 8x8 cell takes a
  * tile drawn from the real confetti tiles of the outer columns, chosen by a
  * fixed hash so the continuation is stable but never an echo of the edge. */
 if((margins==DB2_MENU_FLAT||margins==DB2_MENU_BRIEFING)&&plane==1&&
    (x<0||x>=320||line<0||line>=224)){
  int cx=x>=0?x>>3:-((-x+7)>>3),cy=line>=0?line>>3:-((-line+7)>>3);
  unsigned h=(unsigned)cx*2654435761u^(unsigned)cy*40503u*2246822519u;h^=h>>15;h*=2246822519u;h^=h>>13;
  unsigned name=pool_n?pool[h%(unsigned)pool_n]:0;
  int tx=((x%8)+8)%8,ty=((line%8)+8)%8;
  if(name&0x800)tx=7-tx;
  if(name&0x1000)ty=7-ty;
  unsigned bb=v->vram[((name&2047)*32+(unsigned)ty*4+(unsigned)tx/2)&0xffff];
  unsigned c=(tx&1)?bb&15:bb>>4;
  if(!c)return (uint8_t)((name&0x8000)?0x80:0);
  return (uint8_t)(((name>>9)&48)|c|0x40|((name&0x8000)?0x80:0));
 }
 unsigned base=plane?((unsigned)v->reg[4]&7)<<13:((unsigned)v->reg[2]&0x38)<<10;
 unsigned hbase=((unsigned)v->reg[13]&63)<<10;
 int hmode=v->reg[11]&3;
 int l=line<0?0:line>223?223:line;   /* letterbox rows reuse the edge scroll */
 int hidx=hmode==0?0:hmode==2?(l&~7):l;
 int hs=(int)(rd(v,hbase+(unsigned)hidx*4+(plane?2u:0u))&1023);
 int vs;
 if(v->reg[11]&4){
  int col=x>>4;if(col<0)col=0;if(col>19)col=19;
  vs=v->vsram[(col*2+plane)&(GVDP_VSRAM_ENTRIES-1)]&1023;
 }else vs=v->vsram[plane]&1023;
 int px=((x-hs)%pw+pw)%pw,py=((line+vs)%ph+ph)%ph;
 if(x<0||x>=320){
  int center=(((160-hs)%pw+pw)%pw)>>3;
  px=(remap(col_art[plane],px>>3,wt,center,40)<<3)|(px&7);
 }
 if(line<0||line>=224){
  int center=(((112+vs)%ph+ph)%ph)>>3;
  py=(remap(row_art[plane],py>>3,ht,center,28)<<3)|(py&7);
 }
 unsigned name=rd(v,base+((unsigned)((py>>3)*wt+(px>>3))<<1));
 int tx=px&7,ty=py&7;
 if(name&0x800)tx=7-tx;
 if(name&0x1000)ty=7-ty;
 unsigned b=v->vram[((name&2047)*32+(unsigned)ty*4+(unsigned)tx/2)&0xffff];
 unsigned c=(tx&1)?b&15:b>>4;
 if(!c)return (uint8_t)((name&0x8000)?0x80:0);
 return (uint8_t)(((name>>9)&48)|c|0x40|((name&0x8000)?0x80:0));
}

/* Window plane replaces plane A on its region; the margin beside a window
 * region has no plane A of its own, so plane A is left transparent there. */
static int window_side(const GVDP *v,int line,int x){
 unsigned wx=v->reg[17],wy=v->reg[18];
 int wyl=(int)(wy&31)*8;
 if(wy&128?line>=wyl:line<wyl)return 1;
 int wxp=(int)(wx&31)*16;
 if(!wxp&&!(wx&128))return 0;
 if(wx&128)return x>=wxp;   /* window to the right of wxp */
 return x<wxp;              /* window to the left of wxp */
}

static int title_channel_logo(unsigned attr,unsigned size){
 if((size&15)!=15)return 0;
 switch(attr&0xe7ff){
 case 0x24b5:case 0x24c5:case 0x26a0:case 0x26b0:
 case 0x26d8:case 0x26e8:case 0x27d0:case 0x27e0:return 1;
 default:return 0;
 }
}

int db2_wide_menu_parked_title_logo(const GVDP *v){
 unsigned sat=((unsigned)v->reg[5]&0x7f)<<9;
 /* The first pieces of both rows are parked at x=-68 on a skipped reveal. */
 return rd(v,sat+20*8+4)==0x24b5&&(rd(v,sat+20*8+6)&511)==60&&
        rd(v,sat+24*8+4)==0x26d8&&(rd(v,sat+24*8+6)&511)==60;
}

int db2_wide_menu_title_logo_sprite(const GVDP *v,unsigned sat_index){
 if(sat_index<20||sat_index>27)return 0;
 unsigned p=(((unsigned)v->reg[5]&0x7f)<<9)+sat_index*8;
 return title_channel_logo(rd(v,p+4),v->vram[(p+2)&0xffff]);
}

static void sprites(const GVDP *v,int line,int left,int w,uint8_t *s,uint8_t *o,uint8_t *hi){
 unsigned base=((unsigned)v->reg[5]&0x7f)<<9,link=0;uint8_t seen[80]={0};
 int cloud_x=0,cloud_y=0,cloud_spawn=0,cloud_piece=0,cloud_moved=0;
 unsigned cloud_pattern=0;
 for(int n=0;n<80;n++){
  if(link>=80||seen[link])return;seen[link]=1;
  unsigned link_index=link;
  unsigned p=base+link*8;
  int sy=(int)(rd(v,p)&511)-128;
  unsigned size=v->vram[(p+2)&0xffff],next=v->vram[(p+3)&0xffff]&127;
  unsigned attr=rd(v,p+4);
  int sx=(int)(rd(v,p+6)&511)-128;
  int hc=(int)((size>>2)&3)+1,vc=(int)(size&3)+1;
  int spawn=right_spawn[link_index],right=w-left;
  int moved=moved_recent[link_index];
  unsigned pattern=attr&2047;
  /* The title's clouds use adjacent 32px pieces. The guest parks individual
   * pieces at x=336 until it needs them in 4:3. Derive their margin positions
   * from the moving first piece so the wide reveal keeps the cloud together. */
  if(sprite_gate&&(attr&0xe000)==0x2000&&(size&15)==15&&
     (pattern==0x780||pattern==0x7a0)){
   cloud_pattern=pattern;cloud_x=sx;cloud_y=sy;cloud_spawn=spawn;
   cloud_piece=0;cloud_moved=moved;
  }else if(cloud_pattern&&sy==cloud_y&&(attr&0xe000)==0x2000&&
           (size&15)==15&&pattern==cloud_pattern+16u*(unsigned)(cloud_piece+1)&&
           cloud_piece<(cloud_pattern==0x780?1:2)){
   ++cloud_piece;
   if(cloud_x>=256){sx=cloud_x+cloud_piece*32;spawn=cloud_spawn;moved=cloud_moved;}
  }else cloud_pattern=0;
  /* Map this cloud's lifecycle from the wide edge back to native x=320.
   * Once it reaches the native area its original SAT position takes over. */
  int anchor=cloud_pattern&&cloud_x>=256?cloud_x:sx;
  if(sprite_gate&&anchor>320&&spawn>320&&spawn<right)
   sx+=320+(anchor-320)*(right-320)/(spawn-320)-anchor;
  if(line>=sy&&line<sy+vc*8&&sx>-128){
   int ry=line-sy;if(attr&0x1000)ry=vc*8-1-ry;
   /* The guest parks the eight logo pieces left of the native screen when
    * Start skips their reveal. Their Y motion still follows the menu scroll. */
   int gated=sprite_gate&&(!moved||title_channel_logo(attr,size));
   /* The briefing parks its waiting actors in these slots. They enter the
    * presentation when the guest advances them toward the center. */
   if(margins==DB2_MENU_BRIEFING&&!moved_x_recent[link_index]&&
      (sx==-32||sx==319||sx==320))gated=1;
   for(int c=0;c<hc*8;c++){
    int x=sx+c;
    if(x>=0&&x<320&&(margins!=DB2_MENU_PANORAMA||(line>=0&&line<224)))continue;
    if(gated)continue;
    int dx=x+left;if(dx<0||dx>=w||o[dx])continue;
    int rx=c;if(attr&0x800)rx=hc*8-1-rx;
    unsigned tile=((attr&2047)+(unsigned)(rx>>3)*(unsigned)vc+(unsigned)(ry>>3))&2047;
    unsigned b=v->vram[(tile*32+(unsigned)(ry&7)*4+(unsigned)(rx&7)/2)&0xffff];
    unsigned col=(rx&1)?b&15:b>>4;
    if(!col)continue;
    s[dx]=(uint8_t)(((attr>>9)&48)|col);o[dx]=1;hi[dx]=(uint8_t)(attr>>15);
   }
  }
  if(!next)return;
  link=next;
 }
}

int db2_wide_menu_row(const GVDP *v,int line,uint32_t *out,int w,int left,int native_row){
 if(!v||!out||w<=320||w>MAX_W||left<0||left+320>w)return 0;
 if(!(v->reg[12]&1))return 0;        /* H40 only */
 if(!(v->reg[1]&0x40)){for(int x=0;x<w;x++)if(!native_row||x<left||x>=left+320)out[x]=0xff000000u;return 1;}
 uint8_t a[MAX_W]={0},b[MAX_W]={0},s[MAX_W]={0},o[MAX_W]={0},hi[MAX_W]={0},z[MAX_W]={0},px[MAX_W];
 for(int dx=0;dx<w;dx++){
  int x=dx-left;
  if(native_row&&x>=0&&x<320)continue;
  if(margins!=DB2_MENU_SPRITES)b[dx]=db2_wide_menu_plane_pixel(v,1,x,line);
  if(margins==DB2_MENU_PLANES&&!window_side(v,line,x))a[dx]=db2_wide_menu_plane_pixel(v,0,x,line);
 }
 if(margins!=DB2_MENU_FLAT&&margins!=DB2_MENU_BACKGROUND)sprites(v,line,left,w,s,o,hi);
 int shadow=(v->reg[12]&8)!=0;
 gvdp_compose_span(px,a,b,s,o,hi,z,z,w,(uint8_t)(v->reg[7]&63),shadow);
 for(int dx=0;dx<w;dx++){
  if(native_row&&dx>=left&&dx<left+320)continue;
  unsigned p=px[dx];
  out[dx]=genesis_dac_cram_to_argb(v->cram[p&63],p>=128?2:p>=64?1:0);
 }
 return 1;
}

int db2_wide_menu_extended(void){return extended_used;}
