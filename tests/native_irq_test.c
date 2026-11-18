#include "m68k_step.h"
#include "native_interrupt_frame.h"
#include <assert.h>
#include <string.h>

static uint8_t memory[65536];
static M68kStepBusResult read8(void *u,uint32_t a,uint8_t *v){(void)u;*v=memory[a&65535];return M68K_BUS_OK;}
static M68kStepBusResult read16(void *u,uint32_t a,uint16_t *v){(void)u;*v=(uint16_t)((memory[a&65535]<<8)|memory[(a+1)&65535]);return M68K_BUS_OK;}
static M68kStepBusResult write8(void *u,uint32_t a,uint8_t v){(void)u;memory[a&65535]=v;return M68K_BUS_OK;}
static M68kStepBusResult write16(void *u,uint32_t a,uint16_t v){(void)u;memory[a&65535]=(uint8_t)(v>>8);memory[(a+1)&65535]=(uint8_t)v;return M68K_BUS_OK;}
static void compiled_rte(M68kStepContext *c){m68k_step_rte(c);}
static const M68kStepEntry rte={.pc=0x200,.words={0x4e73},.word_count=1,.cycles=20,.execute=compiled_rte};

int main(void){
    int vint=1,hint=1;
    assert(!native_interrupt_pending_level(vint,hint,0x2600,1));
    assert(vint&&hint);
    assert(native_interrupt_pending_level(vint,hint,0x2300,1)==6);
    assert(vint&&hint);
    native_interrupt_ack_pending(&vint,&hint,6);
    assert(!vint&&hint);
    assert(!native_interrupt_pending_level(vint,hint,0x2300,0));
    assert(hint);
    assert(!native_interrupt_pending_level(vint,hint,0x2400,1));
    assert(native_interrupt_pending_level(vint,hint,0x2300,1)==4);
    native_interrupt_ack_pending(&vint,&hint,4);
    assert(!hint);

    M68kStepBus bus={.read8=read8,.read16=read16,.write8=write8,.write16=write16,.fetch16=read16};
    M68kStepContext ctx;
    m68k_step_init(&ctx,&bus,0);
    ctx.state.regs.PC=0x1234;ctx.state.regs.SR=0xa315;ctx.state.regs.A[7]=0xff8000;
    ctx.state.regs.D[2]=0xdeadbeef;ctx.state.stopped=1;
    memory[0x7a]=2;memory[0x200]=0x4e;memory[0x201]=0x73;
    assert(m68k_step_interrupt(&ctx,6));
    assert(ctx.state.regs.PC==0x200&&ctx.state.regs.A[7]==0xff7ffa);
    assert(ctx.state.regs.SR==0x2615&&!ctx.state.stopped);
    assert(ctx.state.cycles==44&&ctx.state.regs.D[2]==0xdeadbeef);
    uint16_t sr;uint32_t pc;
    native_interrupt_frame_read(memory,0xff7ffa,&sr,&pc);
    assert(sr==0xa315&&pc==0x1234);
    /* Guest-owned return context: the host must not overwrite this frame. */
    native_interrupt_frame_write(memory,0xff7ffa,0x2315,0x5678);
    assert(m68k_step_execute(&ctx,&rte)==M68K_STEP_OK);
    assert(ctx.state.regs.PC==0x5678&&ctx.state.regs.SR==0x2315&&ctx.state.regs.A[7]==0xff8000);
    assert(ctx.state.cycles==64);

    /* User mode must switch to SSP and recover USP through the core's RTE. */
    m68k_step_init(&ctx,&bus,0);
    ctx.state.regs.PC=0x1234;ctx.state.regs.SR=0x15;ctx.state.regs.A[7]=0xff6000;ctx.state.ssp=0xff8000;
    assert(m68k_step_interrupt(&ctx,6));
    assert(ctx.state.regs.A[7]==0xff7ffa&&ctx.state.regs.USP==0xff6000);
    assert(m68k_step_execute(&ctx,&rte)==M68K_STEP_OK);
    assert(ctx.state.regs.A[7]==0xff6000&&ctx.state.regs.SR==0x15&&ctx.state.regs.PC==0x1234);

    m68k_step_init(&ctx,&bus,0);memory[0x72]=2;
    ctx.state.regs.PC=0x1234;ctx.state.regs.SR=0x2315;ctx.state.regs.A[7]=0xff8000;
    assert(m68k_step_interrupt(&ctx,4));
    assert(ctx.state.regs.SR==0x2415&&ctx.state.regs.A[7]==0xff7ffa);
    assert(m68k_step_interrupt(&ctx,6));
    assert(ctx.state.regs.SR==0x2615&&ctx.state.regs.A[7]==0xff7ff4);
    assert(m68k_step_execute(&ctx,&rte)==M68K_STEP_OK);
    assert(ctx.state.regs.SR==0x2415&&ctx.state.regs.A[7]==0xff7ffa);
    assert(m68k_step_execute(&ctx,&rte)==M68K_STEP_OK);
    assert(ctx.state.regs.SR==0x2315&&ctx.state.regs.A[7]==0xff8000&&ctx.state.regs.PC==0x1234);
    return 0;
}
