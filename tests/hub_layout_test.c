#include "hub_layout.h"
#include <assert.h>
#include <stdio.h>
static int inside(DynaHubRect a,int w,int h){return a.x>=0&&a.y>=0&&a.w>0&&a.h>0&&a.x+a.w<=w&&a.y+a.h<=h;}
static int overlap(DynaHubRect a,DynaHubRect b){return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
int main(void){
 int sizes[][2]={{1920,1080},{2560,1440},{1366,768},{800,600}};
 unsigned dpis[]={96,120,144};
 for(unsigned s=0;s<4;s++)for(unsigned d=0;d<3;d++){
  unsigned dpi=dpis[d];int fw=(16*(int)dpi+48)/96,fh=(39*(int)dpi+48)/96;
  DynaHubRect work={-100,30,sizes[s][0],sizes[s][1]},window,minimum;
  dyna_hub_window_bounds(work,dpi,fw,fh,0,&window);
  dyna_hub_window_bounds(work,dpi,fw,fh,1,&minimum);
  assert(window.x>=work.x&&window.y>=work.y&&window.x+window.w<=work.x+work.w&&window.y+window.h<=work.y+work.h);
  assert(minimum.w<=window.w&&minimum.h<=window.h);
  for(int compact=0;compact<2;compact++){
   DynaHubRect a=compact?minimum:window;DynaHubLayout l;
   int w=a.w-fw,h=a.h-fh;dyna_hub_layout(w,h,dpi,&l);
   DynaHubRect all[]={l.banner,l.version,l.cards[0],l.cards[1],l.settings[0],l.settings[1],l.note,l.status};
   for(unsigned i=0;i<8;i++){assert(inside(all[i],w,h));for(unsigned j=i+1;j<8;j++)assert(!overlap(all[i],all[j]));}
   assert(l.ui_scale_milli>0&&l.ui_scale_milli<=(int)dpi*1000/96);
  }
 }
 DynaHubRect w;dyna_hub_window_bounds((DynaHubRect){0,0,1920,1080},120,20,49,0,&w);
 assert(w.w==1395&&w.h==824); /* 1375x775 client =1100x620 DIP at125%. */
 puts("hub layout:24 work-area/DPI/default/minimum cases passed");return 0;
}
