#ifndef DB1_ACHIEVEMENT_EVENTS_H
#define DB1_ACHIEVEMENT_EVENTS_H
#include <stddef.h>
#include <stdint.h>
/* Local interpretation of the public achievement goals, not RA trigger logic.
 * RAM is a big-endian byte image of FF0000..FFFFFF. Call before each main-CPU
 * instruction. eligible must be false for debugger/test injection or unverified
 * ROMs; dropping eligibility cancels the in-flight battle. No RAM is modified. */
typedef void (*Db1AchievementEmit)(void *user, uint32_t official_id);
typedef struct {
    uint32_t descriptor, king_candidate;
    uint16_t stage, previous_enemy_defeats;
    uint8_t armed, saw_victory_return;
    uint8_t timeout_pending, timeout_triggered;
    uint8_t enemy_defeats, enemy_counter_valid;
    uint8_t surf_pending, surf_emitted, king_emitted;
    uint8_t disaster_casts; /* Earthquake1, Tsunami2, Meteor4 */
    uint8_t rain_casts; /* saturates at the public goal threshold100 */
} Db1AchievementEvents;
void db1_achievement_events_reset(Db1AchievementEvents *);
void db1_achievement_events_observe(Db1AchievementEvents *, uint32_t before_pc,
    const uint8_t *ram, size_t ram_size, int eligible,
    Db1AchievementEmit emit, void *user);
/* Extended observer qualifies the live actor pointer for King Tyranno.
 * Pass the main CPU A2 value before the instruction. The compatibility
 * observer above handles every other goal but cannot qualify this one. */
void db1_achievement_events_observe_cpu(Db1AchievementEvents *, uint32_t before_pc,
    const uint8_t *ram, size_t ram_size, int eligible, uint32_t a2,
    Db1AchievementEmit emit, void *user);
#endif
