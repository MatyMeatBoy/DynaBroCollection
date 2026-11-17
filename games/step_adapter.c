/* Explicit-PC static operations use the shared MC68000 AOT ABI. */
#include "genesis_runtime.h"
#include "glue.h"
#include "game_spec.h"
#include "native_interruptible.h"
#include "video/genesis_machine.h"
#include "m68k_step.h"
#include "app_config.h"
#include "input_map.h"
#include "language_settings.h"
#include "language_patch.h"
#include "controller_profiles.h"
#include "step_lookup.h"
#include DYNA_LANGUAGE_HEADER
#if RECOMP_LAUNCHER
#include "recomp_runtime_ui.h"
static const char *const controller_choices[]={"Automatic","Custom SDL bindings","M30 Bluetooth / DirectInput","M30 Mini 2.4GHz receiver","M30 XInput"};
static const RecompRuntimeUiItem language_items[]={
    {"dyna.english","Language","English translation (restart)",
     "Partial translation. Changes apply after restarting the application; the running game keeps its current language.",
     RECOMP_RUNTIME_UI_BOOL,0,1,1},
    {"controls.p1_profile","Controls","Player 1 layout","Automatic detects known M30 devices. Select an M30 preset for shared/generic IDs; Custom uses your SDL bindings.",RECOMP_RUNTIME_UI_CHOICE,0,4,1,controller_choices,5},
    {"controls.p2_profile","Controls","Player 2 layout","Saved per connected device; does not overwrite another controller's bindings.",RECOMP_RUNTIME_UI_CHOICE,0,4,1,controller_choices,5}
};
#endif
#include DYNA_STEP_HEADER
#ifdef DYNA_HAS_JP
#include "db1jp.h"
#include "translation_identity_db1.h"
static const GameRomAlias db1_aliases[]={
    {"Japanese original",0x360C1B20u},
    {"English map-label repairs (partial)",DYNA_DB1_ENGLISH_CRC}
};
#endif
#ifdef DYNA_HAS_OVERLAY
#include "db2overlay.h"
#include "translation_identity.h"
static const GameRomAlias english_prompts_alias={"English gameplay prompts (partial)",DYNA_DB2_ENGLISH_CRC};
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void genesis_native_mainloop(uint32_t, GenesisNativeInstruction, void *);
extern void glue_own_interrupt(int, GVDP *);
static M68kStepContext main_cpu;
static int cached_lookup=1;
static M68kStepResult execute_module(M68kStepContext *ctx,const M68kStepModule *module) {
    return cached_lookup?dyna_step_cached(ctx,module):m68k_step_module_execute(ctx,module);
}
static uint32_t expected_hint_audio;
extern const M68kStepEntry dyna_db2_ram_return;
static M68kStepBusResult read8(void *u,uint32_t a,uint8_t *v) { (void)u;*v=m68k_read8(a);return M68K_BUS_OK; }
static M68kStepBusResult read16(void *u,uint32_t a,uint16_t *v) { (void)u;*v=m68k_read16(a);return M68K_BUS_OK; }
static M68kStepBusResult fetch16(void *u,uint32_t a,uint16_t *v) { (void)u;*v=glue_peek16(a);return M68K_BUS_OK; }
static M68kStepBusResult write8(void *u,uint32_t a,uint8_t v) { (void)u;m68k_write8(a,v);return M68K_BUS_OK; }
static M68kStepBusResult write16(void *u,uint32_t a,uint16_t v) { (void)u;m68k_write16(a,v);return M68K_BUS_OK; }
static const M68kStepBus bus={.read8=read8,.read16=read16,.write8=write8,.write16=write16,.fetch16=fetch16};

static int retire(void *user,unsigned *cycles) {
    M68kStepContext *ctx=user;
    if(DYNA_ENTRY==0x200u && g_cpu.PC==0x9828u && getenv("DYNA_LZ_TRACE")) {
        uint32_t dst=glue_peek32(g_cpu.A[7]+4u);
        uint32_t src=glue_peek32(g_cpu.A[7]+8u);
        fprintf(stderr,"[DB1-LZ] source=$%06X target=$%06X bytes=%u method=%u\n",
                src,dst,glue_peek16(src),glue_peek8(src+2u));
    }
    if(ctx==&main_cpu && !ctx->state.instructions && getenv("DYNA_IRQ_SELFTEST")) {
        g_cpu.SR=0x2300u;
        g_machine.vdp.reg[0]|=16u;
        glue_own_interrupt(4,&g_machine.vdp);
    }
    if(DYNA_ENTRY==0x738u && (g_cpu.PC==0xFDEu || g_cpu.PC==0x1080u) && getenv("DYNA_LZ_TRACE")) {
        uint32_t dst=glue_peek32(g_cpu.A[6]);
        uint32_t src=glue_peek32(g_cpu.A[6]+4u);
        fprintf(stderr,"[LZ] routine=$%04X source=$%06X target=$%06X bytes=%u\n",g_cpu.PC,src,dst,glue_peek16(src));
    }
    _Static_assert(sizeof(ctx->state.regs)==sizeof(g_cpu),"MC68000 register ABI");
    memcpy(&ctx->state.regs,&g_cpu,sizeof(g_cpu));
    uint64_t before=ctx->state.cycles;
    M68kStepResult result=execute_module(ctx,&DYNA_STEP_MODULE);
#ifdef DYNA_HAS_JP
    if(result==M68K_STEP_MISS)
        result=execute_module(ctx,&db1jp_module);
#endif
#ifdef DYNA_HAS_OVERLAY
    if(result==M68K_STEP_MISS)
        result=execute_module(ctx,&db2overlay_module);
#endif
    if(result==M68K_STEP_MISS && DYNA_ENTRY==0x738u && g_cpu.PC==0xFF0070u)
        result=m68k_step_execute(ctx,&dyna_db2_ram_return);
    memcpy(&g_cpu,&ctx->state.regs,sizeof(g_cpu));
    if(result==M68K_STEP_STOPPED){*cycles=4;return 1;}
    if(result!=M68K_STEP_OK) {
        fprintf(stderr,"[AOT] missing/invalid operation PC=$%06X opcode=$%04X result=%d\n",g_cpu.PC,glue_peek16(g_cpu.PC),result);
        return 0;
    }
    *cycles=(unsigned)(ctx->state.cycles-before);
    if(ctx==&main_cpu && ctx->state.instructions==1 && getenv("DYNA_IRQ_SELFTEST"))
        expected_hint_audio=g_audio_cycle_counter+*cycles+44u;
    return 1;
}
static void entry(void) {
    const char *lookup=getenv("DYNA_AOT_LOOKUP");
    cached_lookup=!lookup||strcmp(lookup,"binary");
    dyna_step_cache_clear();
    char error[160];
    if(!dyna_language_apply(g_rom,DYNA_SIZE,&DYNA_LANGUAGE_IMAGE,dyna_language_english(),error,sizeof error)) {
        fprintf(stderr,"[language] %s\n",error);abort();
    }
    dyna_language_started();
    fprintf(stderr,"[language] %s (translation remains partial)\n",dyna_language_english()?"English":"Japanese original");
    /* Both qualified dumps are Japan NTSC. DB2 reads $A10001 & $C0 at
     * $000A54 and rejects the shared runner's overseas default $80. */
    g_machine.bus.version=0x00;
    m68k_step_init(&main_cpu,&bus,NULL);
    genesis_native_mainloop(DYNA_ENTRY,retire,&main_cpu);
}
static void interrupt(uint32_t pc,unsigned level) {
    M68kStepContext irq;
    m68k_step_init(&irq,&bus,NULL);
    uint32_t sp=g_cpu.A[7];
    if(getenv("DYNA_IRQ_VERIFY")) {
        static unsigned checked;
        uint16_t saved_sr=glue_peek16(sp);
        if(glue_peek32(sp+2u)!=(g_cpu.PC&0xFFFFFFu) ||
           ((g_cpu.SR>>8)&7u)!=level || ((saved_sr>>8)&7u)>=level) {
            fprintf(stderr,"[AOT-IRQ] INVALID level=%u sp=$%06X saved_pc=$%06X interrupted_pc=$%06X sr=%04X saved_sr=%04X\n",
                    level,sp,glue_peek32(sp+2u),g_cpu.PC,g_cpu.SR,saved_sr);
            abort();
        }
        if(level==4 && !(checked&(1u<<4)) && getenv("DYNA_IRQ_SELFTEST") && g_audio_cycle_counter!=expected_hint_audio) {
            fprintf(stderr,"[AOT-IRQ] INVALID exception entry cost: actual=%u expected=%u\n",g_audio_cycle_counter,expected_hint_audio);
            abort();
        }
        if(!(checked&(1u<<level))) {
            checked|=1u<<level;
            fprintf(stderr,"[AOT-IRQ] PASS level=%u real exception frame\n",level);
        }
    }
    g_cpu.PC=pc;
    main_cpu.state.stopped=0;
    for(unsigned guard=0;guard<1000000u;++guard) {
        int returning=g_cpu.A[7]==sp && glue_peek16(g_cpu.PC)==0x4E73u;
        unsigned cycles=0;
        if(!retire(&irq,&cycles))abort();
        g_native_insn_count++;
        g_cycle_accumulator+=cycles;g_audio_cycle_counter+=cycles;
        if(returning)return;
    }
    fprintf(stderr,"[AOT] IRQ instruction guard\n");abort();
}
static void vint(void){interrupt(DYNA_VINT,6);}
static void hint(void){interrupt(DYNA_HINT,4);}
static void settings(const char *path){
    dyna_language_settings_load(path);
    /* Use the common parser again after installing game defaults. Explicit
     * saved 3-button choices and unrelated settings retain their values. */
    for(int p=0;p<2;++p)g_input_map.p[p].pad_type=PAD_6BUTTON;
    app_config_load(path);
    dyna_controllers_load(path);
}
static const char *snapshot_reason(void){return "Saving and rewind are not available in this development build.";}
const GameSpec g_game_spec={
    .display_name=DYNA_DISPLAY_NAME,.short_name=DYNA_NAME,
    .region_label="NTSC-J (Japan)",
#if RECOMP_LAUNCHER
    .runtime_menu_items=language_items,.runtime_menu_item_count=3,
    .runtime_menu_get=dyna_language_runtime_get,.runtime_menu_set=dyna_language_runtime_set,
#endif
    .controller_prepare=dyna_controller_prepare,.controller_mask=dyna_controller_mask,
    .controller_removed=dyna_controller_removed,
    .expected_rom_crc32=DYNA_CRC,.expected_rom_size=DYNA_SIZE,
    .call_entry_point=entry,.call_vblank=vint,.call_hblank=hint,
    .tier3_floor_default=0,
    .load_settings=settings,
    .mods=dyna_language_mods,
    .state_unavailable_reason=snapshot_reason,
#ifdef DYNA_HAS_JP
    .rom_aliases=db1_aliases,.num_rom_aliases=2,
#elif defined(DYNA_HAS_OVERLAY)
    .rom_aliases=&english_prompts_alias,.num_rom_aliases=1,
#endif
};
