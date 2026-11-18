#include "menu_controls.h"
#include "gamepad.h"
#include <assert.h>

int main(void) {
    DynaMenuControls s={0};
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_START,0,0)==DYNA_MENU_NONE);
    assert(!dyna_menu_controls_blocks(&s,0));
    assert(dyna_menu_controls_tick(&s,0,0,1)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_BACK,0,2)==DYNA_MENU_NONE);
    assert(!dyna_menu_controls_blocks(&s,0));
    uint32_t chord=GP_OVERLAY_START|GP_OVERLAY_BACK;
    assert(dyna_menu_controls_tick(&s,chord,0,3)==DYNA_MENU_TOGGLE);
    assert(dyna_menu_controls_blocks(&s,0));
    assert(dyna_menu_controls_tick(&s,chord,1,4)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_START,1,5)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,chord,1,6)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,0,1,7)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_UP,1,10)==DYNA_MENU_UP);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_UP,1,309)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_UP,1,310)==DYNA_MENU_UP);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_UP,1,390)==DYNA_MENU_UP);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_UP|GP_OVERLAY_DOWN,1,400)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_LEFT|GP_OVERLAY_A,1,410)==DYNA_MENU_ACCEPT);
    /* Route may close the menu immediately; suppress that same guest tick. */
    assert(dyna_menu_controls_blocks(&s,0));
    assert(dyna_menu_controls_tick(&s,GP_OVERLAY_A,0,411)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_blocks(&s,0));
    assert(dyna_menu_controls_tick(&s,0,0,412)==DYNA_MENU_NONE);
    assert(!dyna_menu_controls_blocks(&s,0));
    assert(dyna_menu_controls_tick(&s,chord,0,413)==DYNA_MENU_TOGGLE);
    assert(dyna_menu_controls_tick(&s,0,1,414)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_tick(&s,chord,1,415)==DYNA_MENU_TOGGLE);
    assert(dyna_menu_controls_tick(&s,chord,0,416)==DYNA_MENU_NONE);
    assert(dyna_menu_controls_blocks(&s,0));
    assert(dyna_menu_controls_tick(&s,0,0,417)==DYNA_MENU_NONE);
    assert(!dyna_menu_controls_blocks(&s,0));
    return 0;
}
