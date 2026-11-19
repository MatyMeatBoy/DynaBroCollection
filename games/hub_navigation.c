#include "hub_navigation.h"
#include "gamepad.h"
int dyna_hub_navigation_tick(DynaHubNavigation *s,uint32_t mask,uint64_t now){
 if(!s)return 0;if(s->focus>5)s->focus=0;
 int start=(mask&GP_OVERLAY_START)&&!(mask&GP_OVERLAY_BACK)&&!(s->previous&GP_OVERLAY_START);
 s->previous=mask;int action=dyna_menu_controls_tick(&s->controls,mask,1,now);
 unsigned row=s->focus/2,col=s->focus%2;
 if(action==DYNA_MENU_LEFT)col=0;
 else if(action==DYNA_MENU_RIGHT)col=1;
 else if(action==DYNA_MENU_UP){if(row>0)row--;}
 else if(action==DYNA_MENU_DOWN){if(row<2)row++;}
 s->focus=row*2+col;
 if(action==DYNA_MENU_ACCEPT||start){
  static const int base[3]={101,201,301};
  return base[row]+(int)col;
 }
 return 0;
}
