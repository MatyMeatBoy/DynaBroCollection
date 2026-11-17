#include "m68k_step.h"
#include <assert.h>
#include <string.h>
extern const M68kStepEntry dyna_db2_ram_return;
static unsigned char memory[65536];
static M68kStepBusResult read16(void *u,uint32_t a,uint16_t *v){(void)u;*v=(uint16_t)((memory[a&65535]<<8)|memory[(a+1)&65535]);return M68K_BUS_OK;}
int main(void){
    M68kStepBus bus={.read16=read16,.fetch16=read16};
    M68kStepContext ctx;
    m68k_step_init(&ctx,&bus,0);
    ctx.state.regs.PC=0xFF0070;ctx.state.regs.SR=0x2300;ctx.state.regs.A[7]=0xFF8000;
    memory[0x70]=0x4E;memory[0x71]=0x75;
    memory[0x8000]=0;memory[0x8001]=1;memory[0x8002]=0x23;memory[0x8003]=0x46;
    assert(m68k_step_execute(&ctx,&dyna_db2_ram_return)==M68K_STEP_OK);
    assert(ctx.state.regs.PC==0x12346 && ctx.state.regs.A[7]==0xFF8004);
    assert(ctx.state.regs.SR==0x2300 && ctx.state.cycles==16);
    ctx.state.regs.PC=0xFF0070;ctx.state.regs.A[7]=0xFF8000;
    memory[0x71]=0x71; /* another opcode must never reuse the RTS translation */
    assert(m68k_step_execute(&ctx,&dyna_db2_ram_return)==M68K_STEP_MISS);
    assert(ctx.state.regs.PC==0xFF0070 && ctx.state.regs.A[7]==0xFF8000);
    return 0;
}
