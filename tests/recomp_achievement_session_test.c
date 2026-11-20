#include "achievement_session.h"
#include "achievement_smoke.h"
#include "achievement_ui.h"
#include "achievements.h"
#include "diagnostic_replay.h"
#include "genesis_runtime.h"
#include "input_script.h"
#include <assert.h>
#include <string.h>

M68KState g_cpu;
uint8_t g_ram[65536];
static bool scripted;

bool input_script_active(void) { return scripted; }
int genesis_replay_playing(void) { return 0; }
void dyna_achievement_ui_init(const char *assets) { (void)assets; }
void dyna_achievement_ui_open(void) {}
void dyna_achievement_smoke_init(void) {}
void dyna_achievement_smoke_frame(uint64_t frame) { (void)frame; }

static void db1_practice_win(unsigned stage) {
    const unsigned offset=0xaddeu+(stage-1)*0x4au;
    g_ram[0x2f]=0xff;
    g_ram[0x30]=(uint8_t)(offset>>8);
    g_ram[0x31]=(uint8_t)offset;
    g_ram[offset]=(uint8_t)stage;
    g_ram[0x3d]=0;
    dyna_achievement_session_before(0x1c40);
    g_ram[0x3d]=1;
    dyna_achievement_session_before(0x28ae);
    dyna_achievement_session_before(0x1c44);
}

static void db2_practice_win(unsigned stage) {
    static const uint8_t maps[]={51,53,52,54,55,57,58,56};
    g_ram[0x279]=0xff;
    g_ram[0x27a]=0x02;
    g_ram[0x27b]=0x92;
    g_ram[0x27d]=g_ram[0x293]=(uint8_t)stage;
    g_ram[0x294]=stage==8?255:0;
    g_ram[0x295]=maps[stage-1];
    g_ram[0x9c86]=0;
    dyna_achievement_session_before(0x3bb0);
    g_ram[0x9c86]=1;
    dyna_achievement_session_before(0x3bb4);
    dyna_achievement_session_before(0x3cb2);
}

int main(int argc, char **argv) {
    assert(argc==3);
    const int db1=!strcmp(argv[2],"db1");
    const int cheat=!strcmp(argv[2],"cheat");
    const int final_only=!strcmp(argv[2],"final");
    dyna_achievement_session_init(argv[1],!db1);
    dyna_achievement_session_frame(1);
    if(db1) {
        dyna_achievement_session_save_loaded();
        db1_practice_win(1);
        db1_practice_win(1);
        scripted=true;
        db1_practice_win(2);
        scripted=false;
        db1_practice_win(3);
        assert(!dyna_achievement_tracking);
    } else {
        if(cheat) {
            g_ram[0x4c2]=0x55;
            g_ram[0x4c3]=0x48;
            g_ram[0x4c4]=0x59;
            g_ram[0x4c5]=0x6f;
        }
        db2_practice_win(8);
        if(!final_only && !cheat)
            for(unsigned stage=1;stage<8;++stage)db2_practice_win(stage);
    }
    dyna_achievement_session_frame(2);
    return 0;
}
