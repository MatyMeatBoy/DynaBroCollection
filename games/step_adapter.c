#include "progress_runtime.h"
#include "visor_settings.h"
#include "extras_ui.h"
#include <SDL.h>
#include "menu_controls.h"
/* Explicit-PC static operations use the shared MC68000 AOT ABI. */
#include "genesis_runtime.h"
#include "glue.h"
#include "game_spec.h"
#include "native_interruptible.h"
#include "video/genesis_machine.h"
#include "m68k_step.h"
#include "app_config.h"
#include "input_map.h"
#include "gamepad.h"
#include "language_settings.h"
#include "db2_bio_overlay.h"
#include "db2_glyph_text.h"
#include "language_patch.h"
#include "controller_profiles.h"
#include "step_lookup.h"
#include "achievement_session.h"
#include "achievement_ui.h"
#include "achievement_smoke.h"
#include DYNA_LANGUAGE_HEADER
#if RECOMP_LAUNCHER
#include "recomp_runtime_ui.h"
static const char *const controller_choices[]={"Automatic","Custom SDL bindings","M30 Bluetooth / DirectInput","M30 Mini 2.4GHz receiver","M30 XInput"};
static const RecompRuntimeUiItem language_items[]={
    {"host.settings","Settings","Full Settings","Open the launcher settings in this game window.",RECOMP_RUNTIME_UI_ACTION,0,0,0},
    {"dyna.english","Language","English translation (restart)",
     "Partial translation. Changes apply after restarting the application; the running game keeps its current language.",
     RECOMP_RUNTIME_UI_BOOL,0,1,1},
    {"controls.p1_profile","Controls","Player 1 layout","Automatic detects known M30 devices. Select an M30 preset for shared/generic IDs; Custom uses your SDL bindings.",RECOMP_RUNTIME_UI_CHOICE,0,4,1,controller_choices,5},
    {"controls.p2_profile","Controls","Player 2 layout","Saved per connected device; does not overwrite another controller's bindings.",RECOMP_RUNTIME_UI_CHOICE,0,4,1,controller_choices,5},
    {"dyna.save","Game","Save Game","Save the progress code shown by the original game.",RECOMP_RUNTIME_UI_ACTION,0,0,0},
    {"dyna.extras","Extras","Extras","Level Selection and Game Guide.",RECOMP_RUNTIME_UI_ACTION,0,0,0},
    {"dyna.achievements","Achievements","Achievement list","Local progress, badges and stage goals. Opens a paused list.",RECOMP_RUNTIME_UI_ACTION,0,0,0}
};
#endif
#include DYNA_STEP_HEADER
#ifdef DYNA_HAS_JP
#include "db1_video.h"
#include "db1_title_menu.h"
#include "db1_original_title.h"
#include "db1_title_menu_smoke.h"
#include "db1_password_nav.h"
static Db1TitleMenu title_menu;
static Db1PasswordNav password_nav;
static Db1PasswordNavStatus navigation_seen;
static int native_title_entered,native_title_mature;
static void title_selection_update(void){
 unsigned selected;db1_video_set_title_menu_selection(db1_title_menu_selection(&title_menu,&selected)?(int)selected:-1);
}
#include "db1jp.h"
#include "translation_identity_db1.h"
static const GameRomAlias db1_aliases[]={
    {"Japanese original",0x360C1B20u},
    {"English text repairs (partial)",DYNA_DB1_ENGLISH_CRC},
    {"English v0.2 map-label repairs (partial)",0x3BE23E09u},
    {"English previous text repairs (partial)",0x950A8FC3u},
    {"English speech repairs (partial)",0x38B20F8Fu}
};
#endif
#ifdef DYNA_HAS_OVERLAY
#include "db2overlay.h"
#include "db2_video.h"
#include "translation_identity.h"
static const GameRomAlias english_prompts_alias={"English gameplay prompts (partial)",DYNA_DB2_ENGLISH_CRC};
static unsigned pad_trace_remaining;
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
extern const M68kStepEntry dyna_db2_ram_return;
static M68kStepBusResult read8(void *u,uint32_t a,uint8_t *v) { (void)u;*v=m68k_read8(a);return M68K_BUS_OK; }
static M68kStepBusResult read16(void *u,uint32_t a,uint16_t *v) { (void)u;*v=m68k_read16(a);return M68K_BUS_OK; }
static M68kStepBusResult fetch16(void *u,uint32_t a,uint16_t *v) { (void)u;*v=glue_peek16(a);return M68K_BUS_OK; }
static M68kStepBusResult write8(void *u,uint32_t a,uint8_t v) { (void)u;m68k_write8(a,v);return M68K_BUS_OK; }
static M68kStepBusResult write16(void *u,uint32_t a,uint16_t v) { (void)u;m68k_write16(a,v);return M68K_BUS_OK; }
static unsigned ack_interrupt(void *u,unsigned level) {
    (void)u;genesis_native_ack_interrupt(level);return M68K_STEP_AUTOVECTOR;
}
static const M68kStepBus bus={.read8=read8,.read16=read16,.write8=write8,.write16=write16,.fetch16=fetch16,.ack_interrupt=ack_interrupt};

#ifdef DYNA_HAS_JP
static void navigation_sync(void){
 Db1PasswordNavStatus state=db1_password_nav_status(&password_nav);
 if(state==navigation_seen)return;
 navigation_seen=state;
 if(state!=DB1_PASSWORD_IDLE){
  int cancel=state==DB1_PASSWORD_CANCELED || state==DB1_PASSWORD_TIMEOUT || state==DB1_PASSWORD_UNAVAILABLE;
  dyna_progress_runtime_navigation_status(db1_password_nav_status_text(&password_nav),cancel);
  fprintf(stderr,"[level-nav] state=%u native=%u owns=%d\n",(unsigned)state,
          password_nav.native_selection,db1_password_nav_owns(&password_nav));
 }
}
static void navigation_observe(uint32_t pc){
 if(!db1_password_nav_owns(&password_nav))return;
 switch(pc){
 case 0x1950:case 0x1a14:case 0x1e024:case 0x1d7b6:case 0x1c5a4:
 case 0x1c9ce:case 0x1caf6:case 0x1cb0c:case 0x1cbc4:case 0x1cbea:
 case 0x1cc16:case 0x1cc5c:case 0x1cc7c:case 0x1cc80:case 0x1cc92:case 0x1cc96:
 case 0x1cb30:case 0x1cb34:case 0x1cb46:case 0x1cb4a:case 0x1cb4e:case 0x1cb54:break;
 default:return;
 }
 unsigned confirm=0;
 for(unsigned i=0;i<3;i++)if(g_ram[0x28+i]==1){confirm=1u<<i;break;}
 db1_password_nav_before(&password_nav,pc,((unsigned)g_ram[0xfe]<<8)|g_ram[0xff],confirm);
 navigation_sync();
}
#endif
static int retire(void *user,unsigned *cycles) {
    M68kStepContext *ctx=user;
    dyna_achievement_session_before(g_cpu.PC);
#ifdef DYNA_HAS_JP
    navigation_observe(g_cpu.PC);
#endif
    dyna_progress_runtime_before(g_cpu.PC,g_ram,sizeof g_ram,g_cpu.D,g_cpu.A);
#ifdef DYNA_HAS_OVERLAY
    if(pad_trace_remaining && g_cpu.PC==0x4e0 &&
       (g_cpu.A[0]==0xa10003 || g_cpu.A[0]==0xa10005)){
        unsigned port=(g_cpu.A[0]-0xa10003)/2,low=g_cpu.D[0]&255;
        unsigned id=((g_cpu.D[2]&255)+((low&12)?1:0))*2+((low&3)?1:0);
        if(id!=13){
            fprintf(stderr,"[pad-burst] invalid id=%u port=%u low=%02X count=%u buttons=%04X pad6=%u audio_cycles=%u master=%u\n",
                    id,port,low,g_machine.bus.pad_th_count[port],g_machine.bus.pad[port],g_machine.bus.pad_type[port],g_audio_cycle_counter,g_machine.master_cycle);
            --pad_trace_remaining;
        }
    }
    if(g_cpu.PC==0x3bb0 || g_cpu.PC==0x3bb4 || g_cpu.PC==0x734e ||
       g_cpu.PC==0xf794 || g_cpu.PC==0xf728)db2_video_observe(g_cpu.PC);
    if(g_cpu.PC==0x5b278){static int trace2=-1;if(trace2<0){const char *t=getenv("DYNA_TRACE_GLYPH");trace2=t&&t[0]=='1';}
        if(trace2){unsigned a1=g_cpu.A[1]&0xffffffu,w=(((unsigned)g_ram[(g_cpu.A[6]+12)&0xffff]<<8)|g_ram[(g_cpu.A[6]+13)&0xffff])>>2;
            unsigned word=a1<0x400000u?(((unsigned)g_rom[a1]<<8)|g_rom[a1+1]):(((unsigned)g_ram[a1&0xffff]<<8)|g_ram[(a1+1)&0xffff]);
            fprintf(stderr,"[glyph2] a1=%06X word=%04X width=%u a0=%08X\n",a1,word,w,g_cpu.A[0]);}}
    if(g_cpu.PC==0x5b174){static int trace=-1;if(trace<0){const char *t=getenv("DYNA_TRACE_GLYPH");trace=t&&t[0]=='1';}
        if(trace){unsigned a1=g_cpu.A[1]&0xffffffu,w=(((unsigned)g_ram[(g_cpu.A[6]+12)&0xffff]<<8)|g_ram[(g_cpu.A[6]+13)&0xffff])>>2;
            fprintf(stderr,"[glyph] a1=%06X word=%04X width=%u a0=%08X\n",a1,a1<0x400000u?(((unsigned)g_rom[a1]<<8)|g_rom[a1+1]):(((unsigned)g_ram[a1&0xffff]<<8)|g_ram[(a1+1)&0xffff]),w,g_cpu.A[0]);}}
    if(g_cpu.PC==0x5b174 && dyna_language_english())db2_glyph_text_observe(g_cpu.PC);
    if(dyna_language_english() && (g_cpu.PC==0x5d3d4 || g_cpu.PC==0x5d57a ||
       g_cpu.PC==0x5d674 || g_cpu.PC==0x5d722))db2_bio_overlay_observe(g_cpu.PC);
#endif
#ifdef DYNA_HAS_JP
    switch(g_cpu.PC){
    case 0x1950:case 0x1a14:case 0x1e024:case 0x1d7b6:case 0x1c5a4:
    case 0x1c9ce:case 0x1caf6:case 0x1cb0c:case 0x1cbc4:case 0x1cbea:
    case 0x1cc16:case 0x1cc5c:case 0x1cc7c:case 0x1cc92:
    case 0x1cb30:case 0x1cb34:case 0x1cb46:case 0x1cb4a:case 0x1cb4e:case 0x1cb54:
        if(g_cpu.PC>=0x1cb30 && g_cpu.PC<=0x1cb54){
            if(g_cpu.PC==0x1cb30)native_title_entered=1;
            if(native_title_entered && g_cpu.PC!=0x1cb30 && g_cpu.PC!=0x1cb34)native_title_mature=1;
        }else native_title_entered=native_title_mature=0;
        db1_title_menu_before(&title_menu,g_cpu.PC,((unsigned)g_ram[0xfe]<<8)|g_ram[0xff]);
        title_selection_update();break;
    default:break;
    }
    switch(g_cpu.PC){
    case 0x1c40:case 0x1c44:case 0x19980:case 0x1b040:
    case 0x1c5a4:case 0x1c7d0:case 0x1c9ce:case 0x1caf6:case 0x1cb0c:
    case 0x1e024:case 0x1d7b6:case 0x1b42:case 0x1b38:case 0x1c04:case 0x1950:case 0x1bc04:
    case 0x5ff6:case 0x62fc:case 0x132d8:case 0x13630:
    case 0x12fe6:case 0x13286:case 0x14070:case 0x1429e:case 0x1429c:case 0x144f6:
    case 0x15d2c:case 0x15e52:case 0x5650:case 0x5d62:
        db1_video_observe(g_cpu.PC);break;
    default:break;
    }
#endif
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
    if(ctx==&main_cpu) {
        unsigned level=genesis_native_pending_interrupt();
        if(level) {
            uint32_t pc=ctx->state.regs.PC;
            uint16_t sr=ctx->state.regs.SR;
            if(!m68k_step_interrupt(ctx,level)) {
                fprintf(stderr,"[AOT-IRQ] INVALID core entry level=%u fault=%u\n",level,ctx->state.fault);
                return 0;
            }
            if(getenv("DYNA_IRQ_VERIFY")) {
                static unsigned checked;
                uint32_t sp=ctx->state.regs.A[7];
                if(ctx->state.cycles-before!=44 || glue_peek16(sp)!=sr ||
                   glue_peek32(sp+2u)!=(pc&0xffffffu) ||
                   ((ctx->state.regs.SR>>8)&7u)!=level) {
                    fprintf(stderr,"[AOT-IRQ] INVALID instruction-boundary frame/cycles level=%u\n",level);
                    return 0;
                }
                if(!(checked&(1u<<level))) {
                    checked|=1u<<level;
                    fprintf(stderr,"[AOT-IRQ] PASS level=%u real exception frame, 44 entry cycles, continuous native execution\n",level);
                }
            }
            memcpy(&g_cpu,&ctx->state.regs,sizeof(g_cpu));
        }
    }
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
    return 1;
}
static void entry(void) {
    dyna_achievement_session_reset();
#ifdef DYNA_HAS_JP
    db1_video_reset();
    db1_title_menu_smoke_init();
    db1_password_nav_reset(&password_nav);navigation_seen=DB1_PASSWORD_IDLE;
    native_title_entered=native_title_mature=0;
    db1_title_menu_reset(&title_menu);
    db1_title_menu_enable(&title_menu,db1_video_enhanced());
#elif defined(DYNA_HAS_OVERLAY)
    db2_video_reset();
    const char *pad_trace=getenv("DYNA_PAD_TRACE");
    pad_trace_remaining=pad_trace&&!strcmp(pad_trace,"1")?32:0;
#endif
    const char *lookup=getenv("DYNA_AOT_LOOKUP");
    cached_lookup=!lookup||strcmp(lookup,"binary");
    dyna_step_cache_clear();
    char error[160];
    if(!dyna_language_apply(g_rom,DYNA_SIZE,&DYNA_LANGUAGE_IMAGE,dyna_language_english(),error,sizeof error)) {
        fprintf(stderr,"[language] %s\n",error);abort();
    }
#ifdef DYNA_HAS_JP
    db1_video_set_english(dyna_language_english());
#endif
#ifdef DYNA_HAS_JP
    dyna_progress_runtime_db1_identities(db1_language_sha256,sizeof db1_language_sha256/sizeof db1_language_sha256[0]);
#endif
    dyna_progress_runtime_start(DYNA_ENTRY==0x738u?2:1,g_rom,DYNA_SIZE);
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
    fprintf(stderr,"[AOT] IRQ instruction guard level=%u PC=$%06X opcode=$%04X SP=$%06X frame_sp=$%06X SR=$%04X frame_sr=$%04X frame_pc=$%06X\n",
        level,g_cpu.PC,glue_peek16(g_cpu.PC),g_cpu.A[7],sp,g_cpu.SR,glue_peek16(sp),glue_peek32(sp+2u));abort();
}
static void vint(void){interrupt(DYNA_VINT,6);}
static void hint(void){interrupt(DYNA_HINT,4);}
static void settings(const char *path){
    dyna_language_settings_load(path);
#ifdef DYNA_HAS_JP
    dyna_visor_settings_bind(db1_video_set_visor_offset_tiles);
#elif defined(DYNA_HAS_OVERLAY)
    dyna_visor_settings_bind(db2_video_visor_offset);
#else
    dyna_visor_settings_bind(NULL);
#endif
    dyna_visor_settings_load(path,DYNA_ENTRY==0x738u?2:1);
    dyna_progress_runtime_settings(path);
    dyna_achievement_session_init(path,DYNA_ENTRY==0x738u);
    {
        char assets[2048];char *base=SDL_GetBasePath();
        snprintf(assets,sizeof assets,"%sassets/guide",base?base:"");SDL_free(base);
        dyna_extras_ui_init(DYNA_ENTRY==0x738u?2:1,assets);
    }
    dyna_controller_defaults();
    g_app_config.integer_scale=1;
    /* Use the common parser again after installing game defaults. Explicit
     * saved 3-button choices and unrelated settings retain their values. */
    for(int p=0;p<2;++p)g_input_map.p[p].pad_type=PAD_6BUTTON;
    app_config_load(path);
    dyna_controllers_load(path);
}
static uint16_t game_input_filter(unsigned player,uint16_t mask){
#ifdef DYNA_HAS_JP
 if(!player){
  if(dyna_progress_runtime_take_navigation_request()){
   int started=db1_password_nav_request(&password_nav,native_title_mature,((unsigned)g_ram[0xfe]<<8)|g_ram[0xff]);
   navigation_sync();
   if(!started){
    dyna_progress_runtime_navigation_status(db1_password_nav_status_text(&password_nav),1);
    dyna_extras_ui_open();return 0;
   }
  }
  if(db1_password_nav_owns(&password_nav)){
   unsigned input=0,pad=0;int replace=0;
   if(mask&GPAD_A)input|=DB1_PASSWORD_A;
   if(mask&GPAD_B)input|=DB1_PASSWORD_B;
   if(mask&GPAD_C)input|=DB1_PASSWORD_C;
   if(mask&GPAD_UP)input|=DB1_PASSWORD_UP;
   if(mask&GPAD_DOWN)input|=DB1_PASSWORD_DOWN;
   if(mask&~(GPAD_A|GPAD_B|GPAD_C|GPAD_UP|GPAD_DOWN))input|=DB1_PASSWORD_OTHER;
   db1_title_menu_enable(&title_menu,0);title_selection_update();
   db1_password_nav_frame(&password_nav,input,&replace,&pad);navigation_sync();
   if(!db1_password_nav_owns(&password_nav)){
    db1_title_menu_resume_mature(&title_menu,db1_video_enhanced(),native_title_mature,!input,
                                ((unsigned)g_ram[0xfe]<<8)|g_ram[0xff]);
    title_selection_update();
   }
   /* Exclusive navigation skips replay_frame and virtual Extras until handoff. */
   if(replace)return (uint16_t)((pad&DB1_PASSWORD_A?GPAD_A:0)|(pad&DB1_PASSWORD_B?GPAD_B:0)|
       (pad&DB1_PASSWORD_C?GPAD_C:0)|(pad&DB1_PASSWORD_UP?GPAD_UP:0)|(pad&DB1_PASSWORD_DOWN?GPAD_DOWN:0));
  }
 }
#endif
 mask=dyna_progress_runtime_input(player,mask);
#ifdef DYNA_HAS_JP
 if(!player){
  db1_title_menu_smoke_frame();
  db1_title_menu_enable_mature(&title_menu,db1_video_enhanced(),native_title_mature,
                               ((unsigned)g_ram[0xfe]<<8)|g_ram[0xff]);
  if(title_menu.modal && !dyna_extras_ui_is_open())db1_title_menu_host_closed(&title_menu);
  unsigned input=0,pad=0;int replace=0;
  if(mask&GPAD_UP)input|=DB1_TITLE_UP;
  if(mask&GPAD_DOWN)input|=DB1_TITLE_DOWN;
  if(mask&(GPAD_A|GPAD_B|GPAD_C))input|=DB1_TITLE_CONFIRM;
  if(mask&GPAD_START)input|=DB1_TITLE_START;
  if(mask&~(GPAD_UP|GPAD_DOWN|GPAD_A|GPAD_B|GPAD_C|GPAD_START))input|=DB1_TITLE_OTHER;
  if(db1_title_menu_frame(&title_menu,input,&replace,&pad)){
   dyna_extras_ui_open();db1_title_menu_smoke_opened();
  }
  title_selection_update();
  if(replace)return (uint16_t)((pad&DB1_TITLE_UP?GPAD_UP:0)|(pad&DB1_TITLE_DOWN?GPAD_DOWN:0));
 }
#endif
 return mask;
}
static int game_runtime_get(const char *key,int *value){
 return dyna_visor_runtime_get(key,value)||dyna_language_runtime_get(key,value);
}
static int game_runtime_set(const char *key,int value){
 return dyna_visor_runtime_set(key,value)||dyna_language_runtime_set(key,value);
}
static const struct RecompLauncherCModProvider *game_mods(const struct RecompLauncherCModProvider *base){
 return dyna_visor_mods(dyna_language_mods(base));
}
static const char *snapshot_reason(void){return "Mid-battle save states and rewind are unavailable. Save Game stores native progress codes.";}
static int game_ui_open(void){return dyna_extras_ui_is_open() || dyna_achievement_ui_is_open();}
static int game_ui_active(void){
 int active=dyna_extras_ui_is_open() || dyna_achievement_ui_active() || dyna_progress_runtime_prompt(NULL,NULL);
#ifdef DYNA_HAS_JP
 active=active || db1_original_title_active(&g_machine.vdp,db1_video_english(),db1_video_enhanced());
#endif
 return active;
}
static void game_ui_input(int action,int down,int repeat) {
 if(dyna_extras_ui_is_open())dyna_extras_ui_input(action,down,repeat);
 else dyna_achievement_ui_input(action,down,repeat);
}
static void game_ui_render(uint32_t *pixels,int w,int h,int pitch) {
#ifdef DYNA_HAS_JP
 db1_original_title_render(&g_machine.vdp,pixels,w,h,pitch,db1_video_english(),db1_video_enhanced());
#endif
 unsigned selected;const char *feedback;
 if(dyna_progress_runtime_prompt(&selected,&feedback)){
  if(dyna_progress_runtime_prompt_kind()==2)dyna_extras_ui_ending_save_prompt(pixels,w,h,pitch,feedback);
  else dyna_extras_ui_progress_prompt(pixels,w,h,pitch,selected,feedback);
 }
 dyna_achievement_smoke_render(pixels,w,h,pitch);
 dyna_extras_ui_render(pixels,w,h,pitch);
#ifdef DYNA_HAS_JP
 db1_title_menu_smoke_render(pixels,w,h,pitch);
#endif
}
static int game_ui_action(const char *key) {
 if(key && !strcmp(key,"dyna.extras")){dyna_extras_ui_open();return 1;}
 if(dyna_progress_runtime_action(key))return 1;
 return dyna_achievement_session_action(key);
}
/* The engine maps only arrows/Return/Space/Escape to host-menu actions. Add the
 * keyboard keys bound to pad A/C (accept) and B (back) so the Extras menu can
 * be driven with the same keys as the game (default Z/C and X). */
static uint32_t keyboard_menu_bits(void){
 int count=0;const Uint8 *keys=SDL_GetKeyboardState(&count);
 const PlayerInput *p=&g_input_map.p[0];
 uint32_t bits=0;
 if(!keys||!(p->device&INPUT_DEV_KEYBOARD))return 0;
#define HELD(b) (p->key[b]>0&&p->key[b]<count&&keys[p->key[b]])
 if(HELD(GB_A)||HELD(GB_C))bits|=GP_OVERLAY_A;
 if(HELD(GB_B))bits|=GP_OVERLAY_B;
#undef HELD
 return bits;
}
static int menu_tick(uint32_t mask,int open,uint64_t now,int *blocked) {
 static DynaMenuControls state;
 int action=dyna_menu_controls_tick(&state,mask|keyboard_menu_bits(),open,now);
 *blocked=dyna_menu_controls_blocks(&state,open);return action;
}
const GameSpec g_game_spec={
    .menu_input_tick=menu_tick,
    .input_filter=game_input_filter,
    .pacing_default="accurate",.frame_rate_numerator=53693175,.frame_rate_denominator=3420*262,
    .native_aspect_width=4,.native_aspect_height=3,
    .display_name=DYNA_DISPLAY_NAME,.short_name=DYNA_NAME,
    .region_label="NTSC-J (Japan)",
#if RECOMP_LAUNCHER
    .runtime_menu_items=language_items,.runtime_menu_item_count=7,
    .runtime_menu_get=game_runtime_get,.runtime_menu_set=game_runtime_set,
    .runtime_menu_action=game_ui_action,
#endif
    .host_ui_open=game_ui_open,.host_ui_active=game_ui_active,
    .host_ui_input=game_ui_input,.host_ui_render=game_ui_render,
    .on_diagnostic_session=dyna_achievement_session_diagnostic,
    .on_frame_post=dyna_achievement_session_frame,
    .controller_prepare=dyna_controller_prepare,.controller_mask=dyna_controller_mask,
    .controller_removed=dyna_controller_removed,
    .expected_rom_crc32=DYNA_CRC,.expected_rom_size=DYNA_SIZE,
    .call_entry_point=entry,.call_vblank=vint,.call_hblank=hint,
    .tier3_floor_default=0,
    .load_settings=settings,
    .mods=game_mods,
    .state_unavailable_reason=snapshot_reason,
#ifdef DYNA_HAS_JP
    .video=&db1_video,.video_default="16:9",
    .rom_aliases=db1_aliases,.num_rom_aliases=5,
#elif defined(DYNA_HAS_OVERLAY)
    .video=&db2_video,.video_default="16:9",
    .rom_aliases=&english_prompts_alias,.num_rom_aliases=1,
#endif
};
