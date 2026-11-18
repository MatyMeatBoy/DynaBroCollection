#include "db1_unit_stats.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t rom[0x100000];
int main(int argc, char **argv) {
    Db1UnitParameters p, before;
    memset(&p, 0xa5, sizeof p); before = p;
    assert(!db1_unit_parameters_read(NULL, sizeof rom, 1, &p));
    assert(!db1_unit_parameters_read(rom, sizeof rom, 0, &p));
    assert(!db1_unit_parameters_read(rom, sizeof rom, 6, &p));
    assert(!db1_unit_parameters_read(rom, sizeof rom, ~0u, &p));
    assert(!db1_unit_parameters_read(rom, 0xf189, 1, &p));
    assert(!db1_unit_parameters_read(rom, 0, 1, &p));
    assert(!memcmp(&p, &before, sizeof p));
    assert(!db1_unit_parameters_read(rom, sizeof rom, 1, NULL));
    for (unsigned slot = 1; slot <= 5; ++slot) {
        size_t offset = 0xf08a + slot * 128;
        for (unsigned i = 0; i < 128; ++i) rom[offset+i] = (uint8_t)i;
        assert(db1_unit_parameters_read(rom, offset+128, slot, &p));
        assert(p.slot == slot && p.rom_offset == offset);
        for (unsigned i = 0; i < 64; ++i)
            assert(p.words[i] == (uint16_t)((2*i << 8) | (2*i+1)));
    }
    /* Optional private original-ROM check; no copyrighted ROM fixture needed. */
    if (argc == 2) {
        static const unsigned first[5] = {800,800,700,1000,800};
        static const unsigned initial[5] = {1000,1000,1500,2000,1500};
        FILE *f = fopen(argv[1], "rb"); assert(f);
        assert(fread(rom, 1, sizeof rom, f) == sizeof rom);
        assert(fgetc(f) == EOF); fclose(f);
        for (unsigned slot = 1; slot <= 5; ++slot) {
            assert(db1_unit_parameters_read(rom, sizeof rom, slot, &p));
            assert(p.words[0] == first[slot-1]);
            assert(p.words[3] == initial[slot-1]);
        }
    }
    puts("db1_unit_stats_test: passed");
    return 0;
}
