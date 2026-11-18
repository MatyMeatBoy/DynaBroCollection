#ifndef DB2_ACHIEVEMENT_EVENTS_H
#define DB2_ACHIEVEMENT_EVENTS_H
#include <stddef.h>
#include <stdint.h>
/* Custom local goals, not RetroAchievements definitions. RAM is a big-endian
 * byte image of FF0000..FFFFFF. Observe before each main CPU instruction.
 * eligible must be false for unverified ROMs and diagnostic/debug injection.
 * Dropping eligibility cancels the in-flight stage. This module never writes RAM.
 * Completion milestones are derived from persisted stage goals by the frontend. */
typedef void (*Db2AchievementEmit)(void *user, uint32_t local_id);
typedef struct {
    uint8_t mode, stage, armed, saw_battle_return;
} Db2AchievementEvents;
void db2_achievement_events_reset(Db2AchievementEvents *);
void db2_achievement_events_observe(Db2AchievementEvents *, uint32_t before_pc,
    const uint8_t *ram, size_t ram_size, int eligible,
    Db2AchievementEmit emit, void *user);
#endif
