/* Initial adapter: addresses read from the dump's vector table. */
#include "game_spec.h"
#include "genesis_runtime.h"
static void entry(void) { recomp_call_addr(DYNA_ENTRY); }
static void vint(void) { recomp_call_addr(DYNA_VINT); }
static void hint(void) { recomp_call_addr(DYNA_HINT); }
#ifdef DYNA_DB2
/* The cartridge deliberately uses FFxx as compact software calls. Execute
 * its original vector-11 handler with the real six-byte exception frame,
 * then dispatch the PC restored by its RTE. No game helper is replaced. */
static int db2_instruction_hook(uint32_t pc) {
    uint16_t opcode = m68k_read16(pc);
    if ((opcode & 0xFF00u) != 0xFF00u || (opcode & 255u) >= 62u)
        return 0;
    uint16_t sr = g_cpu.SR;
    g_cpu.A[7] -= 4;
    m68k_write32(g_cpu.A[7], pc);
    g_cpu.A[7] -= 2;
    m68k_write16(g_cpu.A[7], sr);
    g_cpu.SR |= 0x2000u;
    g_native_insn_count++;
    g_cycle_accumulator += 34;
    g_audio_cycle_counter += 34;
    recomp_call_addr(0x694u);
    g_rte_pending = 0;
    recomp_tail_call(g_cpu.PC);
    return 1;
}
#endif
const GameSpec g_game_spec = {
    .display_name = DYNA_NAME,
    .short_name = DYNA_NAME,
    .expected_rom_crc32 = DYNA_CRC,
    .expected_rom_size = DYNA_SIZE,
    .tier3_floor_default = 1,
    .call_entry_point = entry,
    .call_vblank = vint,
    .call_hblank = hint,
#ifdef DYNA_DB2
    .instruction_hook = db2_instruction_hook,
#endif
};
