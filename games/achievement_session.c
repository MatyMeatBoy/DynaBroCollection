#include "achievement_session.h"
#include "achievements.h"
#include "achievement_ui.h"
#include "achievement_smoke.h"
#include "db1_achievement_events.h"
#include "db2_achievement_events.h"
#include "genesis_runtime.h"
#include "input_script.h"
#include "diagnostic_replay.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int game2,registered;
int dyna_achievement_tracking=1;
static Db1AchievementEvents db1;
static Db2AchievementEvents db2;
static void flush_exit(void){(void)dyna_achievements_flush();}
void dyna_achievement_session_init(const char *settings,int second) {
    game2=second;dyna_achievement_tracking=1;
    if(!dyna_achievements_init(settings,second))
        fprintf(stderr,"[achievements] Profile could not be read; goals remain locked.\n");
    char root[2048];char *base=SDL_GetBasePath();
    int n=snprintf(root,sizeof root,"%sassets/achievements",base?base:"");
    SDL_free(base);
    dyna_achievement_ui_init(n>=0 && n<(int)sizeof root?root:"");
    dyna_achievement_smoke_init();
    if(!registered){atexit(flush_exit);registered=1;}
    dyna_achievement_session_reset();
}
void dyna_achievement_session_reset(void) {
    db1_achievement_events_reset(&db1);db2_achievement_events_reset(&db2);
}
void dyna_achievement_session_diagnostic(void){dyna_achievement_tracking=0;dyna_achievement_session_reset();}
void dyna_achievement_session_observe(uint32_t pc) {
    switch(pc) {
    case 0x1c40:case 0x1c44:case 0x28ae:case 0xa10e:case 0xa13c:
    case 0xa162:case 0x11ef4:case 0x11ff6:case 0x12224:case 0x12194:case 0xd37a:
    case 0x1429e:case 0x14070:case 0x14566:case 0xc7a6:case 0xc7b8:
    case 0x3b1e:case 0x6f9a:case 0x3a38:case 0x3c70:case 0x3bb0:case 0x3bb4:case 0x3cb2:break;
    default:return;
    }
    /* Replay and scripted tests are deliberately excluded from progression.
     * Dropping eligibility invalidates the whole in-flight battle. */
    if(dyna_achievement_tracking && (input_script_active() || genesis_replay_playing()))
        dyna_achievement_session_diagnostic();
    if(!dyna_achievement_tracking)return;
    if(game2)db2_achievement_events_observe(&db2,pc,g_ram,sizeof g_ram,1,dyna_achievements_emit,NULL);
    else db1_achievement_events_observe_cpu(&db1,pc,g_ram,sizeof g_ram,1,g_cpu.A[2],dyna_achievements_emit,NULL);
}
void dyna_achievement_session_frame(uint64_t frame) {
    (void)frame;
    dyna_achievement_smoke_frame(frame);
    (void)dyna_achievements_flush();
}
int dyna_achievement_session_action(const char *key) {
    if(!key || strcmp(key,"dyna.achievements"))return 0;
    dyna_achievement_ui_open();return 1;
}
