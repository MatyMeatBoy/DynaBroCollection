#include "menu_controls.h"
#include "gamepad.h"

int dyna_menu_controls_tick(DynaMenuControls *s,uint32_t mask,int open,uint64_t now)
{
    if(!s)return DYNA_MENU_NONE;
    const uint32_t chord=GP_OVERLAY_START|GP_OVERLAY_BACK;
    uint32_t pressed=mask&~s->previous;
    int action=DYNA_MENU_NONE;
    if(s->was_open&&!open)s->release_guard=1;
    s->was_open=!!open;
    if(!(mask&chord))s->chord_latched=0;
    if(!mask)s->release_guard=0;
    if((mask&chord)==chord&&!s->chord_latched) {
        s->chord_latched=1;s->release_guard=1;s->direction=0;
        action=DYNA_MENU_TOGGLE;
    } else if(open&&!s->release_guard) {
        if(pressed&GP_OVERLAY_B){action=DYNA_MENU_BACK;s->release_guard=1;s->direction=0;}
        else if(pressed&(GP_OVERLAY_A|GP_OVERLAY_START)){action=DYNA_MENU_ACCEPT;s->release_guard=1;s->direction=0;}
        else {
            uint32_t dirs=mask&(GP_OVERLAY_UP|GP_OVERLAY_DOWN|GP_OVERLAY_LEFT|GP_OVERLAY_RIGHT);
            int direction=0;
            /* Opposing directions cancel; diagonals select vertical once. */
            if((dirs&(GP_OVERLAY_UP|GP_OVERLAY_DOWN))==GP_OVERLAY_UP)direction=DYNA_MENU_UP;
            else if((dirs&(GP_OVERLAY_UP|GP_OVERLAY_DOWN))==GP_OVERLAY_DOWN)direction=DYNA_MENU_DOWN;
            else if((dirs&(GP_OVERLAY_LEFT|GP_OVERLAY_RIGHT))==GP_OVERLAY_LEFT)direction=DYNA_MENU_LEFT;
            else if((dirs&(GP_OVERLAY_LEFT|GP_OVERLAY_RIGHT))==GP_OVERLAY_RIGHT)direction=DYNA_MENU_RIGHT;
            if(direction&&direction!=s->direction){action=direction;s->repeat_at=now+300;}
            else if(direction&&now>=s->repeat_at){action=direction;s->repeat_at=now+80;}
            s->direction=direction;
        }
    } else s->direction=0;
    s->previous=mask;
    return action;
}
int dyna_menu_controls_blocks(const DynaMenuControls *s,int open)
{ return !!open||(s&&s->release_guard); }
