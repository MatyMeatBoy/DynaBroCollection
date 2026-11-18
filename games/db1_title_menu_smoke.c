/* Explicit natural-title/SDL modal diagnostic. No guest or UI opening writes.
 * Queued SDL events qualify integration, not physical controller hardware. */
#include "db1_title_menu_smoke.h"
#include "achievement_session.h"
#include "extras_ui.h"
#include "genesis_runtime.h"
#include "video/genesis_machine.h"
#include <SDL.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char *capture;
static int started,draws,finished,level;
static unsigned target_index=1;
static uint8_t ram[65536];
static M68KState cpu;
static GVDP vdp;
static void fail(const char *reason){
 fprintf(stderr,"[title-ui-test] FAIL %s\n",reason);abort();
}
void db1_title_menu_smoke_init(void){
 capture=getenv("DYNA_DB1_TITLE_UI_TEST");started=draws=finished=0;
 const char *mode=getenv("DYNA_DB1_TITLE_UI_MODE");level=mode&&!strcmp(mode,"level");
 target_index=1;
 const char *index=getenv("DYNA_DB1_TITLE_UI_INDEX");
 if(capture&&*capture&&index&&*index){
  unsigned value=0;
  for(const char *p=index;*p;p++){
   if(*p<'0'||*p>'9')fail("invalid level test index");
   value=value*10u+(unsigned)(*p-'0');if(value>96)fail("level test index out of range");
  }
  target_index=value;
 }
 if(capture&&*capture)dyna_achievement_session_diagnostic();
 else capture=NULL;
}
void db1_title_menu_smoke_opened(void){
 /* Root calls only after db1_title_menu_frame's real fifth-row open edge. */
 if(capture&&!started)started=1;
}
void db1_title_menu_smoke_frame(void){
 if(capture&&finished==1&&!dyna_extras_ui_is_open()){
  fprintf(stderr,"[title-ui-test] PASS real SDL %s closed modal\n",level?"level confirm":"Back event");
  finished=2;
 }
}
static void key_event(SDL_Keycode key){
 SDL_Event event;memset(&event,0,sizeof event);
 event.type=SDL_KEYDOWN;event.key.state=SDL_PRESSED;event.key.keysym.sym=key;
 if(SDL_PushEvent(&event)!=1)fail("unable to queue SDL test keydown");
 event.type=SDL_KEYUP;event.key.state=SDL_RELEASED;
 if(SDL_PushEvent(&event)!=1)fail("unable to queue SDL test keyup");
}
void db1_title_menu_smoke_render(uint32_t *pixels,int width,int height,int pitch){
 if(!capture||!started||finished||!dyna_extras_ui_is_open())return;
 if(!pixels||width<=0||height<=0||width>8192||height>8192||pitch<=0||
    pitch%4||pitch/4<width||pitch>INT_MAX/height)fail("invalid capture geometry");
 if(!draws){
  memcpy(ram,g_ram,sizeof ram);memcpy(&cpu,&g_cpu,sizeof cpu);
  memcpy(&vdp,&g_machine.vdp,sizeof vdp);
 }
 if(memcmp(ram,g_ram,sizeof ram)||memcmp(&cpu,&g_cpu,sizeof cpu)||
    memcmp(&vdp,&g_machine.vdp,sizeof vdp))fail("guest advanced inside title Extras modal");
 ++draws;
 if(level){
  DynaExtrasUiState state=dyna_extras_ui_state();
  if(draws==2){
   if(state.screen||state.selected)fail("level test expected Extras home");
   key_event(SDLK_RETURN);
  }
  if(draws==3){
   if(state.screen!=1||state.selected)fail("level test expected first starting level");
   for(unsigned i=0;i<target_index;i++)key_event(SDLK_DOWN);
  }
  if(draws==4&&(state.screen!=1||state.selected!=target_index))fail("level test expected requested selection");
 }
 if(draws==(level?4:5)){
  SDL_Surface *surface=SDL_CreateRGBSurfaceWithFormatFrom(pixels,width,height,32,pitch,SDL_PIXELFORMAT_ARGB8888);
  if(!surface)fail("unable to create BMP surface");
  int saved=SDL_SaveBMP(surface,capture)==0;SDL_FreeSurface(surface);
  if(!saved)fail("unable to save BMP capture");
 }
 if(level&&draws==4){
  fprintf(stderr,"[title-ui-test] PASS 4 level modal presentations; CPU/RAM/VDP unchanged\n");
  key_event(SDLK_RETURN);finished=1;
 }else if(!level&&draws==6){
  fprintf(stderr,"[title-ui-test] PASS 6 modal presentations; CPU/RAM/VDP unchanged\n");
  key_event(SDLK_ESCAPE);finished=1;
 }
}
