#include "db1_unit_stats.h"

int db1_unit_parameters_read(const uint8_t *rom, size_t rom_size,
                             unsigned slot, Db1UnitParameters *out) {
    Db1UnitParameters result;
    size_t offset;
    if (!rom || !out || slot < 1 || slot > 5) return 0;
    offset = DB1_UNIT_PARAMETER_BASE + (size_t)slot * DB1_UNIT_PARAMETER_STRIDE;
    if (offset > rom_size || DB1_UNIT_PARAMETER_STRIDE > rom_size - offset) return 0;
    result.slot = slot;
    result.rom_offset = offset;
    for (unsigned i = 0; i < DB1_UNIT_PARAMETER_WORDS; ++i)
        result.words[i] = (uint16_t)(((unsigned)rom[offset + 2*i] << 8) |
                                    rom[offset + 2*i + 1]);
    *out = result;
    return 1;
}
