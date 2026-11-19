/* Game-owned Extras compositor. No guest memory access. */
#include "extras_ui.h"
#include "guide_content.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 2048
#include "../upstream/recomp-ui/src/third_party/stb_image.h"
typedef struct {uint32_t *p;int w,h,stride;} Canvas;
#define INK 0xffeeeeeeu
#define GOLD 0xffffff66u
#define BLACK 0xff000000u
#define EMBER 0xff751f2bu
#define SHADE 0xff2b131du
#define COPPER 0xffffa044u
#define GREEN 0xff006644u
static DynaExtrasLevelProvider provider;
static DynaExtrasUiState state;
static unsigned game_id=1;
static int opened;
static char notice[128];
void dyna_extras_ui_notice(const char *message){snprintf(notice,sizeof notice,"%s",message?message:"");}
static char root[1024],image_key[96];
static unsigned char *image_rgba;
static int image_w,image_h;
static const unsigned char letters[36][5]={
 {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
 {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},{0x41,0x41,0x7f,0x41,0x41},
 {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
 {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
 {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
 {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
 {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},{0x62,0x51,0x49,0x49,0x46},
 {0x22,0x41,0x49,0x49,0x36},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
 {0x06,0x49,0x49,0x29,0x1e}
};
static void pixel(Canvas *c,int x,int y,uint32_t color){if(x>=0&&y>=0&&x<c->w&&y<c->h)c->p[y*c->stride+x]=color;}
static void rect(Canvas *c,int x,int y,int w,int h,uint32_t color){
    int x0=x<0?0:x,y0=y<0?0:y,x1=x+w>c->w?c->w:x+w,y1=y+h>c->h?c->h:y+h;
    for(int yy=y0;yy<y1;++yy)for(int xx=x0;xx<x1;++xx)c->p[yy*c->stride+xx]=color;
}
static void bubble(Canvas *c,int x,int y,int w,int h,uint32_t color){
    rect(c,x+4,y,w-8,h,color);rect(c,x,y+4,w,h-8,color);
    rect(c,x+2,y+2,w-4,h-4,color);
}
static void glyph(Canvas *c,int x,int y,unsigned char ch,uint32_t color){
    unsigned char tmp[5]={0};const unsigned char *bits=tmp;ch=(unsigned char)toupper(ch);
    if(ch>='A'&&ch<='Z')bits=letters[ch-'A'];else if(ch>='0'&&ch<='9')bits=letters[26+ch-'0'];
    else switch(ch){
      case '>':tmp[0]=0x7f;tmp[1]=0x3e;tmp[2]=0x1c;tmp[3]=8;break;
      case ' ':break;case '.':tmp[2]=0x60;break;case ',':tmp[2]=0x60;tmp[1]=0x40;break;
      case ':':tmp[2]=0x36;break;case ';':tmp[2]=0x36;tmp[1]=0x40;break;
      case '!':tmp[2]=0x5f;break;case '?':tmp[0]=2;tmp[1]=1;tmp[2]=0x51;tmp[3]=9;tmp[4]=6;break;
      case '-':case '_':for(int i=0;i<5;++i)tmp[i]=ch=='-'?8:0x40;break;
      case '/':tmp[0]=0x60;tmp[1]=0x18;tmp[2]=6;tmp[3]=1;break;
      case '\'':tmp[2]=3;break;case '"':tmp[1]=3;tmp[3]=3;break;
      case '(':tmp[2]=0x3e;tmp[3]=0x41;break;case ')':tmp[1]=0x41;tmp[2]=0x3e;break;
      case '+':tmp[2]=0x3e;tmp[0]=tmp[1]=tmp[3]=tmp[4]=8;break;
      case '%':tmp[0]=0x63;tmp[1]=0x13;tmp[2]=8;tmp[3]=0x64;tmp[4]=0x63;break;
      case '&':tmp[0]=0x36;tmp[1]=0x49;tmp[2]=0x55;tmp[3]=0x22;tmp[4]=0x50;break;
      default:tmp[0]=tmp[4]=0x7f;tmp[1]=tmp[3]=0x41;tmp[2]=0x49;break;
    }
    for(int i=0;i<5;++i)for(int j=0;j<7;++j)if(bits[i]&(1u<<j))pixel(c,x+i,y+j,color);
}
static void text(Canvas *c,int x,int y,int width,const char *s,uint32_t color){
    if(!s||width<6)return;
    int n=width/6;
    for(int i=0;s[i]&&i<n;++i)glyph(c,x+i*6,y,(unsigned char)s[i],color);
}
/* Emits a bounded row and advances even on overlong words. */
static const char *line_next(const char *s,char out[128],int limit){
 int n=0,last=-1;if(limit<1)limit=1;if(limit>127)limit=127;
 while(*s==' ')s++;
 while(s[n]&&s[n]!='\n'&&n<limit){if(s[n]==' ')last=n;n++;}
 if(n==limit&&s[n]&&s[n]!='\n'&&last>0)n=last;
 memcpy(out,s,(size_t)n);out[n]=0;s+=n;if(*s=='\n')s++;return s;
}
static unsigned wrapped(Canvas *c,int x,int y,int width,int rows,const char *s,unsigned skip,uint32_t ink){
 unsigned row=0;char line[128];if(!s)return 0;
 while(*s){s=line_next(s,line,width/6);if(row>=skip&&row<skip+(unsigned)rows)text(c,x,y+(int)(row-skip)*10,width,line,ink);row++;}
 return row;
}
static void frame(Canvas *c,int x,int y,int w,int h,uint32_t fill){
 bubble(c,x,y,w,h,INK);rect(c,x+2,y+2,w-4,h-4,BLACK);rect(c,x+4,y+4,w-8,h-8,GOLD);rect(c,x+5,y+5,w-10,h-10,BLACK);rect(c,x+7,y+7,w-14,h-14,fill);
}
static void clear_image(void){stbi_image_free(image_rgba);image_rgba=NULL;image_key[0]=0;image_w=image_h=0;}
static void load_image(const char *key){
 if(!key||!root[0])return;
 if(!strcmp(image_key,key))return;
 clear_image();
 if(strlen(key)>=sizeof image_key||strstr(key,"..")||strchr(key,'/')||strchr(key,'\\')||strchr(key,':'))return;
 snprintf(image_key,sizeof image_key,"%s",key);char path[1200];int channels;
 if(snprintf(path,sizeof path,"%s/%s.png",root,key)>=(int)sizeof path)return;
 if(stbi_info(path,&image_w,&image_h,&channels)&&image_w>0&&image_h>0&&image_w<=2048&&image_h<=2048)
  image_rgba=stbi_load(path,&image_w,&image_h,&channels,4);
}
static void draw_image(Canvas *c,int x,int y,int w,int h){
 if(!image_rgba||w<=0||h<=0)return;
 int dw=w,dh=(int)((long long)image_h*w/image_w);
 if(dh>h){dh=h;dw=(int)((long long)image_w*h/image_h);}if(dw<1||dh<1)return;x+=(w-dw)/2;y+=(h-dh)/2;
 for(int j=0;j<dh;j++)for(int i=0;i<dw;i++){
  const unsigned char *p=image_rgba+((size_t)(j*image_h/dh)*image_w+(i*image_w/dw))*4;
  pixel(c,x+i,y+j,0xff000000u|((uint32_t)p[0]<<16)|((uint32_t)p[1]<<8)|p[2]);
 }
}
static size_t level_count(void){return provider.count?provider.count(provider.userdata,game_id):0;}
static unsigned selection_count(void){size_t n=state.screen==0?2:state.screen==2?DYNA_GUIDE_SECTIONS:level_count();return n>65535?65535:(unsigned)n;}
void dyna_extras_ui_set_level_provider(const DynaExtrasLevelProvider *p){if(p)provider=*p;else memset(&provider,0,sizeof provider);if(state.screen==1)state.selected=0;}
void dyna_extras_ui_reset(void){opened=0;memset(&state,0,sizeof state);clear_image();}
void dyna_extras_ui_init(unsigned game,const char *assets_root){dyna_extras_ui_reset();game_id=game==2?2:1;if(snprintf(root,sizeof root,"%s",assets_root?assets_root:"")>=(int)sizeof root)root[0]=0;}
void dyna_extras_ui_open(void){opened=1;}
int dyna_extras_ui_is_open(void){return opened;}
DynaExtrasUiState dyna_extras_ui_state(void){return state;}
void dyna_extras_ui_input(int semantic,int down,int repeat){
 if(!down||semantic<0||semantic>6)return;
 if(semantic==0){if(!repeat)opened=!opened;return;}if(!opened)return;
 if(semantic==1&&!repeat){
  if(state.image){state.image=0;return;}
  if(state.screen==0)opened=0;
  else if(state.screen==3){state.screen=2;state.selected=state.section;state.scroll=0;}
  else {state.selected=state.screen==2?1:0;state.screen=0;}return;
 }
 if(state.screen==3){
  if(semantic==2&&state.scroll)state.scroll--;
  if(semantic==3&&state.scroll<64)state.scroll++;
  if(semantic==4||semantic==5){state.page=dyna_guide_move(game_id,state.section,state.page,semantic==4?-1:1);state.scroll=0;state.image=0;}
  if(semantic==6&&!repeat){const DynaGuidePage *p=dyna_guide_page(game_id,state.section,state.page);if(p)load_image(p->capture_key);if(image_rgba)state.image=!state.image;}return;
 }
 unsigned n=selection_count();if(!n){state.selected=0;return;}if(state.selected>=n)state.selected=n-1;
 if(semantic==2&&state.selected)state.selected--;
 if(semantic==3&&state.selected+1<n)state.selected++;
 if(semantic==4)state.selected=state.selected>5?state.selected-5:0;
 if(semantic==5)state.selected=state.selected+5<n?state.selected+5:n-1;
 if(semantic!=6||repeat)return;
 if(state.screen==0){state.screen=state.selected==0?1:2;state.selected=0;}
 else if(state.screen==2){state.section=state.selected;state.page=state.scroll=0;state.screen=3;}
 else if(state.screen==1&&provider.item&&provider.launch){DynaExtrasLevel item={0};if(provider.item(provider.userdata,game_id,state.selected,&item)&&item.available&&provider.launch(provider.userdata,game_id,state.selected))opened=0;}
}
void dyna_extras_ui_render(uint32_t *pixels,int w,int h,int pitch_bytes){
 if(!opened||!pixels||w<160||h<144||w>8192||h>8192||pitch_bytes<0||pitch_bytes%4||pitch_bytes/4<w)return;
 Canvas c={pixels,w,h,pitch_bytes/4};int width=w-16;if(width>752)width=752;int x=(w-width)/2,top=8,bottom=h-8;
 if(game_id==1){
  rect(&c,0,0,w,h,SHADE);
  for(int y=2;y<h;y+=8)
   for(int col=(y&8)?2:6;col<w;col+=8)
    rect(&c,col,y,2,2,0xff562432u);
  bubble(&c,x,top,width,bottom-top,COPPER);
  bubble(&c,x+2,top+2,width-4,bottom-top-4,SHADE);
  bubble(&c,x+5,top+5,width-10,bottom-top-10,EMBER);
 }else frame(&c,x,top,width,bottom-top,GREEN);
 char heading[128];const char *title=state.screen==0?"EXTRAS":state.screen==1?"LEVEL SELECTION":state.screen==2?"GAME GUIDE":dyna_guide_section_name(state.section);
 snprintf(heading,sizeof heading,"DYNA BROTHERS %s / %s",game_id==1?"1":"2",title?title:"GUIDE");
 text(&c,x+13,top+13,width-26,heading,GOLD);rect(&c,x+12,top+25,width-24,1,INK);
 if(state.screen==3){
  const DynaGuidePage *p=dyna_guide_page(game_id,state.section,state.page);if(!p)return;load_image(p->capture_key);
  snprintf(heading,sizeof heading,"%u/%u  %s",state.page+1,dyna_guide_page_count(game_id,state.section),p->title);text(&c,x+13,top+33,width-26,heading,GOLD);
  if(state.image&&image_rgba){draw_image(&c,x+13,top+47,width-26,h-90);text(&c,x+13,bottom-17,width-26,"BACK: TEXT  LEFT/RIGHT: PAGE",GOLD);return;}
  int example_rows=width<400?4:3;int example_h=example_rows*10+17;int example_y=bottom-25-example_h;
  int rows=(example_y-(top+49)-5)/10;if(rows<1)rows=1;
  unsigned total=wrapped(&c,x+13,top+49,width-26,0,p->body,0,INK);unsigned max_scroll=total>(unsigned)rows?total-(unsigned)rows:0;if(state.scroll>max_scroll)state.scroll=max_scroll;
  wrapped(&c,x+13,top+49,width-26,rows,p->body,state.scroll,INK);
  frame(&c,x+10,example_y,width-20,example_h,BLACK);text(&c,x+20,example_y+10,width-40,"EXAMPLE",GOLD);
  wrapped(&c,x+20,example_y+21,width-40,example_rows-1,p->example,0,INK);
  const char *help=image_rgba?"L/R PAGE  UP/DOWN SCROLL  A IMAGE":"L/R PAGE  UP/DOWN SCROLL  BACK";
  text(&c,x+13,bottom-17,width-26,help,GOLD);return;
 }
 if(state.screen<=1 && notice[0])wrapped(&c,x+13,bottom-69,width-26,2,notice,0,GOLD);
 unsigned n=selection_count();if(state.selected>=n)state.selected=n?n-1:0;
 if(!n){wrapped(&c,x+16,top+48,width-32,3,"NO LEVELS AVAILABLE",0,INK);text(&c,x+13,bottom-17,width-26,"BACK: EXTRAS",GOLD);return;}
 int rows=(h-(state.screen==1&&notice[0]?146:112))/22;if(rows<1)rows=1;unsigned first=state.selected>=(unsigned)rows?state.selected-(unsigned)rows+1:0;
 if(state.screen==1 && n>(unsigned)rows){
  int track=rows*22,thumb=track*rows/(int)n;
  if(thumb<4)thumb=4;
  int offset=(track-thumb)*(int)first/(int)(n-(unsigned)rows);
  rect(&c,x+width-11,top+35,2,track,SHADE);
  rect(&c,x+width-11,top+35+offset,2,thumb,COPPER);
 }
 for(unsigned r=0;r<(unsigned)rows&&first+r<n;r++){
  unsigned i=first+r;const char *label=NULL;DynaExtrasLevel item={0};int available=1;
  if(state.screen==0)label=i==0?"LEVEL SELECTION":"GAME GUIDE";
  else if(state.screen==2)label=dyna_guide_section_name(i);
  else if(provider.item&&provider.item(provider.userdata,game_id,i,&item)){label=item.title;available=item.available;}
  if(!label)label="UNAVAILABLE";
  int y=top+39+(int)r*22;
  if(i==state.selected){
   rect(&c,x+11,y-4,width-25,18,game_id==1?SHADE:BLACK);
   text(&c,x+16,y,width-32,">",GOLD);
  }
  text(&c,x+28,y,width-43,label,i==state.selected?GOLD:available?INK:0xffe0ada2u);
 }
 if(state.screen==1&&provider.item){DynaExtrasLevel item={0};if(provider.item(provider.userdata,game_id,state.selected,&item))wrapped(&c,x+15,bottom-44,width-30,2,item.detail,0,INK);}
 text(&c,x+13,bottom-17,width-26,"UP/DOWN SELECT  A OPEN  B BACK",GOLD);
}

static uint32_t prompt_tint(uint32_t color,unsigned fade){
 return 0xff000000u|((((color>>16)&255)*fade/255)<<16)|
        ((((color>>8)&255)*fade/255)<<8)|((color&255)*fade/255);
}
/* Result screen extension: framebuffer only, native viewport anchored. */
void dyna_extras_ui_progress_prompt(uint32_t *pixels,int w,int h,int pitch_bytes,
                                    unsigned selected,const char *feedback){
 if(!pixels||w<320||h<224||w>8192||h>8192||pitch_bytes<0||pitch_bytes%4||
    pitch_bytes/4<w||pitch_bytes>INT_MAX/h)return;
 Canvas c={pixels,w,h,pitch_bytes/4};int x=(w-320)/2+80,y=(h-224)/2+176;
 /* Sample the original prompt before replacing it. Its white text/border
  * follows the native result fade, so the host panel must fade with it. */
 unsigned fade=0;
 for(int py=y+9;py<y+25;py++)for(int px=x+4;px<x+158;px++){
  uint32_t value=c.p[py*c.stride+px];
  for(unsigned shift=0;shift<24;shift+=8){unsigned channel=(value>>shift)&255;if(channel>fade)fade=channel;}
 }
 const uint32_t cyan=prompt_tint(0xff66ffffu,fade),ink=prompt_tint(INK,fade);
 rect(&c,x,y,160,45,ink);rect(&c,x+1,y+1,158,43,BLACK);
 rect(&c,x+3,y+3,154,39,cyan);rect(&c,x+4,y+4,152,37,BLACK);
 selected=selected==1?1:0;
 for(int row=0;row<7;row++){
  int distance=row>3?row-3:3-row;
  for(int column=0;column<=3-distance;column++)pixel(&c,x+10+column,y+8+(int)selected*10+row,cyan);
 }
 text(&c,x+23,y+8,128,"CONTINUE",selected==0?cyan:ink);
 text(&c,x+23,y+18,128,"SAVE GAME",selected==1?cyan:ink);
 if(feedback&&*feedback)text(&c,x+13,y+30,134,feedback,ink);
}

void dyna_extras_ui_ending_save_prompt(uint32_t *pixels,int w,int h,int pitch_bytes,
                                      const char *feedback){
 if(!pixels||w<320||h<224||w>8192||h>8192||pitch_bytes<0||pitch_bytes%4||
    pitch_bytes/4<w||pitch_bytes>INT_MAX/h)return;
 Canvas c={pixels,w,h,pitch_bytes/4};int x=(w-320)/2+80,y=(h-224)/2+201;
 const uint32_t cyan=0xff66ffffu;
 rect(&c,x,y,160,22,INK);rect(&c,x+1,y+1,158,20,BLACK);
 rect(&c,x+3,y+3,154,16,cyan);rect(&c,x+4,y+4,152,14,BLACK);
 for(int row=0;row<7;row++){
  int distance=row>3?row-3:3-row;
  for(int column=0;column<=3-distance;column++)pixel(&c,x+9+column,y+7+row,cyan);
 }
 const char *label=feedback&&*feedback?feedback:"SAVE GAME";
 if(!strcmp(label,"SAVE FAILED"))label="SAVE FAILED - RETRY";
 text(&c,x+22,y+7,130,label,INK);
}
