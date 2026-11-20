#include "db2_menu_labels.h"
#include "db2_bio_overlay.h"
#include "video/genesis_vdp.h"
#include "video/genesis_dac.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

enum { MAX_LABELS=4, MODE_TOP=0x17f, MODE_BOTTOM=0x18f };
static const char *const words[MAX_LABELS]={"PRACTICE","STORY","ORIGINAL","VERSUS"};

typedef struct {
 int x0,x1,y0;            /* native screen rectangle, 16 lines tall */
 unsigned fill;           /* plate fill name entry */
 uint8_t ink[16];         /* CRAM index per scanline */
 uint8_t back[16];        /* plate colour per scanline */
 uint8_t shadow;
 int ncells,row,word;
 uint16_t top[24],bot[24]; /* the original plate art, to find what (clouds) is drawn over it */
} Label;
static Label labels[MAX_LABELS];
static int label_count,english;

void db2_menu_labels_set_english(int on){english=on!=0;}

static unsigned rd(const GVDP *v,unsigned a){a&=0xffff;return ((unsigned)v->vram[a]<<8)|v->vram[(a+1)&0xffff];}
static int cells(unsigned bits){return bits==1?64:bits==3?128:32;}
static unsigned tile_index(const GVDP *v,unsigned name,int x,int y){
 if(name&0x800)x=7-x;
 if(name&0x1000)y=7-y;
 unsigned b=v->vram[((name&2047)*32+(unsigned)y*4+(unsigned)x/2)&0xffff];
 unsigned c=(x&1)?b&15:b>>4;
 return c?(((name>>9)&48)|c):0;
}
static unsigned luma(const GVDP *v,unsigned i){
 uint32_t c=genesis_dac_cram_to_argb(v->cram[i&63],0);
 return ((c>>16)&255)*3+((c>>8)&255)*6+(c&255);
}
static void flatten_versus_ink(const GVDP *v,Label *label){
 unsigned votes[64]={0},chosen=0;
 for(int y=0;y<16;y++){
  unsigned ink=label->ink[y]&63;
  if(!ink)continue;
  uint16_t color=v->cram[ink];
  unsigned r=(color>>1)&7,g=(color>>5)&7,b=(color>>9)&7;
  votes[ink]+=g>r&&g>b?17u:1u;
  if(votes[ink]>votes[chosen])chosen=ink;
 }
 if(chosen)memset(label->ink,(int)chosen,sizeof label->ink);
}

void db2_menu_labels_frame(const GVDP *v){
 label_count=0;
 if(!english||!v)return;
 int wt=cells(v->reg[16]&3),ht=cells((v->reg[16]>>4)&3);if(wt*ht>4096)ht=4096/wt;
 int pw=wt*8,ph=ht*8;
 unsigned base=((unsigned)v->reg[2]&0x38)<<10,hbase=((unsigned)v->reg[13]&63)<<10;
 int hs=(int)(rd(v,hbase)&1023),vs=v->vsram[0]&1023;
 Label found[8];int n=0;
 for(int r=0;r<ht&&n<8;r++)for(int c=1;c+6<wt&&n<8;c++){
  unsigned at=base+(unsigned)(r*wt+c)*2,below=base+(unsigned)(((r+1)%ht)*wt+c)*2;
  int ok=1;
  for(int k=0;k<6&&ok;k++)ok=(rd(v,at+2u*k)&2047)==MODE_TOP+k&&(rd(v,below+2u*k)&2047)==MODE_BOTTOM+k;
  if(!ok)continue;
  /* Find the actual left frame edge. The far edge can still be empty while
   * the guest brings the plate in after skipping the intro. */
  int start=c;
  while(start>0&&c-start<14){
   unsigned offset=2u*(unsigned)(start-1-c);
   if((rd(v,at+offset)&2047)==0x203&&(rd(v,below+offset)&2047)==0x213)break;
   --start;
  }
  if(start==0||c-start==14)continue;
  unsigned edge=rd(v,at+2u*(unsigned)(start-1-c));
  unsigned first=2u*(unsigned)(start-c);
  if((rd(v,at+first)&2047)==0x265&&(rd(v,below+first)&2047)==0x265)++start;
  Label *l=found+n++;memset(l,0,sizeof *l);l->row=r;
  int sx=((start*8+hs)%pw+pw)%pw;if(sx>=320+64)sx-=pw;
  int sy=((r*8-vs)%ph+ph)%ph;if(sy>=224+32)sy-=ph;
  l->x0=sx;l->x1=sx+(c+6-start)*8;l->y0=sy;
  /* The stone fill pattern is already loaded when the first plate arrives.
   * The cursor scroll streams in the cells that refer to it much later. */
  l->fill=(edge&0x6000)|0x265;
  l->ncells=c+6-start>24?24:c+6-start;
  for(int k=0;k<l->ncells;k++){l->top[k]=(uint16_t)rd(v,at+2u*(unsigned)(start+k-c));l->bot[k]=(uint16_t)rd(v,below+2u*(unsigned)(start+k-c));}
  /* Ink: dominant non-fill colour per scanline of the Japanese label art. */
  uint8_t fill_colors[64]={0};
  unsigned ink_reference=rd(v,at+12);
  if(!(ink_reference&2047))ink_reference=edge;
  for(int y=0;y<8;y++)for(int x=0;x<8;x++)
   fill_colors[tile_index(v,ink_reference,x,y)]=1;
  unsigned dark=0,dark_l=~0u;
  for(int yy=0;yy<16;yy++){
   unsigned hist[64]={0};
   unsigned row_base=yy<8?at:below;
   for(int cc=start;cc<c+6;cc++){
    unsigned name=rd(v,row_base+(unsigned)(cc-c)*2);
    for(int x=0;x<8;x++){unsigned i=tile_index(v,name,x,yy&7);if(i&&!fill_colors[i])hist[i]++;}
   }
   unsigned best=0,bn=0;
   for(unsigned i=1;i<64;i++){
    if(hist[i]>bn){bn=hist[i];best=i;}
    if(hist[i]){unsigned L=luma(v,i);if(L<dark_l){dark_l=L;dark=i;}}
   }
   l->ink[yy]=(uint8_t)best;
  }
  /* The darkest colour is the outline; the ink is the brightest dominant. */
  for(int yy=0;yy<16;yy++)if(l->ink[yy]==dark){
   unsigned hist[64]={0},row_base=yy<8?at:below;
   for(int cc=start;cc<c+6;cc++){unsigned name=rd(v,row_base+(unsigned)(cc-c)*2);for(int x=0;x<8;x++){unsigned i=tile_index(v,name,x,yy&7);if(i&&!fill_colors[i]&&i!=dark)hist[i]++;}}
   unsigned best=dark,bn=0;for(unsigned i=1;i<64;i++)if(hist[i]>bn){bn=hist[i];best=i;}
   l->ink[yy]=(uint8_t)best;
  }
  l->shadow=(uint8_t)dark;
  /* Plate colour: the most frequent colour of each scanline across the whole
   * label (letters are thin, so the plate dominates), never the ink. */
  for(int yy=0;yy<16;yy++){
   unsigned hist[64]={0},row_base=yy<8?at:below;
   for(int cc=start;cc<c+6;cc++){unsigned name=rd(v,row_base+(unsigned)(cc-c)*2);for(int x=0;x<8;x++)hist[tile_index(v,name,x,yy&7)]++;}
   unsigned best=0,bn=0;for(unsigned i=1;i<64;i++)if(i!=l->ink[yy]&&i!=dark&&hist[i]>bn){bn=hist[i];best=i;}
   if(!bn)best=l->ink[yy];
   l->back[yy]=(uint8_t)best;
  }
 }
 {static int tr=-1;if(tr<0){const char *e=getenv("DYNA_TRACE_LABELS");tr=e&&e[0]=='1';}
  if(tr){fprintf(stderr,"[labels] n=%d",n);for(int i=0;i<n;i++)fprintf(stderr," (r=%d x0=%d y0=%d)",found[i].row,found[i].x0,found[i].y0);fprintf(stderr,"\n");}}
 /* The plates are streamed into the plane as the camera scrolls, so only some
  * are present mid-transition. They sit on fixed nametable rows 3,6,9,12. */
 unsigned seen=0;
 for(int i=0;i<n;i++){
  Label *label=found+i;
  int row=label->row-3;
  if(row<0||row%3||row/3>=MAX_LABELS)continue;
  label->word=row/3;
  unsigned mask=1u<<label->word;
  if(seen&mask)continue;
  seen|=mask;
  if(label->word==3)flatten_versus_ink(v,label);
  labels[label_count++]=*label;
 }
}

/* 16x16 masks in the style of the authored PASSWORD / OPTIONS plates:
 * two-pixel strokes inside columns 2..13, rows 1..14. P A S O R D are copied
 * from the PASSWORD art; the remaining letters are drawn on the same grid. */
static const char *const glyph_rows[][14]={
 /* P */ {"..ccccccccccc...","..cccccccccccc..","..cc.......ccc..","..cc........cc..","..cc........cc..","..cc.......ccc..","..cccccccccccc..","..ccccccccccc...","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............"},
 /* A */ {".....cccccc.....","....ccc..ccc....","...ccc....ccc...","...cc......cc...","..cc........cc..","..cc........cc..","..cc........cc..","..cccccccccccc..","..cccccccccccc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc.."},
 /* S */ {"...cccccccccc...","..cccccccccccc..","..ccc.......cc..","..cc............","..cc............","..ccc...........","...cccccccccc...","....cccccccccc..","............ccc.","............ccc.","............ccc.","..cc.......ccc..","..cccccccccccc..","...cccccccccc..."},
 /* O */ {".....cccccc.....","...cccccccccc...","...ccc....ccc...","..ccc......ccc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..ccc......ccc..","...ccc....ccc...","...cccccccccc...",".....cccccc....."},
 /* R */ {"..ccccccccccc...","..cccccccccccc..","..cc.......ccc..","..cc........cc..","..cc........cc..","..cc.......ccc..","..ccccccccccc...","..cccccccccc....","..cc......ccc...","..cc.......ccc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc.."},
 /* D */ {"..cccccccc......","..cccccccccc....","..cc.....ccc....","..cc......ccc...","..cc.......cc...","..cc.......cc...","..cc.......cc...","..cc.......cc...","..cc.......cc...","..cc.......cc...","..cc......ccc...","..cc.....ccc....","..cccccccccc....","..cccccccc......"},
 /* C */ {"...cccccccccc...","..cccccccccccc..","..ccc.......cc..","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..ccc.......cc..","..cccccccccccc..","...cccccccccc..."},
 /* E */ {"..cccccccccccc..","..cccccccccccc..","..cc............","..cc............","..cc............","..cc............","..cccccccccc....","..cccccccccc....","..cc............","..cc............","..cc............","..cc............","..cccccccccccc..","..cccccccccccc.."},
 /* G */ {"...cccccccccc...","..cccccccccccc..","..ccc.......cc..","..cc............","..cc............","..cc............","..cc....cccccc..","..cc....cccccc..","..cc........cc..","..cc........cc..","..cc........cc..","..ccc.......cc..","..cccccccccccc..","...cccccccccc..."},
 /* I */ {"....cccccccc....","....cccccccc....",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......","....cccccccc....","....cccccccc...."},
 /* L */ {"..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cc............","..cccccccccccc..","..cccccccccccc.."},
 /* N */ {"..cc........cc..","..ccc.......cc..","..cccc......cc..","..ccccc.....cc..","..cc.ccc....cc..","..cc..ccc...cc..","..cc...ccc..cc..","..cc....ccc.cc..","..cc.....ccccc..","..cc......cccc..","..cc.......ccc..","..cc........cc..","..cc........cc..","..cc........cc.."},
 /* T */ {"..cccccccccccc..","..cccccccccccc..",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc......."},
 /* U */ {"..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..ccc......ccc..","...ccc....ccc...","...cccccccccc...",".....cccccc....."},
 /* V */ {"..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","..cc........cc..","...cc......cc...","...cc......cc...","....cc....cc....","....cc....cc....",".....cc..cc.....",".....cc..cc.....","......cccc......",".......cc......."},
 /* Y */ {"..cc........cc..","..cc........cc..","..cc........cc..","...cc......cc...","....cc....cc....",".....cc..cc.....","......cccc......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc.......",".......cc......."},
};
static const char glyph_letters[]="PASORDCEGILNTUVY";
static int glyph_on(const char *word,int len,int px,int py){
 int ch=px>>4;if(px<0||ch>=len||py<1||py>14)return 0;
 const char *at=strchr(glyph_letters,word[ch]);if(!at||!*at)return 0;
 return glyph_rows[at-glyph_letters][py-1][px&15]=='c';
}
static int outline_on(const char *word,int len,int px,int py){
 for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)if(glyph_on(word,len,px+dx,py+dy))return 1;
 return 0;
}

void db2_menu_labels_row(const GVDP *v,int line,uint32_t *out,int width,int origin_x){
 if(!english||!v||!out)return;
 for(int k=0;k<label_count;k++){
  const Label *l=labels+k;
  if(line<l->y0||line>=l->y0+16)continue;
  const char *word=words[l->word];
  int yy=line-l->y0,len=(int)strlen(word);
  int tx=l->x0+((l->x1-l->x0)-len*16)/2;
  uint32_t ink=genesis_dac_cram_to_argb(v->cram[l->ink[yy]&63],0);
  uint32_t shadow=genesis_dac_cram_to_argb(v->cram[l->shadow&63],0);
  for(int x=l->x0;x<l->x1;x++){
   int dx=x+origin_x;if(dx<0||dx>=width)continue;
   int cell=(x-l->x0)>>3;
   if(cell>=l->ncells)continue;
   unsigned old=tile_index(v,yy<8?l->top[cell]:l->bot[cell],(x-l->x0)&7,yy&7);
   /* Keep transparent plate edges and scenery (including sprites) intact. */
   if(!old||out[dx]!=genesis_dac_cram_to_argb(v->cram[old&63],0))continue;
   unsigned fill=tile_index(v,l->fill,(x-l->x0)&7,yy&7);
   if(!fill)fill=l->back[yy];
   if(!fill)continue;
   uint32_t c=genesis_dac_cram_to_argb(v->cram[fill&63],0);
   if(glyph_on(word,len,x-tx,yy))c=ink;
   else if(outline_on(word,len,x-tx,yy))c=shadow;
   out[dx]=c;
  }
 }
}
