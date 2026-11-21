#include "progress_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 DynaProgressUi s;unsigned selected=99;const char *feedback=NULL;int over,start;
 dyna_progress_ui_reset(&s);
 dyna_progress_ui_before(&s,0x1becc,1);
 assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start)&&over&&!start);
 dyna_progress_ui_before(&s,0x1bd6c,1);
 assert(dyna_progress_ui_prompt(&s,&selected,&feedback)&&selected==0&&!strcmp(feedback,""));
 assert(dyna_progress_ui_kind(&s)==1);
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start)&&over&&!start);
 assert(!dyna_progress_ui_frame(&s,0,&over,&start));
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_DOWN,&over,&start));
 dyna_progress_ui_before(&s,0x1bd6c,1);
 assert(dyna_progress_ui_prompt(&s,&selected,NULL)&&selected==1);
 assert(dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_CONFIRM,&over,&start));
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_CONFIRM,&over,&start));
 dyna_progress_ui_saved(&s,0);
 assert(dyna_progress_ui_prompt(&s,NULL,&feedback)&&!strcmp(feedback,"SAVE FAILED"));
 dyna_progress_ui_frame(&s,0,&over,&start);
 assert(dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_CONFIRM,&over,&start));
 dyna_progress_ui_saved(&s,1);
 assert(dyna_progress_ui_prompt(&s,NULL,&feedback)&&!strcmp(feedback,"GAME SAVED"));
 dyna_progress_ui_frame(&s,0,&over,&start);
 dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_UP,&over,&start);
 dyna_progress_ui_frame(&s,0,&over,&start);
 for(unsigned i=0;i<10;i++){
  assert(!dyna_progress_ui_frame(&s,i==0?DYNA_PROGRESS_UI_CONFIRM:0,&over,&start));
  assert(over&&start==(i<3));
 }
 assert(dyna_progress_ui_prompt(&s,NULL,&feedback)&&!strcmp(feedback,"CONTINUING"));
 dyna_progress_ui_before(&s,0x1bd7c,1);
 dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start);assert(!over&&!start);
 assert(dyna_progress_ui_prompt(&s,NULL,&feedback)&&!strcmp(feedback,"CONTINUING"));
 dyna_progress_ui_before(&s,0x1bd6c,1);
 dyna_progress_ui_frame(&s,0,&over,&start);assert(!over);
 dyna_progress_ui_before(&s,0x1950,1);
 assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x1dd90,0);dyna_progress_ui_before(&s,0x1bd6c,0);
 assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x1becc,0);dyna_progress_ui_before(&s,0x1bd6c,1);
 assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x1becc,1);dyna_progress_ui_before(&s,0x1950,1);
 dyna_progress_ui_before(&s,0x1bd6c,1);assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x1becc,1);dyna_progress_ui_before(&s,0x1bd6c,1);
 dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start);assert(over&&!start);
 dyna_progress_ui_frame(&s,0,&over,&start);
 dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start);assert(start);
 dyna_progress_ui_before(&s,0x1c9ce,1);assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x1becc,1);dyna_progress_ui_before(&s,0x1bd6c,1);
 dyna_progress_ui_frame(&s,0,&over,&start);dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start);
 dyna_progress_ui_before(&s,0x1bd7c,1);
 dyna_progress_ui_before(&s,0x1becc,1);
 assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x1bd6c,1);
 assert(dyna_progress_ui_prompt(&s,&selected,&feedback)&&selected==0&&!strcmp(feedback,""));
 dyna_progress_ui_frame(&s,0,&over,&start);dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start);
 dyna_progress_ui_before(&s,0x1bd7c,1);
 assert(dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x1a14,1);
 assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_reset(&s);assert(dyna_progress_ui_kind(&s)==0);
 dyna_progress_ui_before(&s,0x28f1a,0);assert(!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_before(&s,0x28f1a,1);
 assert(dyna_progress_ui_kind(&s)==2);
 assert(dyna_progress_ui_prompt(&s,&selected,&feedback)&&selected==1&&!strcmp(feedback,""));
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start)&&over&&!start);
 dyna_progress_ui_frame(&s,0,&over,&start);
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_START,&over,&start)&&over&&!start);
 dyna_progress_ui_frame(&s,0,&over,&start);
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_DOWN,&over,&start)&&!start);
 assert(dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_CONFIRM,&over,&start)&&!start);
 assert(!dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_CONFIRM,&over,&start)&&!start);
 dyna_progress_ui_saved(&s,0);
 assert(dyna_progress_ui_prompt(&s,NULL,&feedback)&&!strcmp(feedback,"SAVE FAILED"));
 dyna_progress_ui_frame(&s,0,&over,&start);
 assert(dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_CONFIRM,&over,&start)&&!start);
 dyna_progress_ui_saved(&s,1);
 assert(dyna_progress_ui_prompt(&s,NULL,&feedback)&&!strcmp(feedback,"GAME SAVED"));
 dyna_progress_ui_before(&s,0x28f2e,1);
 assert(dyna_progress_ui_kind(&s)==0&&!dyna_progress_ui_prompt(&s,NULL,NULL));
 dyna_progress_ui_frame(&s,DYNA_PROGRESS_UI_CONFIRM,&over,&start);assert(!over&&!start);
 puts("Progress prompt lifecycle, release guard, save retry and bounded Continue passed");
}
