#ifndef DYNA_ACHIEVEMENT_SESSION_H
#define DYNA_ACHIEVEMENT_SESSION_H
#include <stddef.h>
#include <stdint.h>
void dyna_achievement_session_init(const char *settings,int game2);
void dyna_achievement_session_reset(void);
void dyna_achievement_session_observe(uint32_t pc);
extern int dyna_achievement_tracking;
/* Most instructions have no progression event. Keep their hot path to a
 * bounded page lookup; the observer still validates each exact event PC. */
static inline void dyna_achievement_session_before(uint32_t pc) {
    static const uint8_t pages[512]={
        [0x1c]=1,[0x28]=1,[0xa1]=1,[0x11e]=1,[0x11f]=1,[0x121]=1,
        [0x122]=1,[0xd3]=1,[0x3b]=1,[0x6f]=1,[0x3a]=1,[0x3c]=1,
        [0x142]=1,[0x140]=1,[0x145]=1,[0xc7]=1
    };
    if(dyna_achievement_tracking && pc<0x20000 && pages[pc>>8])
        dyna_achievement_session_observe(pc);
}
void dyna_achievement_session_frame(uint64_t frame);
void dyna_achievement_session_diagnostic(void);
void dyna_achievement_session_save_loaded(void);
void dyna_achievement_session_sram_loaded(const char *path,const uint8_t *data,size_t size,int loaded);
int dyna_achievement_session_action(const char *key);
#endif
