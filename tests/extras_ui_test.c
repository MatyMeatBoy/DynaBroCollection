#include "extras_ui.h"
#include "guide_content.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static int launches;
static size_t count(void *u,unsigned g){(void)u;assert(g==1||g==2);return 40;}
static int item(void *u,unsigned g,size_t i,DynaExtrasLevel *out){(void)u;(void)g;if(i>=40)return 0;out->title="VERIFIED STAGE";out->detail="Provider supplied description";out->available=i==1;return 1;}
static int launch(void *u,unsigned g,size_t i){(void)u;(void)g;assert(i==1);launches++;return 1;}
static void key(int k){dyna_extras_ui_input(k,1,0);}
static void render_test(int w,int h){
 size_t stride=(size_t)w+9,n=stride*h;uint32_t *p=malloc((n+2)*4);assert(p);for(size_t i=0;i<n+2;i++)p[i]=0x12345678;
 dyna_extras_ui_render(p+1,w,h,(int)stride*4);assert(p[0]==0x12345678&&p[n+1]==0x12345678);
 for(int y=0;y<h;y++)for(size_t x=(size_t)w;x<stride;x++)assert(p[1+y*stride+x]==0x12345678);
 free(p);
}
static void prompt_test(int w,int h){
 size_t stride=(size_t)w+9,n=stride*h;uint32_t *p=malloc((n+2)*4);assert(p);
 for(size_t i=0;i<n+2;i++)p[i]=0x12345678;
 dyna_extras_ui_progress_prompt(p+1,w,h,(int)stride*4,1,"PROGRESS SAVED");
 int left=(w-320)/2+80,top=(h-224)/2+176;unsigned changed=0;
 assert(p[0]==0x12345678 && p[n+1]==0x12345678);
 for(int y=0;y<h;y++)for(size_t x=0;x<stride;x++){
  uint32_t pixel=p[1+(size_t)y*stride+x];
  if((int)x<left || (int)x>=left+160 || y<top || y>=top+45)assert(pixel==0x12345678);
  else if(pixel!=0x12345678)changed++;
 }
 assert(changed>3000);
 for(size_t i=0;i<n+2;i++)p[i]=0x12345678;
 dyna_extras_ui_progress_prompt(p+1,319,h,(int)stride*4,0,NULL);
 dyna_extras_ui_progress_prompt(p+1,w,h,(int)stride*4-1,0,NULL);
 for(size_t i=0;i<n+2;i++)assert(p[i]==0x12345678);
 free(p);
}
static void prompt_fade_test(void){
 uint32_t p[320*224];
 for(unsigned i=0;i<320*224;i++)p[i]=0xff000000u;
 dyna_extras_ui_progress_prompt(p,320,224,320*4,0,"CONTINUING");
 for(unsigned i=0;i<320*224;i++)assert(p[i]==0xff000000u);
 for(int y=185;y<201;y++)for(int x=84;x<238;x++)p[y*320+x]=0xff7f7f7fu;
 dyna_extras_ui_progress_prompt(p,320,224,320*4,0,"CONTINUING");
 for(int y=176;y<221;y++)for(int x=80;x<240;x++){
  uint32_t value=p[y*320+x];
  assert(((value>>16)&255)<=127 && ((value>>8)&255)<=127 && (value&255)<=127);
 }
}
static void ending_prompt_test(void){
 const int w=427,h=240,stride=436;uint32_t p[436*240];
 for(unsigned i=0;i<436*240;i++)p[i]=0x12345678;
 dyna_extras_ui_ending_save_prompt(p,w,h,stride*4,"PROGRESS SAVED");
 int left=(w-320)/2+80,top=(h-224)/2+201;unsigned changed=0;
 for(int y=0;y<h;y++)for(int x=0;x<stride;x++){
  if(x<left||x>=left+160||y<top||y>=top+22)assert(p[y*stride+x]==0x12345678);
  else if(p[y*stride+x]!=0x12345678)changed++;
 }
 assert(changed>1000);
}
int main(void){
 ending_prompt_test();prompt_fade_test();prompt_test(320,224);prompt_test(427,240);prompt_test(768,480);

 DynaExtrasLevelProvider provider={NULL,count,item,launch};
 dyna_extras_ui_set_level_provider(NULL);dyna_extras_ui_init(1,NULL);assert(!dyna_extras_ui_is_open());dyna_extras_ui_open();
 key(6);assert(dyna_extras_ui_state().screen==1);key(6);assert(dyna_extras_ui_is_open());render_test(320,224);key(1);
 dyna_extras_ui_set_level_provider(&provider);key(6);key(6);assert(!launches);key(3);key(6);assert(launches==1&&!dyna_extras_ui_is_open());
 for(unsigned game=1;game<=2;game++)for(unsigned section=0;section<DYNA_GUIDE_SECTIONS;section++){
  dyna_extras_ui_init(game,"missing-guide-assets");dyna_extras_ui_open();key(3);key(6);assert(dyna_extras_ui_state().screen==2);
  for(unsigned i=0;i<section;i++){key(3);}
  key(6);assert(dyna_extras_ui_state().screen==3);
  for(unsigned page=0;page<dyna_guide_page_count(game,section);page++){
   render_test(320,224);render_test(768,480);render_test(160,144);render_test(641,359);
   for(int i=0;i<100;i++){key(3);}
   render_test(320,224);assert(dyna_extras_ui_state().scroll<64);
   key(6);assert(!dyna_extras_ui_state().image);key(5);assert(dyna_extras_ui_state().page<dyna_guide_page_count(game,section));
  }
  key(1);assert(dyna_extras_ui_state().screen==2);key(1);assert(dyna_extras_ui_state().screen==0);key(1);assert(!dyna_extras_ui_is_open());
 }
 dyna_extras_ui_open();dyna_extras_ui_input(0,1,1);assert(dyna_extras_ui_is_open());dyna_extras_ui_input(0,0,0);assert(dyna_extras_ui_is_open());key(0);assert(!dyna_extras_ui_is_open());
 dyna_extras_ui_reset();return 0;
}
