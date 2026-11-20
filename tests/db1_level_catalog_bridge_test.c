#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Unused host paths are removed by section GC for this focused bridge test. */
#include "../games/progress_runtime.c"
void dyna_extras_ui_notice(const char *text){(void)text;}
void dyna_achievement_session_save_loaded(void){}
static DynaAchievementItem final_stage={.id=183264};
size_t dyna_achievements_count(void){return 1;}
const DynaAchievementItem *dyna_achievements_item(size_t i){return i==0?&final_stage:NULL;}
int main(void){
 static unsigned char rom[0x20000];
 for(unsigned i=0;i<97;i++)for(unsigned j=0;j<4;j++)rom[0x1e87eu+4*i+j]=(unsigned char)(4*((i+j)%26));
 assert(dyna_progress_init(&store,1,"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
 assert(dyna_progress_session_init(&session,&store,rom,sizeof rom));
 dyna_progress_replay_init(&replay,1,rom,sizeof rom);ready=1;
 assert(count(NULL,1)==0 && count(NULL,2)==0);
 assert(!launch(NULL,1,0));
 final_stage.id=183310;
 final_stage.unlocked_at=1;
 DynaExtrasLevel out={0};
 assert(count(NULL,1)==1 && item(NULL,1,0,&out) && !strcmp(out.title,"Practice 1"));
 assert(!item(NULL,1,1,&out));
 final_stage.id=183264;
 assert(count(NULL,1)==2);
 assert(item(NULL,1,1,&out));
 assert(!strcmp(out.title,"Normal 1") && strstr(out.detail,"AVAILABLE TO REPLAY"));
 DynaProgressStore before=store;
 assert(launch(NULL,1,1)==1);
 assert(replay.status==DYNA_REPLAY_WAIT_PASSWORD && replay.requested.mode==1 && replay.requested.stage==1);
 assert(replay.requested.source_pc==0 && strstr(status,"LOADING SELECTED LEVEL"));
 assert(!memcmp(&before,&store,sizeof store));
 assert(!launch(NULL,1,99));
 final_stage.id=183282;
 assert(count(NULL,1)==3 && item(NULL,1,2,&out) && !strcmp(out.title,"Hard 1"));
 assert(!item(NULL,1,3,&out));
 final_stage.id=183302;
 assert(count(NULL,1)==4 && item(NULL,1,3,&out) && !strcmp(out.title,"Back 1"));
 assert(!item(NULL,1,4,&out));
 store.game=2;assert(count(NULL,2)==0);assert(!item(NULL,2,0,&out));assert(!launch(NULL,2,0));
 puts("Runtime catalogue bridge labels/request/pending/immutability and DB2 empty path passed");return 0;
}
void dyna_extras_ui_set_level_provider(const DynaExtrasLevelProvider *p){(void)p;}
void dyna_extras_ui_reset(void){}
void dyna_extras_ui_open(void){}
void dyna_extras_ui_input(int a,int b,int c){(void)a;(void)b;(void)c;}
DynaExtrasUiState dyna_extras_ui_state(void){DynaExtrasUiState s={0};return s;}
void dyna_extras_ui_render(uint32_t *p,int w,int h,int pitch){(void)p;(void)w;(void)h;(void)pitch;}
int dyna_extras_ui_is_open(void){return 0;}
