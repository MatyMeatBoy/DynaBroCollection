#include "controller_profiles.h"
#include "gamepad.h"
#include "game_spec.h"
#include "input_map.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>

const GameSpec g_game_spec={
    .controller_prepare=dyna_controller_prepare,
    .controller_mask=dyna_controller_mask,
    .controller_removed=dyna_controller_removed
};
static void drain(void){SDL_Event event;SDL_PumpEvents();while(SDL_PollEvent(&event))gamepad_handle_event(&event);}
static SDL_Joystick *attach(const char *name,unsigned vendor,unsigned product,int *index) {
    SDL_VirtualJoystickDesc desc={0};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type=SDL_JOYSTICK_TYPE_UNKNOWN;desc.naxes=6;desc.nbuttons=12;desc.nhats=1;
    desc.vendor_id=(Uint16)vendor;desc.product_id=(Uint16)product;desc.name=name;
    *index=SDL_JoystickAttachVirtualEx(&desc);assert(*index>=0);
    SDL_Joystick *stick=SDL_JoystickOpen(*index);assert(stick);return stick;
}
int main(int argc,char **argv) {
    assert(argc==2);SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    assert(!SDL_Init(SDL_INIT_GAMECONTROLLER));
    char profile_file[1024];snprintf(profile_file,sizeof profile_file,"%s/dynabrothers-controllers.ini",argv[1]);remove(profile_file);
    char settings[1024];snprintf(settings,sizeof settings,"%s/settings.ini",argv[1]);
    dyna_controllers_load(settings);
    input_map_init_defaults();g_input_map.p[0].pad_type=PAD_6BUTTON;g_input_map.p[1].pad_type=PAD_6BUTTON;
    gamepad_init();
    int index;SDL_Joystick *stick=attach("8BitDo M30 gamepad",0x2dc8,0x5006,&index);drain();
    assert(gamepad_player_connected(0));
    const int raw[]={0,1,7,3,4,6,11,10};
    const GenesisButton expected[]={GB_A,GB_B,GB_C,GB_X,GB_Y,GB_Z,GB_START,GB_MODE};
    for(unsigned n=0;n<8;++n) {
        assert(!SDL_JoystickSetVirtualButton(stick,raw[n],1));SDL_JoystickUpdate();drain();
        assert(gamepad_player_mask(0)==input_button_bit(expected[n]));
        assert(!gamepad_player_mask(1));
        SDL_JoystickSetVirtualButton(stick,raw[n],0);SDL_JoystickUpdate();drain();
    }
    g_input_map.p[0].pad_type=PAD_3BUTTON;
    SDL_JoystickSetVirtualButton(stick,3,1);SDL_JoystickUpdate();drain();assert(!gamepad_player_mask(0));
    SDL_JoystickSetVirtualButton(stick,3,0);g_input_map.p[0].pad_type=PAD_6BUTTON;
    int generic_index;SDL_Joystick *generic=attach("Unmapped test DirectInput pad",0x1209,0x7001,&generic_index);drain();
    assert(gamepad_player_connected(1));
    SDL_JoystickSetVirtualButton(generic,0,1);SDL_JoystickUpdate();drain();
    assert(gamepad_player_mask(1)==input_button_bit(GB_B));assert(!gamepad_player_mask(0));
    SDL_JoystickDetachVirtual(index);drain();assert(!gamepad_player_connected(0));assert(gamepad_player_connected(1));
    /* Attach another six-face layout into P1 without changing P2's mapping. */
    int mini_index;SDL_Joystick *mini=attach("8BitDo M30 Mini 2.4G",0x0ca3,0x24,&mini_index);drain();
    assert(gamepad_player_connected(0));
    SDL_JoystickSetVirtualButton(mini,2,1);SDL_JoystickUpdate();drain();
    assert(gamepad_player_mask(0)==input_button_bit(GB_A));
    assert(gamepad_player_mask(1)==input_button_bit(GB_B));
    assert(dyna_controller_set("controls.p1_profile",DYNA_PAD_M30_MINI));
    assert(!dyna_controller_set("controls.p1_profile",99));
    SDL_JoystickSetVirtualButton(generic,0,0);
    SDL_JoystickSetVirtualAxis(generic,5,-32768);SDL_JoystickUpdate();drain();
    assert(dyna_controller_set("controls.p2_profile",DYNA_PAD_M30_XINPUT));
    SDL_JoystickSetVirtualButton(generic,1,1);SDL_JoystickUpdate();drain();
    assert(gamepad_player_mask(1)==input_button_bit(GB_A));
    SDL_JoystickSetVirtualButton(generic,1,0);SDL_JoystickSetVirtualAxis(generic,5,32767);SDL_JoystickUpdate();drain();
    assert(gamepad_player_mask(1)==input_button_bit(GB_C));
    assert(dyna_controller_set("controls.p2_profile",DYNA_PAD_CUSTOM));
    dyna_controllers_load(settings);
    (void)gamepad_player_mask(0);(void)gamepad_player_mask(1);
    int choice=-1;assert(dyna_controller_get("controls.p1_profile",&choice)&&choice==DYNA_PAD_M30_MINI);
    assert(dyna_controller_get("controls.p2_profile",&choice)&&choice==DYNA_PAD_CUSTOM);
    gamepad_shutdown();SDL_JoystickClose(stick);SDL_JoystickClose(generic);SDL_JoystickClose(mini);SDL_Quit();
    remove(profile_file);
    return 0;
}
