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
/* Reference title CRAM from title-vdp.bin, little-endian words at 65536.
 * Duplicate entries resolve to the first index, never to a guessed cycle. */
static const uint16_t reference[64]={
1092,3822,2730,1638,1254,1220,1156,1124,1092,1092,1102,3822,1096,1092,1092,1092,
1092,3822,2730,1638,1254,1220,1156,1124,1092,1092,1102,1262,1096,1092,1092,1092,
1092,3822,2730,1638,1254,1220,1156,1124,1092,1092,1102,3822,1096,1092,1092,1092,
1092,1126,1092,1092,1092,1156,1124,1092,2628,2116,1604,1126,1092,1096,1094,2184};
static unsigned word(const GVDP *v,unsigned a){return v->vram[a&65535]*256u+v->vram[(a+1)&65535];}
static int wrap(int n,int p){int r=n%p;return r<0?r+p:r;}
int db1_title_art_band_offset(int w,int h,int sx,int sy){
 if(w<512||w>4096||h<288||h>4096||abs(w*288-h*512)>h*6||sy< -32||sy>=256||(sy>=0&&sy<224))return -1;
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
uint32_t db1_title_art_alpha(uint32_t d,uint32_t s,unsigned a){
 if(a>255)a=255;uint32_t r=0xff000000u;for(unsigned shift=0;shift<24;shift+=8)r|=((((s>>shift)&255)*a+((d>>shift)&255)*(255-a)+127)/255)<<shift;return r;
}
static void load(Art *a,const char *base,const char *name,unsigned first,unsigned count,int target_w,int target_h){
 char path[2048];int channels;if(snprintf(path,sizeof path,"%sassets/img/%s",base,name)>=(int)sizeof path)return;
 if(!stbi_info(path,&a->w,&a->h,&channels)||a->w<=0||a->h<=0||a->w>4096||a->h>4096)return;
 unsigned char *source=stbi_load(path,&a->w,&a->h,&channels,4);if(!source)return;
 if(first==48&&db1_title_art_band_offset(a->w,a->h,0,-32)<0){stbi_image_free(source);return;}
 int source_w=a->w,source_h=a->h;a->w=target_w;a->h=target_h;
 a->rgba=calloc((size_t)a->w*a->h,4);if(!a->rgba){stbi_image_free(source);return;}
 for(int y=0;y<a->h;y++)for(int x=0;x<a->w;x++){
  if(first==48&&y>=32&&y<256)continue;
  unsigned char *p=source+((y*source_h/a->h)*source_w+x*source_w/a->w)*4;
  memcpy(a->rgba+(y*a->w+x)*4,p,4);
 }
 stbi_image_free(source);
 a->map=malloc((size_t)a->w*a->h);if(!a->map){stbi_image_free(a->rgba);a->rgba=NULL;return;}
 for(int i=0;i<a->w*a->h;i++){unsigned char *p=a->rgba+i*4;a->map[i]=p[3]?(unsigned char)db1_title_art_nearest((uint32_t)p[0]<<16|(uint32_t)p[1]<<8|p[2],reference,first,count):0;}
}
static void ensure(void){if(loaded)return;loaded=1;char *base=SDL_GetBasePath();if(!base)return;load(&bands,base,"db1-title-background-512x288.png",48,16,512,288);load(&logo,base,"dyna-logo-v1.png",16,16,224,120);SDL_free(base);}
int db1_title_art_identified(const GVDP *v){
 if(!v||db1_scenes_kind()!=DB1_SCENE_TITLE||(v->reg[16]&0x33)!=1||(v->reg[12]&6)||!(v->reg[1]&64))return 0;
 unsigned a=(v->reg[2]&56)<<10;
 return word(v,a+2*(3*64+13))==0x2102&&word(v,a+2*(17*64+31))==0x2262;
}
static uint32_t mapped(const GVDP *v,const Art *a,int i){unsigned m=a->map[i];return genesis_dac_cram_to_argb(v->cram[m&63],m>>6);}
static int a_coordinates(const GVDP *v,int x,int y,int *sx,int *sy){
 int raster=y<0?0:y>223?223:y;unsigned mode=v->reg[11]&3;int hi=mode==0?0:mode==2?raster&~7:raster;
 int col=x/16;if(x<0&&x%16)--col;if(col<0)col=0;if(col>19)col=19;
 *sx=x-(int)(word(v,((v->reg[13]&63)<<10)+hi*4)&1023);*sy=y+(v->vsram[(v->reg[11]&4)?col*2:0]&1023);return 1;
}
void db1_title_art_row(const GVDP *v,int line,uint32_t *out,int width,int left,int english){
 if(!out||width<=0||!db1_title_art_identified(v))return;ensure();
 for(int dx=0;dx<width;dx++){
  int x=dx-left,sx,sy;
  if(bands.rgba&&db1_scenes_plane_b_coordinates(v,x,line,&sx,&sy)){
   int off=db1_title_art_band_offset(bands.w,bands.h,sx,sy);
   /* Never paint over native foreground, even when scrolling reveals a band. */
   if(off>=0&&(line<0||line>=224||x<0||x>=320))out[dx]=mapped(v,&bands,off/4);
  }
  if(!english||!logo.rgba)continue;
  a_coordinates(v,x,line,&sx,&sy);int px=wrap(sx,512),py=wrap(sy,256);
  if(px<48||px>=272||py<24||py>=144)continue;
  unsigned attr=word(v,((v->reg[2]&56)<<10)+2*((py/8)*64+px/8)),tile=attr&2047;
  if(tile>=0x102&&tile<=0x262){int tx=px&7,ty=py&7;if(attr&2048)tx=7-tx;if(attr&4096)ty=7-ty;unsigned bits=v->vram[(tile*32+ty*4+tx/2)&65535],n=(bits>>(tx&1?0:4))&15;if(n)out[dx]=db1_scenes_background(v,x,line);}
  int i=((py-24)*logo.h/120)*logo.w+(px-48)*logo.w/224;out[dx]=db1_title_art_alpha(out[dx],mapped(v,&logo,i),logo.rgba[i*4+3]);
 }
}
