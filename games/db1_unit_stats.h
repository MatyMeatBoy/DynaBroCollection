#ifndef DB1_UNIT_STATS_H
#define DB1_UNIT_STATS_H
#include <stddef.h>
#include <stdint.h>

/* Original DB1 parameter slots, NOT human-facing HP/attack/speed ratings.
 * Species-name association and game-unit scaling remain unverified.
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
#endif
