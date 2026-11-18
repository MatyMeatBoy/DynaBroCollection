#include "db1_title_menu.h"
#include <string.h>
static void clear(Db1TitleMenu *s){
 unsigned enabled=s->enabled;memset(s,0,sizeof *s);s->enabled=enabled;
}
void db1_title_menu_reset(Db1TitleMenu *s){if(s)memset(s,0,sizeof *s);}
void db1_title_menu_enable(Db1TitleMenu *s,int enabled){
 if(!s)return;
 unsigned value=enabled!=0;
 if(s->enabled!=value){clear(s);s->enabled=value;}
}
void db1_title_menu_before(Db1TitleMenu *s,uint32_t pc,unsigned native){
 if(!s)return;
 switch(pc){
 case 0x1950:case 0x1a14:case 0x1e024:case 0x1d7b6:case 0x1c5a4:
 case 0x1c9ce:case 0x1caf6:case 0x1cb0c:case 0x1cbc4:case 0x1cbea:
 case 0x1cc16:case 0x1cc5c:case 0x1cc7c:case 0x1cc92:clear(s);return;
 case 0x1cb30:case 0x1cb34:case 0x1cb46:case 0x1cb4a:
 case 0x1cb4e:case 0x1cb54:break;
 default:return;
 }
 if(!s->enabled)return;
 if(pc==0x1cb30)s->entered=1;
 if(!s->entered)return;
 if(native>3){clear(s);return;}
 if(s->pending){
  if(native==s->target)s->ack=1;
  else if(native!=s->anchor){clear(s);return;}
 }else if(s->extras&&native!=s->anchor){clear(s);return;}
 s->native_selection=native;
 if(pc!=0x1cb30&&pc!=0x1cb34&&!s->active){
  s->active=1;s->released=0;s->previous=0;
 }
}
int db1_title_menu_frame(Db1TitleMenu *s,unsigned input,int *override,unsigned *pad){
 if(override)*override=0;
 if(pad)*pad=input;
 if(!s||!s->enabled||!s->active)return 0;
 unsigned edges=input&~s->previous;s->previous=input;
 if(s->modal){if(override)*override=1;if(pad)*pad=0;return 0;}
 if(s->pending){
  if(override)*override=1;
  if(pad)*pad=s->pulse<3?s->button:0;
  if(s->pulse<6)++s->pulse;
  else if(s->ack){
   /* Once native navigation acknowledged, physical release is not a timeout:
    * restoring a still-held key would create an unwanted second native step. */
   if(!input){s->pending=0;s->released=1;s->previous=0;}
  }else if(++s->wait>120)clear(s);
  return 0;
 }
 if(!s->released){
  if(override)*override=1;
  if(pad)*pad=0;
  if(!input)s->released=1;
  return 0;
 }
 unsigned direction=edges&(DB1_TITLE_UP|DB1_TITLE_DOWN);
 if(s->extras){
  if(override)*override=1;
  if(pad)*pad=0;
  if(direction==DB1_TITLE_UP||direction==DB1_TITLE_DOWN){
   unsigned target=direction==DB1_TITLE_UP?3:0;
   s->extras=0;s->released=0;
   if(s->native_selection!=target){
    s->pending=1;s->anchor=s->native_selection;s->target=target;
    s->button=direction;s->pulse=1;s->ack=0;s->wait=0;
    if(pad)*pad=direction;
   }
   return 0;
  }
  if(edges&(DB1_TITLE_CONFIRM|DB1_TITLE_START)){s->modal=1;return 1;}
  return 0;
 }
 /* Preserve native confirmation precedence for the original four rows. */
 if(input&~(DB1_TITLE_UP|DB1_TITLE_DOWN))return 0;
 if((direction==DB1_TITLE_DOWN&&s->native_selection==3)||
    (direction==DB1_TITLE_UP&&s->native_selection==0)){
  s->extras=1;s->anchor=s->native_selection;s->released=0;
  if(override)*override=1;
  if(pad)*pad=0;
 }
 return 0;
}
int db1_title_menu_active(const Db1TitleMenu *s){return s&&s->enabled&&s->active;}
int db1_title_menu_selection(const Db1TitleMenu *s,unsigned *selected){
 if(!db1_title_menu_active(s))return 0;
 if(selected)*selected=s->extras?4:s->native_selection;
 return 1;
}
void db1_title_menu_host_closed(Db1TitleMenu *s){
 if(s&&s->active){s->modal=0;s->released=0;s->previous=0;}
}
