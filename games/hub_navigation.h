#pragma once
#include "menu_controls.h"
typedef struct {DynaMenuControls controls;unsigned focus;uint32_t previous;} DynaHubNavigation;
/* Focus is row*2+col over 3 rows of 2 columns: 0/1=gamecards,2/3=settings,
 * 4/5=manual buttons. Returns a Win32 command ID (101+col, 201+col, or
 * 301+col) or0. */
int dyna_hub_navigation_tick(DynaHubNavigation *,uint32_t mask,uint64_t now);
