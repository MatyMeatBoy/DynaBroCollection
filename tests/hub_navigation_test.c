#include "hub_navigation.h"
#include "gamepad.h"
#include <assert.h>
int main(void){DynaHubNavigation s={0};
 assert(!dyna_hub_navigation_tick(&s,0,0));
 assert(!dyna_hub_navigation_tick(&s,GP_OVERLAY_RIGHT,1)&&s.focus==1);
 assert(!dyna_hub_navigation_tick(&s,0,2));
 assert(!dyna_hub_navigation_tick(&s,GP_OVERLAY_DOWN,3)&&s.focus==3);
 assert(!dyna_hub_navigation_tick(&s,0,4));
 assert(dyna_hub_navigation_tick(&s,GP_OVERLAY_A,5)==202);
 assert(!dyna_hub_navigation_tick(&s,GP_OVERLAY_A,6));
 dyna_hub_navigation_tick(&s,0,7);
 assert(dyna_hub_navigation_tick(&s,GP_OVERLAY_START,8)==202);
 assert(!dyna_hub_navigation_tick(&s,GP_OVERLAY_START,9));
 dyna_hub_navigation_tick(&s,0,10);
 assert(!dyna_hub_navigation_tick(&s,GP_OVERLAY_START|GP_OVERLAY_BACK,11));
 s.focus=0;dyna_hub_navigation_tick(&s,0,12);
 assert(dyna_hub_navigation_tick(&s,GP_OVERLAY_A,13)==101);
 s.focus=2;dyna_hub_navigation_tick(&s,0,14);
 assert(dyna_hub_navigation_tick(&s,GP_OVERLAY_A,15)==201);
 return 0;}
