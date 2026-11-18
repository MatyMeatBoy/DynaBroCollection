#ifndef DB1_UNIT_STATS_H
#define DB1_UNIT_STATS_H
#include <stddef.h>
#include <stdint.h>

/* Original DB1 parameter slots, NOT human-facing HP/attack/speed ratings.
 * Species association is verified through the original credits renderer.
 * See docs/research/db1-unit-stat-contracts.md before presenting these values. */
enum { DB1_UNIT_PARAMETER_BASE = 0xf08a, DB1_UNIT_PARAMETER_STRIDE = 0x80,
       DB1_UNIT_PARAMETER_WORDS = 64 };
typedef struct {
    unsigned slot;
    size_t rom_offset;
    uint16_t words[DB1_UNIT_PARAMETER_WORDS];
} Db1UnitParameters;

/* Accepts species slot 1..5, not an actor ID. Reads a complete 128-byte entry.
 * No mutation or CPU/RNG execution. Failure preserves *out. Input/output must
 * not alias. This is a decoder, not a ROM identity verifier. */
int db1_unit_parameters_read(const uint8_t *rom, size_t rom_size,
                             unsigned slot, Db1UnitParameters *out);

enum { DB1_UNIT_STEGO = 1, DB1_UNIT_TRICERA, DB1_UNIT_TYRANNO,
       DB1_UNIT_ALLO, DB1_UNIT_PCHAN, DB1_UNIT_SPECIES_COUNT = 5 };
typedef struct {
    unsigned species;
    /* Shared vitality is changed by combat, food/activity and reproduction.
     * Initial value is NOT a maximum. Reproduction requires a value strictly
     * greater than its threshold; cost is parental vitality, not player power. */
    uint16_t low_vitality_threshold;
    uint16_t reproduction_threshold;
    uint16_t reproduction_cost;
    uint16_t initial_vitality;
    /* Per execution of C730..C734, target order follows species enum above.
     * No attacks-per-second claim; evolved attacker may apply this twice. */
    uint16_t combat_drain[DB1_UNIT_SPECIES_COUNT];
} Db1UnitStats;
int db1_unit_stats_read(const uint8_t *rom, size_t rom_size,
                       unsigned species, Db1UnitStats *out);
#endif
