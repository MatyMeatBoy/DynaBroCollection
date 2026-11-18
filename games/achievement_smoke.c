/* Explicit diagnostic harness for our own SDL event/modal integration.
 * It cannot award goals; the session is disqualified before guest execution. */
#include "achievement_smoke.h"
#include "achievement_session.h"
#include "achievement_ui.h"
#include "genesis_runtime.h"
#include "video/genesis_machine.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *capture;
static int started,draws,finished;
static uint8_t ram[65536];
static M68KState cpu;
static GVDP vdp;
void dyna_achievement_smoke_init(void) {
    capture=getenv("DYNA_ACHIEVEMENT_UI_TEST");started=draws=finished=0;
    if(capture&&*capture)dyna_achievement_session_diagnostic();else capture=NULL;
}
void dyna_achievement_smoke_frame(uint64_t frame) {
    if(!capture)return;
    if(!started&&frame>=30){started=1;dyna_achievement_ui_open();}
    if(finished==1&&!dyna_achievement_ui_is_open()) {
        fprintf(stderr,"[achievement-ui-test] PASS real SDL Back event closed modal\n");finished=2;
    }
}
static void key(SDL_Keycode symbol) {
    SDL_Event event;memset(&event,0,sizeof event);
    event.type=SDL_KEYDOWN;event.key.state=SDL_PRESSED;event.key.keysym.sym=symbol;
    if(SDL_PushEvent(&event)!=1)abort();
    event.type=SDL_KEYUP;event.key.state=SDL_RELEASED;
    if(SDL_PushEvent(&event)!=1)abort();
}
void dyna_achievement_smoke_render(uint32_t *pixels,int width,int height,int pitch) {
    dyna_achievement_ui_render(pixels,width,height,pitch);
    if(!capture || !started || finished || !dyna_achievement_ui_is_open())return;
    if(!draws){memcpy(ram,g_ram,sizeof ram);cpu=g_cpu;vdp=g_machine.vdp;}
    if(memcmp(ram,g_ram,sizeof ram)||memcmp(&cpu,&g_cpu,sizeof cpu)||memcmp(&vdp,&g_machine.vdp,sizeof vdp)) {
        fprintf(stderr,"[achievement-ui-test] FAIL guest advanced inside modal\n");abort();
    }
    ++draws;
    if(draws==2)key(SDLK_DOWN);
    if(draws==3)key(SDLK_RIGHT);
    if(draws==5) {
        SDL_Surface *surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels,width,height,32,pitch,SDL_PIXELFORMAT_ARGB8888);
        if(!surface || SDL_SaveBMP(surface,capture)!=0)abort();
        SDL_FreeSurface(surface);
    }
    if(draws==6) {
        fprintf(stderr,"[achievement-ui-test] PASS 6 modal presentations; CPU/RAM/VDP unchanged\n");
        key(SDLK_ESCAPE);finished=1;
    }
}
