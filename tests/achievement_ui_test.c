#include "achievement_ui.h"
#include "achievements.h"
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static DynaAchievementItem items[12];
static DynaAchievementUnlock event;
static int pending;
size_t dyna_achievements_count(void){return 12;}
const DynaAchievementItem *dyna_achievements_item(size_t i){return i<12?&items[i]:NULL;}
DynaAchievementUnlock dyna_achievements_last_unlock(void){return event;}
int dyna_achievements_pop_unlock(DynaAchievementUnlock *out){if(!pending)return 0;*out=event;pending=0;return 1;}
static uint32_t checksum(const uint32_t *p,int stride,int w,int h){
    uint32_t hash=2166136261u;for(int y=0;y<h;++y)for(int x=0;x<w;++x)hash=(hash^p[y*stride+x])*16777619u;return hash;
}
static void clipped_render(int width,int height){
    const uint32_t sentinel=0xffaba9a7u;int stride=width+8;
    size_t count=(size_t)stride*height+32;uint32_t *storage=malloc(count*sizeof *storage);assert(storage);
    for(size_t i=0;i<count;++i)storage[i]=sentinel;
    uint32_t *canvas=storage+16;
    dyna_achievement_ui_render(canvas,width,height,stride*4);
    for(int i=0;i<16;++i){assert(storage[i]==sentinel);assert(storage[count-1-i]==sentinel);}
    for(int y=0;y<height;++y)for(int x=width;x<stride;++x)assert(canvas[y*stride+x]==sentinel);
    uint32_t before=checksum(canvas,stride,width,height);
    dyna_achievement_ui_render(canvas,width,height,width*4-1); /* reject malformed pitch */
    assert(checksum(canvas,stride,width,height)==before);
    free(storage);
}
int main(int argc,char **argv){
    assert(argc==2||argc==3);SDL_SetMainReady();assert(SDL_Init(SDL_INIT_TIMER)==0);
    for(size_t i=0;i<12;++i){
        items[i]=(DynaAchievementItem){.id=(uint32_t)i+1,.title="I Bless the Rains Down in Pangea",
            .description="Use Rain 100+ times in one battle, and win. Normal/Hard/Back mode only.",
            .points=5,.badge_file_locked="db1/214459_lock.png",.badge_file_unlocked="db1/214459.png",.qualified=1};
    }
    items[1].title="Alien Predator";items[1].qualified=0;
    items[2].title="The First Victory";items[2].unlocked_at=100;
    dyna_achievement_ui_init(argv[1]);assert(!dyna_achievement_ui_active());
    uint32_t pixels[320*224];for(size_t i=0;i<320*224;++i)pixels[i]=0xff203040u;
    uint32_t blank=checksum(pixels,320,320,224);
    dyna_achievement_ui_render(pixels,320,224,320*4);assert(checksum(pixels,320,320,224)==blank);
    event=(DynaAchievementUnlock){1,1};pending=1;items[0].unlocked_at=101;
    assert(dyna_achievement_ui_active()&&!dyna_achievement_ui_is_open());
    dyna_achievement_ui_render(pixels,320,224,320*4);assert(checksum(pixels,320,320,224)!=blank);
    assert(pixels[223*320]==0xff203040u); /* Toast doesn't dim the world. */
    dyna_achievement_ui_reset();assert(!dyna_achievement_ui_active());
    dyna_achievement_ui_open();assert(dyna_achievement_ui_is_open());
    dyna_achievement_ui_input(1,0,0);assert(dyna_achievement_ui_is_open());
    dyna_achievement_ui_input(0,1,1);assert(dyna_achievement_ui_is_open());
    clipped_render(320,224);clipped_render(768,224);clipped_render(96,96);
    for(size_t i=0;i<320*224;++i)pixels[i]=0xff203040u;
    dyna_achievement_ui_render(pixels,320,224,320*4);
    uint32_t first_view=checksum(pixels,320,320,224);
    if(argc==3){
        SDL_Surface *surface=SDL_CreateRGBSurfaceFrom(pixels,320,224,32,320*4,0xff0000,0xff00,0xff,0xff000000);
        assert(surface);assert(SDL_SaveBMP(surface,argv[2])==0);SDL_FreeSurface(surface);
    }
    dyna_achievement_ui_input(3,1,0);dyna_achievement_ui_input(3,1,1);dyna_achievement_ui_input(5,1,0);
    for(size_t i=0;i<320*224;++i)pixels[i]=0xff203040u;
    dyna_achievement_ui_render(pixels,320,224,320*4);
    assert(checksum(pixels,320,320,224)!=first_view);
    dyna_achievement_ui_input(1,1,0);assert(!dyna_achievement_ui_is_open());
    dyna_achievement_ui_input(0,1,0);assert(dyna_achievement_ui_is_open());
    dyna_achievement_ui_reset();SDL_Quit();return 0;
}
