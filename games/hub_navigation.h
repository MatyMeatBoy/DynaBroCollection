#pragma once
#include "menu_controls.h"
typedef struct {DynaMenuControls controls;unsigned focus;uint32_t previous;} DynaHubNavigation;
/* Focus0/1=gamecards,2/3=settings; returns Win32 command ID or0. */
int dyna_hub_navigation_tick(DynaHubNavigation *,uint32_t mask,uint64_t now);
