#include "db1_title_menu_art.h"
#include "video/genesis_dac.h"
#include <string.h>
#define ROW_CAP 8192
static unsigned word(const GVDP *v,unsigned a){return ((unsigned)v->vram[a&65535]<<8)|v->vram[(a+1)&65535];}
static unsigned nibble(const GVDP *v,unsigned tile,int x,int y){
 unsigned a=(tile*32u+(unsigned)y*4+(unsigned)x/2)&65535;
 return (v->vram[a]>>((x&1)?0:4))&15;
}
static unsigned pattern(const GVDP *v,unsigned attr,int x,int y){
 if(attr&0x800)x=7-x;
 if(attr&0x1000)y=7-y;
 return nibble(v,attr&2047,x,y);
}
static int wrap(int x,int period){int r=x%period;return r<0?r+period:r;}
static int foreground(const GVDP *v,int x,int line){
 static const int sizes[4]={32,64,0,128};
 int wt=sizes[v->reg[16]&3],ht=sizes[(v->reg[16]>>4)&3];
 unsigned mode=v->reg[11]&3,hi=mode==0?0:mode==2?(unsigned)line&~7u:(unsigned)line;
 unsigned scroll=word(v,((v->reg[13]&63)<<10)+hi*4)&1023;
 int col=x/16;if(x<0&&x%16)--col;
 if(col<0)col=0;
 if(col>19)col=19;
 int sx=wrap(x-(int)scroll,wt*8),sy=wrap(line+(v->vsram[(v->reg[11]&4)?col*2:0]&1023),ht*8);
 unsigned attr=word(v,((v->reg[2]&56)<<10)+2u*((unsigned)(sy/8)*(unsigned)wt+(unsigned)sx/8));
 return pattern(v,attr,sx&7,sy&7)!=0;
}
static int identify(const GVDP *v,int *y){
 unsigned sat=(v->reg[5]&127)<<9;
 static const int xs[4]={116,195,115,194};
 for(unsigned i=0;i<4;i++){
  unsigned a=sat+i*8,attr=word(v,a+4);
  int sy=(int)(word(v,a)&511)-128;
  if(i==0)*y=sy;
  if((v->vram[(a+2)&65535]&15)||
     (v->vram[(a+3)&65535]&127)!=i+1||
     (int)(word(v,a+6)&511)-128!=xs[i]||sy!=*y+(i>=2)||
     (attr&2047)!=(i<2?0x32cu:0x32du)||
     (attr&0x1800)!=(i&1?0x800u:0u)||
     (attr&0x6000)!=(i<2?0x2000u:0u))return 0;
 }
 return *y>=151&&*y<=187&&(*y-151)%12==0;
}
static int other_sprites(const GVDP *v,int line,int width,int left,unsigned char *mask){
 unsigned sat=(v->reg[5]&127)<<9,index=4;
 unsigned char seen[80]={0};
 for(unsigned guard=0;guard<80;guard++){
  if(index<4||index>=80||seen[index])return 0;
  seen[index]=1;
  unsigned a=sat+index*8,attr=word(v,a+4),size=v->vram[(a+2)&65535];
  int sx=(int)(word(v,a+6)&511)-128,sy=(int)(word(v,a)&511)-128;
  int wt=(int)((size>>2)&3)+1,ht=(int)(size&3)+1,row=line-sy;
  if(row>=0&&row<ht*8){
   if(attr&0x1000)row=ht*8-1-row;
   for(int x=0;x<wt*8;x++){
    int64_t dx=(int64_t)left+sx+x;if(dx<0||dx>=width)continue;
    int column=attr&0x800?wt*8-1-x:x;
    unsigned tile=((attr&2047)+(unsigned)(column/8*ht+row/8))&2047;
    if(nibble(v,tile,column&7,row&7))mask[(size_t)dx]=1;
   }
  }
  index=v->vram[(a+3)&65535]&127;
  if(!index)return 1;
 }
 return 0;
}
static void draw(const GVDP *v,int line,uint32_t *out,int width,int left,
 unsigned attr,int x,int y,const unsigned char *protected){
 int row=line-y;if(row<0||row>=8)return;
 for(int column=0;column<8;column++){
  int64_t dx=(int64_t)left+x+column;if(dx<0||dx>=width)continue;
  unsigned n=pattern(v,attr,column,row);
  if(n&&!protected[(size_t)dx]&&!foreground(v,x+column,line))
   out[(size_t)dx]=genesis_dac_cram_to_argb(v->cram[((attr>>9)&48)+n],GENESIS_DAC_NORMAL);
 }
}
int db1_title_menu_art_row(const GVDP *v,int line,uint32_t *out,int width,int left,
 unsigned selected,int modal,Db1TitleMenuBackground background){
 if(!v||!out||!background||width<=0||width>ROW_CAP||selected>4||line<0||line>=224||
    !(v->reg[1]&64)||!(v->reg[12]&1)||(v->reg[12]&14)||
    (v->reg[16]&3)==2||((v->reg[16]>>4)&3)==2||(v->reg[11]&3)==1)return 0;
 int old_y=0;
 if(!identify(v,&old_y))return 0;
 if(!(line>=200&&line<=207)&&
    !(selected==4&&((line>=old_y&&line<=old_y+8)||(line>=199&&line<=207))))return 0;
 unsigned char protected[ROW_CAP]={0};
 if(!other_sprites(v,line,width,left,protected))return 0;
 unsigned sat=(v->reg[5]&127)<<9;
 if(selected==4&&!modal){
  unsigned char cursor[ROW_CAP]={0};
  for(unsigned i=0;i<4;i++){
   unsigned a=sat+i*8,attr=word(v,a+4);
   int x=(int)(word(v,a+6)&511)-128,y=(int)(word(v,a)&511)-128,row=line-y;
   if(row<0||row>=8)continue;
   for(int column=0;column<8;column++){
    int64_t dx=(int64_t)left+x+column;
    if(dx>=0&&dx<width&&pattern(v,attr,column,row))cursor[(size_t)dx]=1;
   }
  }
  for(int dx=0;dx<width;dx++)if(cursor[dx]&&!protected[dx]){
   int64_t x=(int64_t)dx-left;
   if(x>=0&&x<320&&!foreground(v,(int)x,line))out[dx]=background(v,(int)x,line);
  }
  /* Lower SAT index owns overlaps; draw shadows before the main arrows. */
  for(int i=3;i>=0;i--){
   unsigned a=sat+(unsigned)i*8;
   int x=(int)(word(v,a+6)&511)-128;
   draw(v,line,out,width,left,word(v,a+4),x,199+(i>=2),protected);
  }
 }
 static const unsigned letters[6]={0x30e,0x321,0x31d,0x31b,0x30a,0x31c};
 /* Modal (Extras panel open): fall back to the same unselected white bank
  * every other row already uses, instead of staying on the hover-yellow
  * bank. Matches how a row's highlight disappears once its action opens. */
 for(unsigned i=0;i<6;i++)draw(v,line,out,width,left,letters[i]|((selected==4&&!modal)?0x2000u:0u),136+(int)i*8,200,protected);
 return 1;
}
