#include "step_lookup.h"
#include <assert.h>
#include <string.h>
static uint8_t memory[]={0x30,0x3c,0x12,0x34};
static unsigned fetches;
static M68kStepBusResult fetch(void *u,uint32_t address,uint16_t *value){(void)u;++fetches;address&=3;*value=(uint16_t)((memory[address]<<8)|memory[address+1]);return M68K_BUS_OK;}
static void one(M68kStepContext *c){++c->state.regs.D[0];c->state.regs.PC+=4;}
static void two(M68kStepContext *c){c->state.regs.D[0]+=2;c->state.regs.PC+=4;}
static const M68kStepEntry a[]={ {0x100,{0x303c,0x1234},2,8,one} };
static const M68kStepEntry b[]={ {0x100,{0x303c,0x5678},2,8,two} };
static const M68kStepModule ma={"a",0x100,4,0,a,1},mb={"b",0x100,4,0,b,1};
int main(void) {
    M68kStepBus bus={.read16=fetch,.fetch16=fetch};M68kStepContext c;
    m68k_step_init(&c,&bus,0);c.state.regs.SR=0x2300;c.state.regs.PC=0x100;dyna_step_cache_clear();
    assert(dyna_step_cached(&c,&ma)==M68K_STEP_OK);assert(c.state.regs.D[0]==1);
    c.state.regs.PC=0x100;unsigned before=fetches;
    assert(dyna_step_cached(&c,&ma)==M68K_STEP_OK);assert(fetches==before+2);
    c.state.regs.PC=0x100;memory[3]=0x35;
    M68kStepState state=c.state;
    assert(dyna_step_cached(&c,&ma)==M68K_STEP_MISS);assert(!memcmp(&state,&c.state,sizeof state));
    memory[3]=0x34;
    assert(dyna_step_cached(&c,&ma)==M68K_STEP_OK);assert(c.state.regs.D[0]==3);
    c.state.regs.PC=0x100;
    memory[2]=0x56;memory[3]=0x78;
    assert(dyna_step_cached(&c,&mb)==M68K_STEP_OK);assert(c.state.regs.D[0]==5);
    c.state.regs.PC=0x102;state=c.state;assert(dyna_step_cached(&c,&ma)==M68K_STEP_MISS);assert(!memcmp(&state,&c.state,sizeof state));
    before=fetches;
    assert(dyna_step_cached(&c,&ma)==M68K_STEP_MISS);assert(fetches==before);assert(!memcmp(&state,&c.state,sizeof state));
    return 0;
}
