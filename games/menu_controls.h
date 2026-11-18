#pragma once
#include <stdint.h>

/* Actions match RecompRuntimeUiInput; -1 means no action this tick. */
enum { DYNA_MENU_NONE=-1,DYNA_MENU_TOGGLE=0,DYNA_MENU_BACK=1,
    DYNA_MENU_UP=2,DYNA_MENU_DOWN=3,DYNA_MENU_LEFT=4,DYNA_MENU_RIGHT=5,DYNA_MENU_ACCEPT=6 };
typedef struct DynaMenuControls {
    uint32_t previous;
    uint64_t repeat_at;
    int direction,chord_latched,release_guard,was_open;
} DynaMenuControls;
/* Call after SDL event polling and before guest input, even when menus are closed.
 * mask uses GP_OVERLAY_* physical buttons plus deadzone-filtered directions. */
int dyna_menu_controls_tick(DynaMenuControls *state,uint32_t mask,int menu_open,uint64_t now_ms);
/* Keep guest input captured while a menu is visible or closing buttons remain held. */
int dyna_menu_controls_blocks(const DynaMenuControls *state,int menu_open);
