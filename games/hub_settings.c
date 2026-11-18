/* Same game settings/model used by the green hub and native executable. */
#include "hub_settings.h"
#include "visor_settings.h"
#include "hub_mod_inventory.h"
#include "app_config.h"
#include "input_map.h"
#include "gamepad.h"
#include "game_spec.h"
#include "controller_profiles.h"
#include "language_settings.h"
#include "recomp_launcher.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>
/* The hub has no guest or diagnostic replay. These callbacks configure devices. */
int genesis_replay_playing(void){return 0;}
const GameSpec g_game_spec={.controller_prepare=dyna_controller_prepare,
 .controller_mask=dyna_controller_mask,.controller_removed=dyna_controller_removed};
static int video_configure(const char *s){return s!=NULL;}
static int video_enabled(void){return g_app_config.custom_widescreen;}
static int video_width(int a,int b,int c,int d){(void)a;(void)b;(void)c;(void)d;return 398;}
static const GameVideo video={.configure=video_configure,.enabled=video_enabled,.width=video_width};
static int bindings(int p,int b,int *key,int *kind,int *code,int *dir){
 if(p<0||p>=INPUT_MAX_PLAYERS||b<0||b>=GB_COUNT||!key||!kind||!code||!dir)return 0;
 *key=g_input_map.p[p].key[b];*kind=g_input_map.p[p].pad[b].kind;
 *code=g_input_map.p[p].pad[b].code;*dir=g_input_map.p[p].pad[b].axis_dir;return 1;
}
int dyna_hub_settings_run(SDL_Window *window,SDL_Renderer *renderer,unsigned game,
 const char *config,const char *cache,const char *initial,char *picked,size_t capacity,const ld_inventory *inventory){
 if(!window||!renderer||!config||!cache||!picked||capacity<2||(game!=1&&game!=2))return 2;
 app_config_set_video_default("16:9");app_config_defaults();input_map_init_defaults();dyna_controller_defaults();
 g_app_config.integer_scale=1;app_config_load(config);dyna_controllers_load(config);dyna_language_settings_load(config);
 dyna_visor_settings_bind(NULL);dyna_visor_settings_load(config,game);
 gamepad_init_players(2);
 RecompLauncherCSettings s={0};
 s.output_method=0;s.window_scale=g_app_config.window_scale;s.fullscreen=g_app_config.fullscreen;
 s.linear_filter=g_app_config.linear_filter;s.integer_scale=g_app_config.integer_scale;
 s.widescreen=g_app_config.widescreen;s.widescreen_cells=g_app_config.widescreen_cells;
 s.enable_audio=1;s.audio_freq=g_app_config.audio_freq;s.volume=g_app_config.volume;s.skip_launcher=1;
 for(int p=0;p<2;p++){
  int device=g_input_map.p[p].device;
  s.player_src[p]=device==3?3:device==1?1:device==2?2:0;
  s.pad_mode[p]=g_input_map.p[p].pad_type==PAD_6BUTTON;s.deadzone[p]=g_input_map.p[p].deadzone_pct;
 }
 RecompLauncherCGameInfo gi={0};
 gi.name=game==1?"Dyna Brothers":"Dyna Brothers 2 Special";
 gi.initial_page="settings";
 gi.platform="SEGA GENESIS";gi.theme="dyna";gi.region="NTSC-J";gi.num_players=2;
 gi.expected_crc=game==1?0x360c1b20u:0xba3554bcu;gi.has_expected_crc=1;
 RecompLauncherCRomRevision revisions[6]={0};
 const char *crc1[]={"360C1B20","2740C64E","C1E5C0DA","3BE23E09","950A8FC3","38B20F8F"};
 const char *crc2[]={"BA3554BC","DAC9BAFC","07FEA88F"};
 gi.num_rom_revisions=game==1?6:3;
 for(int i=0;i<gi.num_rom_revisions;i++){revisions[i].label=i?"English translation":"Japanese original";revisions[i].crc32_hex=game==1?crc1[i]:crc2[i];revisions[i].supported=1;}
 gi.rom_revisions=revisions;gi.rom_noun="ROM";
 static const char *const patterns[]={"*.md","*.bin","*.gen","*.68k"};gi.rom_patterns=patterns;gi.num_rom_patterns=4;
 gi.rom_filter_desc="Dyna Brothers ROM";gi.config_path=config;gi.keybinds_path=config;gi.rom_cache_path=cache;
 gi.pad_mode_supported=gi.pad_mode_selectable=1;gi.has_integer_scale=1;
 gi.input_binding_snapshot=bindings;
 gi.host_sdl_window=window;gi.host_sdl_renderer=renderer;
 char brand[1400],art[1400],assets[1400];char *base=SDL_GetBasePath();
 snprintf(brand,sizeof brand,"%sassets/img/dyna-logo-1plus2-v1.png",base?base:"");
 snprintf(art,sizeof art,"%sassets/img/db%u-gameplay.png",base?base:"",game);
 snprintf(assets,sizeof assets,"%sassets",base?base:"");SDL_free(base);
 gi.brand_logo_path=brand;gi.boxart_path=art;
 gi.mods=dyna_hub_mod_inventory(dyna_visor_mods(dyna_language_mods(app_config_video_mods(&video,config))),inventory);
 int result=recomp_launcher_run_window(gi.name,&s,&gi,assets,initial,picked,capacity);
 if(result==0||result==1){
  app_config_reload_preserving_mods(config);
  g_app_config.window_scale=s.window_scale;g_app_config.fullscreen=s.fullscreen;
  g_app_config.linear_filter=s.linear_filter;g_app_config.integer_scale=s.integer_scale;
  g_app_config.volume=s.volume;g_app_config.audio_freq=s.audio_freq;g_app_config.skip_launcher=1;
  for(int p=0;p<2;p++){
   g_input_map.p[p].device=s.player_src[p]==3?3:s.player_src[p]==1?1:s.player_src[p]==2?2:0;
   g_input_map.p[p].pad_type=s.pad_mode[p]?PAD_6BUTTON:PAD_3BUTTON;
   g_input_map.p[p].deadzone_pct=s.deadzone[p];
  }
  if(!app_config_save(config)){fprintf(stderr,"[hub] Could not save settings\n");result=2;}
 }
 gamepad_shutdown();return result;
}

void dyna_hub_controls_start(const char *config){
 app_config_set_video_default("16:9");app_config_defaults();input_map_init_defaults();dyna_controller_defaults();
 if(config)app_config_load(config);dyna_controllers_load(config);gamepad_init_players(2);
}
uint32_t dyna_hub_controls_poll(void){
 SDL_Event event;SDL_PumpEvents();while(SDL_PollEvent(&event))gamepad_handle_event(&event);
 (void)gamepad_player_mask(0);return gamepad_menu_mask();
}
