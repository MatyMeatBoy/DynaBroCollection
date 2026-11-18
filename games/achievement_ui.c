/* Game-owned CPU compositor. No guest memory or input state is accessed. */
#include "achievement_ui.h"
#include "achievements.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 256
#include "../upstream/recomp-ui/src/third_party/stb_image.h"

typedef struct {uint32_t *p;int w,h,stride;} Canvas;
typedef struct {char path[256];unsigned char *rgba;int w,h;} Badge;
static Badge badges[8];
static unsigned next_badge;
static char root[1024];
static int opened;
static size_t selected,first;
static uint64_t toast_start;
static uint32_t toast_id;
#define INK 0xffe7f1fbu
#define MUTED 0xff99b1c8u
#define BLUE 0xff5acdf5u
#define GOLD 0xffffd279u

/* Five columns, low bit at the top. Lowercase is displayed as uppercase. */
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
    if(!s||width<6)return;int n=width/6;
    for(int i=0;s[i]&&i<n;++i)glyph(c,x+i*6,y,(unsigned char)s[i],color);
}
/* Word wrapping preserves words where possible, bounds every output row. */
static void wrapped(Canvas *c,int x,int y,int width,const char *s,int rows,uint32_t color){
    if(!s||width<6)return;int limit=width/6;if(limit>127)limit=127;
    for(int r=0;r<rows&&*s;++r){
        while(*s==' ')++s;int n=0,last=-1;
        while(s[n]&&s[n]!='\n'&&n<limit){if(s[n]==' ')last=n;++n;}
        if(n==limit&&s[n]&&s[n]!='\n'&&last>0)n=last;
        char line[128];memcpy(line,s,(size_t)n);line[n]=0;text(c,x,y+r*10,width,line,color);
        s+=n;if(*s=='\n')++s;
    }
}
static void free_badges(void){for(unsigned i=0;i<8;++i){stbi_image_free(badges[i].rgba);memset(&badges[i],0,sizeof badges[i]);}next_badge=0;}
static Badge *badge(const char *relative){
    if(!root[0]||!relative||!*relative||relative[0]=='/'||relative[0]=='\\'||strchr(relative,':')||strstr(relative,"..")||strlen(relative)>=256)return NULL;
    for(unsigned i=0;i<8;++i)if(!strcmp(badges[i].path,relative))return &badges[i];
    Badge *b=&badges[next_badge++%8];stbi_image_free(b->rgba);memset(b,0,sizeof *b);
    snprintf(b->path,sizeof b->path,"%s",relative);char filename[1300];
    if(snprintf(filename,sizeof filename,"%s/%s",root,relative)>=(int)sizeof filename)return b;
    int channels=0;
    if(stbi_info(filename,&b->w,&b->h,&channels)&&b->w>0&&b->h>0&&b->w<=256&&b->h<=256)
        b->rgba=stbi_load(filename,&b->w,&b->h,&channels,4);
    return b;
}
static void draw_badge(Canvas *c,int x,int y,int size,const DynaAchievementItem *item,int unlocked){
    Badge *b=badge(unlocked?item->badge_file_unlocked:item->badge_file_locked);
    bubble(c,x-2,y-2,size+4,size+4,unlocked?GOLD:0xff304b62u);
    if(!b||!b->rgba){text(c,x+(size-6)/2,y+(size-7)/2,6,unlocked?"*":"?",unlocked?GOLD:MUTED);return;}
    for(int j=0;j<size;++j)for(int i=0;i<size;++i){
        unsigned char *p=b->rgba+((j*b->h/size)*b->w+i*b->w/size)*4;
        int dx=x+i,dy=y+j;if(dx<0||dy<0||dx>=c->w||dy>=c->h)continue;
        uint32_t old=c->p[dy*c->stride+dx],a=p[3],r=(p[0]*a+((old>>16)&255)*(255-a))/255;
        uint32_t g=(p[1]*a+((old>>8)&255)*(255-a))/255,bb=(p[2]*a+(old&255)*(255-a))/255;
        pixel(c,dx,dy,0xff000000u|(r<<16)|(g<<8)|bb);
    }
}
static const DynaAchievementItem *find_id(uint32_t id){
    for(size_t i=0;i<dyna_achievements_count();++i){const DynaAchievementItem *p=dyna_achievements_item(i);if(p&&p->id==id)return p;}return NULL;
}
static void poll_toast(void){
    if(opened)return;
    if(toast_id&&SDL_GetTicks64()-toast_start>=5000)toast_id=0;
    if(!toast_id){DynaAchievementUnlock event;if(dyna_achievements_pop_unlock(&event)){toast_id=event.id;toast_start=SDL_GetTicks64();}}
}
void dyna_achievement_ui_reset(void){DynaAchievementUnlock discarded;while(dyna_achievements_pop_unlock(&discarded)){}opened=0;selected=first=0;toast_id=0;toast_start=0;free_badges();}
void dyna_achievement_ui_init(const char *assets_root){dyna_achievement_ui_reset();if(snprintf(root,sizeof root,"%s",assets_root?assets_root:"")>=(int)sizeof root)root[0]=0;}
void dyna_achievement_ui_open(void){opened=1;size_t n=dyna_achievements_count();if(selected>=n)selected=0;}
int dyna_achievement_ui_is_open(void){return opened;}
int dyna_achievement_ui_active(void){poll_toast();return opened||toast_id!=0;}
void dyna_achievement_ui_input(int semantic,int down,int repeat){
    if(!down)return;
    if(semantic==0&&!repeat){opened=!opened;return;}
    if(!opened)return;
    if(semantic==1&&!repeat){opened=0;return;}
    size_t n=dyna_achievements_count();if(!n){selected=first=0;return;}
    if(selected>=n)selected=n-1;
    if(semantic==2&&selected) --selected;
    if(semantic==3&&selected+1<n) ++selected;
    if(semantic==4)selected=selected>3?selected-3:0;
    if(semantic==5)selected=selected+3<n?selected+3:n-1;
}
void dyna_achievement_ui_render(uint32_t *pixels,int w,int h,int pitch_bytes){
    if(!pixels||w<96||h<96||w>8192||h>8192||pitch_bytes<0||pitch_bytes%4||pitch_bytes/4<w)return;
    poll_toast();Canvas c={pixels,w,h,pitch_bytes/4};
    if(!opened){
        const DynaAchievementItem *item=toast_id?find_id(toast_id):NULL;if(!item)return;
        int width=w-16;if(width>312)width=312;int x=(w-width)/2;
        bubble(&c,x,8,width,54,0xff304b62u);bubble(&c,x+1,9,width-2,52,0xff122130u);
        draw_badge(&c,x+9,19,30,item,1);text(&c,x+48,17,width-58,"ACHIEVEMENT UNLOCKED",GOLD);
        wrapped(&c,x+48,30,width-58,item->title,2,INK);return;
    }
    /* Dim only the host buffer, preserving its pitch padding and alpha. */
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){uint32_t p=pixels[y*c.stride+x];pixels[y*c.stride+x]=(p&0xff000000u)|((p&0x00fefefeu)>>1);}
    int width=w-16;if(width>600)width=600;int x=(w-width)/2,top=8,bottom=h-8;
    bubble(&c,x,top,width,bottom-top,0xff304b62u);bubble(&c,x+1,top+1,width-2,bottom-top-2,0xff09111bu);
    size_t n=dyna_achievements_count(),unlocked=0;unsigned points=0,total=0;
    for(size_t i=0;i<n;++i){const DynaAchievementItem *item=dyna_achievements_item(i);if(item){total+=item->points;if(item->unlocked_at){++unlocked;points+=item->points;}}}
    char summary[100];snprintf(summary,sizeof summary,"%zu/%zu UNLOCKED  %u/%u POINTS",unlocked,n,points,total);
    text(&c,x+12,top+11,width-24,"ACHIEVEMENTS",BLUE);text(&c,x+12,top+24,width-24,summary,MUTED);
    if(!n){text(&c,x+12,top+49,width-24,"NO ACHIEVEMENTS AVAILABLE",MUTED);return;}
    if(selected>=n)selected=n-1;
    int row_h=36,list_y=top+42,rows=(h-112)/row_h;if(rows<1)rows=1;
    if(first>selected)first=selected;if(selected>=first+(size_t)rows)first=selected-(size_t)rows+1;
    for(int r=0;r<rows&&first+(size_t)r<n;++r){
        size_t index=first+(size_t)r;const DynaAchievementItem *item=dyna_achievements_item(index);if(!item)continue;
        int y=list_y+r*row_h;if(index==selected)bubble(&c,x+6,y-2,width-12,34,0xff17334au);
        draw_badge(&c,x+13,y+2,24,item,item->unlocked_at!=0);
        text(&c,x+47,y+2,width-60,item->title,item->unlocked_at?INK:MUTED);
        char status[100];snprintf(status,sizeof status,"%s  %u PTS",item->unlocked_at?"UNLOCKED":item->qualified?"LOCKED":"LOCKED / NOT YET TRACKED",item->points);
        text(&c,x+47,y+15,width-60,status,item->unlocked_at?GOLD:MUTED);
    }
    const DynaAchievementItem *item=dyna_achievements_item(selected);
    if(item)wrapped(&c,x+12,bottom-49,width-24,item->description,3,INK);
    snprintf(summary,sizeof summary,"UP/DOWN SELECT  LEFT/RIGHT PAGE  BACK CLOSE");
    text(&c,x+12,bottom-13,width-24,summary,BLUE);
    if(n>(size_t)rows){int track=rows*row_h-4,bar=track*rows/(int)n;if(bar<4)bar=4;int y=list_y+(track-bar)*(int)first/(int)(n-rows);rect(&c,x+width-6,list_y,2,track,0xff304b62u);rect(&c,x+width-6,y,2,bar,BLUE);}
}
