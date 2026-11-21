#include "progress_ui.h"
#include <string.h>
void dyna_progress_ui_reset(DynaProgressUi *s){
 if(s){memset(s,0,sizeof *s);s->feedback="";}
}
void dyna_progress_ui_before(DynaProgressUi *s,uint32_t pc,int qualified){
 if(!s)return;
 switch(pc){
 case 0x1950:case 0x1a14:case 0x1c5a4:case 0x1c9ce:case 0x1caf6:case 0x1cb0c:
 case 0x1d7b6:case 0x28f2e:dyna_progress_ui_reset(s);return;
 case 0x1bd7c:
  s->exiting=s->exiting || s->active;
  s->active=0;s->candidate=0;return;
 default:break;
 }
 if(pc==0x1becc){
  dyna_progress_ui_reset(s);s->candidate=qualified!=0;s->kind=1;
 }
 if(pc==0x28f1a){
  dyna_progress_ui_reset(s);
  if(qualified){s->kind=2;s->active=1;s->selected=1;}
 }
 if(pc==0x1bd6c && s->candidate){
  if(!qualified){dyna_progress_ui_reset(s);return;}
  if(!s->active){s->active=1;s->selected=0;s->released=0;s->previous=0;}
 }
}
int dyna_progress_ui_frame(DynaProgressUi *s,unsigned buttons,int *override,int *start){
 if(override)*override=0;
 if(start)*start=0;
 if(!s)return 0;
 /* Native result drawing spans guest frames before its first input poll.
  * Release that already-collected pad before the stable loop can consume it. */
 if(s->candidate && !s->active){if(override)*override=1;return 0;}
 if(!s->active)return 0;
 if(override)*override=1;
 if(s->continuing){
  if(start)*start=s->pulse<3;
  if(s->pulse<6)++s->pulse;
  return 0;
 }
 unsigned edges=buttons&~s->previous;s->previous=buttons;
 if(!s->released){if(!buttons)s->released=1;return 0;}
 /* Timed native ending never polls START and has no manual Continue. */
 if(s->kind==2)return (edges&DYNA_PROGRESS_UI_CONFIRM)!=0;
 if((edges&(DYNA_PROGRESS_UI_UP|DYNA_PROGRESS_UI_DOWN))==DYNA_PROGRESS_UI_UP)s->selected=0;
 if((edges&(DYNA_PROGRESS_UI_UP|DYNA_PROGRESS_UI_DOWN))==DYNA_PROGRESS_UI_DOWN)s->selected=1;
 if(!(edges&(DYNA_PROGRESS_UI_CONFIRM|DYNA_PROGRESS_UI_START)))return 0;
 if(s->selected)return 1;
 s->continuing=1;s->pulse=1;
 if(start)*start=1;
 return 0;
}
void dyna_progress_ui_saved(DynaProgressUi *s,int ok){
 if(s && s->active && !s->continuing)s->feedback=ok?"GAME SAVED":"SAVE FAILED";
}
int dyna_progress_ui_prompt(const DynaProgressUi *s,unsigned *selected,const char **feedback){
 if(selected)*selected=s?s->selected:0;
 if(feedback)*feedback=s && (s->continuing || s->exiting)?"CONTINUING":
  s && s->feedback?s->feedback:"";
 return s && (s->active || s->exiting);
}
int dyna_progress_ui_kind(const DynaProgressUi *s){
 return dyna_progress_ui_prompt(s,NULL,NULL)?(int)s->kind:0;
}
