/* DB2 Special writes RTS to $FF0070 at ROM $BA2 and $F718. The byte guard
 * is retained: a different RAM instruction is an unsupported native target. */
#include "m68k_step.h"
static void ram_return(M68kStepContext *ctx){
    uint32_t sp=ctx->state.regs.A[7];
    ctx->state.regs.PC=m68k_step_read32(ctx,sp)&0xFFFFFFu;
    ctx->state.regs.A[7]=sp+4u;
}
const M68kStepEntry dyna_db2_ram_return={0xFF0070u,{0x4E75u},1,16,ram_return};
