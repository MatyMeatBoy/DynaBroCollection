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
int main(void){
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
