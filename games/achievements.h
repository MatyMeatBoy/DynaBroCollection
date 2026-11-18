#ifndef DYNA_ACHIEVEMENTS_H
#define DYNA_ACHIEVEMENTS_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t id;
    const char *title, *description;
    unsigned points;
    const char *badge_file_locked, *badge_file_unlocked;
    int qualified;
    uint64_t unlocked_at; /* Offline Unix seconds; zero means locked. */
} DynaAchievementItem;
typedef struct { uint32_t id; uint64_t serial; } DynaAchievementUnlock;
/* settings_path is the existing settings filename; profile lives beside it.
 * Returns zero on invalid path/corrupt profile, with all goals safely locked.
 * Missing profile is successful. No network, accounts or RA synchronization. */
int dyna_achievements_init(const char *settings_path,int game2);
void dyna_achievements_emit(void *user,uint32_t id);
/* Atomic replacement; failure keeps dirty state for the next retry. */
int dyna_achievements_flush(void);
size_t dyna_achievements_count(void);
const DynaAchievementItem *dyna_achievements_item(size_t index);
DynaAchievementUnlock dyna_achievements_last_unlock(void);
/* FIFO notification queue; returns1 and consumes an event, or0 if empty/NULL. */
int dyna_achievements_pop_unlock(DynaAchievementUnlock *out);
#endif
