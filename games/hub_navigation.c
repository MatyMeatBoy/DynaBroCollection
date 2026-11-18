#include "hub_navigation.h"
#include "gamepad.h"
int dyna_hub_navigation_tick(DynaHubNavigation *s,uint32_t mask,uint64_t now){
 if(!s)return 0;if(s->focus>3)s->focus=0;
 int start=(mask&GP_OVERLAY_START)&&!(mask&GP_OVERLAY_BACK)&&!(s->previous&GP_OVERLAY_START);
 s->previous=mask;int action=dyna_menu_controls_tick(&s->controls,mask,1,now);
 if(action==DYNA_MENU_LEFT)s->focus&=2;
 else if(action==DYNA_MENU_RIGHT)s->focus|=1;
 else if(action==DYNA_MENU_UP)s->focus&=1;
 else if(action==DYNA_MENU_DOWN)s->focus|=2;
 if(action==DYNA_MENU_ACCEPT||start)return(s->focus<2?101:199)+(int)s->focus;
 return 0;
}
