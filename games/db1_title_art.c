#include "db1_title_art.h"
#include "video/genesis_dac.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 4096
#include "../upstream/recomp-ui/src/third_party/stb_image.h"
typedef struct {unsigned char *rgba;unsigned char *map;int w,h;} Art;
static Art bands,logo;
static int loaded;
static int background_shift;
void db1_title_art_geometry(int height,int top,int native_height){
 background_shift=native_height==224&&height>=224&&height<=480&&top>=0&&top+224<=height?height-top-224:0;
}
int db1_title_art_background_line(int line){return line-background_shift;}
/* Background reference: title-vdp.bin CRAM words at65536. Logo bank1 is the
 * stable native palette reconstructed from pattern-index/native-JA pixel
 * correlation; that dump's white-fade floor is unsuitable for logo ink. */
static const uint16_t reference[64]={
1092,3822,2730,1638,1254,1220,1156,1124,1092,1092,1102,3822,1096,1092,1092,1092,
0x000,0xeee,0xaaa,0x666,0x4e6,0x4c4,0x282,0x0e0,0x020,0x000,0x02e,0x0ee,0x008,0x444,0x222,0x000,
1092,3822,2730,1638,1254,1220,1156,1124,1092,1092,1102,3822,1096,1092,1092,1092,
1092,1126,1092,1092,1092,1156,1124,1092,2628,2116,1604,1126,1092,1096,1094,2184};
static unsigned word(const GVDP *v,unsigned a){return v->vram[a&65535]*256u+v->vram[(a+1)&65535];}
static int wrap(int n,int p){int r=n%p;return r<0?r+p:r;}
int db1_title_art_band_offset(int w,int h,int sx,int sy){
 if(w<512||w>4096||h<288||h>4096||abs(w*288-h*512)>h*6||sy< -32||sy>=0)return -1;
 return (((sy+32)*h/288)*w+wrap(sx,512)*w/512)*4;
}
unsigned db1_title_art_nearest(uint32_t rgb,const uint16_t p[64],unsigned first,unsigned count){
 unsigned best=first,dist=~0u;if(first>=64||count>64-first)return 0;
 /* DB1 title has shadow/highlight disabled (reg12=0x81). Introducing a
  * synthetic shadow would leave gray ghosts in its native white fade. */
 for(unsigned level=0;level<1;level++)for(unsigned i=first;i<first+count;i++){
  uint32_t c=genesis_dac_cram_to_argb(p[i],(int)level);int dr=(int)((rgb>>16)&255)-(int)((c>>16)&255),dg=(int)((rgb>>8)&255)-(int)((c>>8)&255),db=(int)(rgb&255)-(int)(c&255);unsigned d=(unsigned)(dr*dr+dg*dg+db*db);
  if(d<dist){dist=d;best=i+level*64;}
 }return best;
}
unsigned db1_title_art_logo_nearest(uint32_t rgb){
 unsigned index=db1_title_art_nearest(rgb,reference,17,15);
 /* Original opaque outline uses nibble15, not unused palette slot0 or the
  * duplicate volcanic black. Preserve its semantic live fade ownership. */
 return reference[index]==0?31:index;
}
uint32_t db1_title_art_alpha(uint32_t d,uint32_t s,unsigned a){
 if(!a)return d|0xff000000u;
 if(a>=255)return s|0xff000000u;
 uint32_t r=0xff000000u;for(unsigned shift=0;shift<24;shift+=8)r|=((((s>>shift)&255)*a+((d>>shift)&255)*(255-a)+127)/255)<<shift;return r;
}
static void load(Art *a,const char *base,const char *name,unsigned first,unsigned count,int target_w,int target_h){
 char path[2048];int channels;if(snprintf(path,sizeof path,"%sassets/img/%s",base,name)>=(int)sizeof path)return;
 if(!stbi_info(path,&a->w,&a->h,&channels)||a->w<=0||a->h<=0||a->w>4096||a->h>4096)return;
 unsigned char *source=stbi_load(path,&a->w,&a->h,&channels,4);if(!source)return;
 if(first==48&&db1_title_art_band_offset(a->w,a->h,0,-32)<0){stbi_image_free(source);return;}
 int source_w=a->w,source_h=a->h;a->w=target_w;a->h=target_h;
 a->rgba=calloc((size_t)a->w*a->h,4);if(!a->rgba){stbi_image_free(source);return;}
 for(int y=0;y<a->h;y++)for(int x=0;x<a->w;x++){
  if(first==48&&y>=32)continue;
  unsigned char *p=source+((y*source_h/a->h)*source_w+x*source_w/a->w)*4;
  memcpy(a->rgba+(y*a->w+x)*4,p,4);
 }
 stbi_image_free(source);
 a->map=malloc((size_t)a->w*a->h);if(!a->map){stbi_image_free(a->rgba);a->rgba=NULL;return;}
 for(int i=0;i<a->w*a->h;i++){unsigned char *p=a->rgba+i*4;uint32_t rgb=(uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2];a->map[i]=p[3]?(unsigned char)(first==16?db1_title_art_logo_nearest(rgb):db1_title_art_nearest(rgb,reference,first,count)):0;}
}
void db1_title_art_preload(void)
{
 if(loaded)return;
 loaded=1;
 char *base=SDL_GetBasePath();
 if(!base)return;
 const char *timing=getenv("GENESIS_HOST_TIMING_DUMP");
 int trace=timing && *timing && *timing!='0';
 Uint64 start=trace?SDL_GetPerformanceCounter():0;
 load(&bands,base,"db1-title-background-native.png",48,16,512,288);
 if(!bands.map)load(&bands,base,"db1-title-background-512x288.png",48,16,512,288);
 Uint64 bands_done=trace?SDL_GetPerformanceCounter():0;
 load(&logo,base,"db1-title-logo-native.png",16,16,224,120);
 if(!logo.map)load(&logo,base,"dyna-logo-v1.png",16,16,224,120);
 if(trace){
  Uint64 done=SDL_GetPerformanceCounter(), frequency=SDL_GetPerformanceFrequency();
  if(frequency)fprintf(stderr,"[DB1-TITLE-ART] background_us=%llu logo_us=%llu ready=%d\n",
      (unsigned long long)((bands_done-start)*1000000/frequency),
      (unsigned long long)((done-bands_done)*1000000/frequency),logo.map!=NULL);
 }
 SDL_free(base);
}
int db1_title_art_logo_ready(void){return logo.rgba!=NULL && logo.map!=NULL;}
int db1_title_art_identified(const GVDP *v){
 if(!v||db1_scenes_kind()!=DB1_SCENE_TITLE||(v->reg[16]&0x33)!=1||(v->reg[12]&6)||!(v->reg[1]&64))return 0;
 unsigned a=(v->reg[2]&56)<<10;
 return word(v,a+2*(3*64+13))==0x2102&&word(v,a+2*(17*64+31))==0x2262;
}
static uint32_t mapped(const GVDP *v,const Art *a,int i){unsigned m=a->map[i];return genesis_dac_cram_to_argb(v->cram[m&63],m>>6);}
static unsigned pattern_pixel(const GVDP *v,unsigned attr,int tx,int ty){
 if(attr&2048)tx=7-tx;if(attr&4096)ty=7-ty;
 unsigned bits=v->vram[((attr&2047)*32+ty*4+tx/2)&65535];return (bits>>(tx&1?0:4))&15;
}
static void sprite_mask(const GVDP *v,int line,int width,int left,unsigned char *mask){
 unsigned base=(v->reg[5]&0x7f)<<9,index=0;
 for(unsigned guard=0;guard<80;guard++){
  unsigned a=base+index*8,y=word(v,a)&511,size=v->vram[(a+2)&65535],attr=word(v,a+4);
  int sx=(int)(word(v,a+6)&511)-128,sy=(int)y-128,wt=(int)((size>>2)&3)+1,ht=(int)(size&3)+1;
  int row=line-sy;
  if(row>=0&&row<ht*8){
   if(attr&4096)row=ht*8-1-row;
   for(int x=0;x<wt*8;x++){
    int dx=sx+x+left;if(dx<0||dx>=width)continue;
    int col=(attr&2048)?wt*8-1-x:x;unsigned tile=((attr&2047)+(unsigned)(col/8*ht+row/8))&2047;
    if(pattern_pixel(v,tile,col&7,row&7))mask[dx]=1;
   }
  }
  index=v->vram[(a+3)&65535]&127;if(!index)break;
 }
}
uint32_t db1_title_art_background_pixel(const GVDP *v,int x,int line){
 int shifted=db1_title_art_background_line(line),sx,sy;
 if(shifted<0&&db1_scenes_plane_b_coordinates(v,x,shifted,&sx,&sy)&&sy<0){
  if(bands.rgba){int off=db1_title_art_band_offset(bands.w,bands.h,sx,sy< -32?-32:sy);if(off>=0)return mapped(v,&bands,off/4);}
  /* Missing optional sky falls back to the authored first sky raster. */
  return db1_scenes_background(v,x,0);
 }
 return db1_scenes_background(v,x,shifted);
}
void db1_title_art_row(const GVDP *v,int line,uint32_t *out,int width,int left,int english){
 if(!out||width<=0||width>768||!db1_title_art_identified(v))return;
 unsigned char sprites[768]={0};sprite_mask(v,line,width,left,sprites);
 unsigned plane=(v->reg[2]&56)<<10;
 unsigned mode=v->reg[11]&3;
 int raster=line<0?0:line>223?223:line;
 int scroll_row=mode==0?0:mode==2?raster&~7:raster;
 unsigned scroll=word(v,((v->reg[13]&63)<<10)+scroll_row*4)&1023;
 int column_scroll=v->reg[11]&4;

 /* The title signature fixes Plane A at 64x32 tiles. Horizontal scroll
  * stays fixed across this raster, and vertical scroll changes only at
  * native 16-pixel column boundaries. */
 for(int dx=0;dx<width;){
  int x=dx-left;
  int col=x<0?0:x/16;
  if(col>19)col=19;
  int end=width;
  if(column_scroll&&col<19){
   int next=(col+1)*16+left;
   if(next<end)end=next;
  }
  unsigned py=(unsigned)(line+(v->vsram[column_scroll?col*2:0]&1023))&255;
  unsigned name_row=plane+128*(py/8);
  int logo_row=english&&logo.rgba&&py>=24&&py<144
              ? (int)(py-24)*logo.w : -1;
  for(;dx<end;dx++,x++){
   unsigned px=((unsigned)x-scroll)&511;
   int i=logo_row>=0&&px>=48&&px<272?logo_row+(int)px-48:-1;
   unsigned alpha=i>=0?logo.rgba[i*4+3]:0;

   /* An opaque replacement covers the backing regardless of tile or SAT
    * ownership. Transparent replacement pixels still follow native rules. */
   if(alpha==255){
    out[dx]=mapped(v,&logo,i);
    continue;
   }
   if(!sprites[dx]){
    unsigned attr=word(v,name_row+2*(px/8)),tile=attr&2047;
    unsigned opaque=pattern_pixel(v,attr,px&7,py&7);
    if(!opaque||(i>=0&&tile>=0x102&&tile<=0x262))
     out[dx]=db1_title_art_background_pixel(v,x,line);
   }
   if(alpha)out[dx]=db1_title_art_alpha(out[dx],mapped(v,&logo,i),alpha);
   else if(i>=0)out[dx]|=0xff000000u;
  }
 }
}
