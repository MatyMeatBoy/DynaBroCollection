/* Host persistence bridge. Guest code capture remains immutable. */
#include "progress_runtime.h"
#include "achievement_session.h"
#include "achievements.h"
#include "db1_progress_migration.h"
#include "progress_session.h"
#include "db1_level_catalog.h"
#include "progress_replay.h"
#include "progress_ui.h"
#include "video/genesis_bus.h"
#include "extras_ui.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#endif
static DynaProgressStore store;
static DynaProgressSession session;
static DynaProgressReplay replay;
static DynaProgressUi prompt;
static char folder[1800],path[2048],status[128];
static int ready;
static const char *const *db1_hashes;
static size_t db1_hash_count;
static int navigation_requested;
/* Dormant unless explicitly requested by a native qualification run. */
static int test_index=-1,test_queued,test_queue_title,trace_progress;
static unsigned test_cancel_after;
static int save_test_mode,save_test_pending,save_test_done;
static void save_test_frame(void);
static void test_setup(unsigned game) {
 const char *trace=getenv("DYNA_PROGRESS_TRACE");trace_progress=trace&&!strcmp(trace,"1");
 const char *v=getenv("DYNA_DB1_REPLAY_TEST_INDEX");char *end;
 const char *save=getenv("DYNA_DB1_SAVE_TEST");
 save_test_mode=0;save_test_pending=save_test_done=0;
 if(game==1 && save){
  if(!strcmp(save,"save"))save_test_mode=1;
  else if(!strcmp(save,"reload"))save_test_mode=2;
  else if(!strcmp(save,"empty"))save_test_mode=3;
 }
 test_index=-1;test_queued=0;test_cancel_after=0;test_queue_title=0;
 if(game!=1 || !v || !*v || *v<'0' || *v>'9')return;
 unsigned long i=strtoul(v,&end,10);
 if(*end || i>96 || (i>=8 && i<20))return;
 test_index=(int)i;
 v=getenv("DYNA_DB1_REPLAY_TEST_QUEUE_TITLE");
 test_queue_title=v && !strcmp(v,"1");
 v=getenv("DYNA_DB1_REPLAY_TEST_CANCEL_AFTER");
 if(v && *v>='0' && *v<='9'){
  unsigned long n=strtoul(v,&end,10);
  if(!*end && n<=1800)test_cancel_after=(unsigned)n;
 }
}
static void test_log(uint32_t pc) {
 fprintf(stderr,"[progress-test] pc=%06X status=%u elapsed=%u pulse=%u\n",
  pc,(unsigned)replay.status,replay.elapsed,replay.pulse_frame);
}
void dyna_progress_runtime_db1_identities(const char *const *hashes,size_t count){
 db1_hashes=hashes;db1_hash_count=count;
}
void dyna_progress_runtime_settings(const char *settings) {
 ready=0;folder[0]=path[0]=0;
 if(!settings || strlen(settings)>=sizeof folder)return;
 strcpy(folder,settings);char *a=strrchr(folder,'/'),*b=strrchr(folder,'\\');
 char *sep=!a?b:!b?a:a>b?a:b;
 if(sep)sep[1]=0;else strcpy(folder,"./");
}
static int identity(const uint8_t *rom,size_t size,char text[65]) {
#ifdef _WIN32
 BCRYPT_ALG_HANDLE alg=NULL;BCRYPT_HASH_HANDLE hash=NULL;
 unsigned char digest[32];int ok=0;
 if(size>0xffffffffu)return 0;
 if(BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,NULL,0)<0)return 0;
 if(BCryptCreateHash(alg,&hash,NULL,0,NULL,0,0)>=0 &&
    BCryptHashData(hash,(PUCHAR)rom,(ULONG)size,0)>=0 &&
    BCryptFinishHash(hash,digest,sizeof digest,0)>=0) {
  static const char digits[]="0123456789abcdef";
  for(unsigned i=0;i<32;i++){text[2*i]=digits[digest[i]>>4];text[2*i+1]=digits[digest[i]&15];}
  text[64]=0;ok=1;
 }
 if(hash)BCryptDestroyHash(hash);
 BCryptCloseAlgorithmProvider(alg,0);return ok;
#else
 (void)rom;(void)size;(void)text;return 0;
#endif
}
static void refresh_db1_clears(void) {
 static const uint32_t final_stage_ids[]={183310,183264,183282,183302};
 session.db1_cleared_modes=0;
 for(size_t i=0;i<dyna_achievements_count();i++){
  const DynaAchievementItem *achievement=dyna_achievements_item(i);
  if(!achievement || !achievement->unlocked_at)continue;
  for(unsigned mode=0;mode<sizeof final_stage_ids/sizeof final_stage_ids[0];mode++)
   if(achievement->id==final_stage_ids[mode])session.db1_cleared_modes|=1u<<mode;
 }
}
static size_t count(void *u,unsigned game){
 (void)u;
 if(!ready || game!=store.game)return 0;
 if(game!=1)return dyna_progress_session_count(&session);
 refresh_db1_clears();
 return db1_level_catalog_count(&session);
}
static int item(void *u,unsigned game,size_t i,DynaExtrasLevel *out) {
 (void)u;static char title[80],detail[120];
 if(!out || !ready || game!=store.game || i>255)return 0;
 if(game==1){
  refresh_db1_clears();
  Db1LevelCatalogItem entry;
  if(!db1_level_catalog_item(&session,(unsigned)i,&entry))return 0;
  snprintf(title,sizeof title,"%s",entry.label);
  snprintf(detail,sizeof detail,"%s",entry.has_saved_code?"SAVED PROGRESS":"AVAILABLE TO REPLAY");
  out->title=title;out->detail=detail;out->available=1;return 1;
 }
 const DynaProgressCode *c=dyna_progress_session_item(&session,(unsigned)i);char code[9];
 if(!c || !dyna_progress_session_text(&session,c,code))return 0;
 snprintf(title,sizeof title,"MODE %u / STAGE %u",c->mode,c->stage);
 snprintf(detail,sizeof detail,"PASSWORD %s - ENTER ON PASSWORD SCREEN",code);
 out->title=title;out->detail=detail;out->available=1;return 1;
}
static int launch(void *u,unsigned game,size_t i) {
 (void)u;if(!ready || game!=store.game || i>255)return 0;
 if(game==1){
  refresh_db1_clears();
  Db1LevelCatalogItem entry;DynaProgressCode generated;
  if(!db1_level_catalog_item(&session,(unsigned)i,&entry) ||
     !db1_level_catalog_request_code(&session,entry.mode,entry.stage,&generated) ||
     !dyna_progress_replay_request(&replay,&generated))return 0;
  snprintf(status,sizeof status,"LOADING SELECTED LEVEL");
  navigation_requested=1;dyna_extras_ui_notice(status);return 1;
 }
 const DynaProgressCode *code=dyna_progress_session_item(&session,(unsigned)i);
 if(!code || !dyna_progress_replay_request(&replay,code))return 0;
 dyna_extras_ui_notice(dyna_progress_replay_status_text(&replay));
 return 0; /* Keep the guidance visible until the user closes Extras. */
}
int dyna_progress_runtime_start(unsigned game,const uint8_t *rom,size_t size) {
 dyna_progress_ui_reset(&prompt);
 navigation_requested=0;
 test_setup(game);
 char hash[65];ready=0;snprintf(status,sizeof status,"SAVE UNAVAILABLE");
 if(!folder[0] || !rom || !identity(rom,size,hash))return 0;
 const char *profile_hash=hash;
 if(game==1 && db1_hashes){
  int known=0;
  if(!db1_hash_count || db1_hash_count>DB1_PROGRESS_MIGRATION_MAX_HASHES)return 0;
  for(size_t i=0;i<db1_hash_count;i++)if(db1_hashes[i]&&!strcmp(hash,db1_hashes[i]))known=1;
  if(!known){snprintf(status,sizeof status,"SAVE IMAGE IDENTITY UNAVAILABLE");return 0;}
  profile_hash=db1_hashes[0];
 }
 if(!dyna_progress_init(&store,game,profile_hash))return 0;
 if(snprintf(path,sizeof path,"%sdyna%u-progress-%.16s.bin",folder,game,profile_hash)>=(int)sizeof path)return 0;
 if(game==1 && db1_hashes){
  Db1ProgressMigrationReport migration;
  if(!db1_progress_migration_run(&store,folder,profile_hash,db1_hashes,db1_hash_count,rom,size,&migration)){
   fprintf(stderr,"[progress-migration] FAIL status=%u; prior files preserved\n",(unsigned)migration.status);
   snprintf(status,sizeof status,"SAVED PROGRESS COULD NOT BE LOADED");path[0]=0;return 0;
  }
  if(migration.written || migration.invalid_codes || migration.corrupt_legacy)
   fprintf(stderr,"[progress-migration] imported=%u invalid=%u corrupt_legacy=%u written=%u\n",
           migration.imported,migration.invalid_codes,migration.corrupt_legacy,migration.written);
 }else if(dyna_progress_file_exists(path) && !dyna_progress_load(&store,path)){
  snprintf(status,sizeof status,"SAVE FILE COULD NOT BE READ");path[0]=0;return 0;
 }
 ready=dyna_progress_session_init(&session,&store,rom,size);
 dyna_progress_replay_init(&replay,game,rom,size);
 DynaExtrasLevelProvider provider={NULL,count,item,launch};dyna_extras_ui_set_level_provider(&provider);
 snprintf(status,sizeof status,"SAVE AT A PASSWORD SCREEN");return ready;
}
void dyna_progress_runtime_before(uint32_t pc,uint8_t *ram,size_t size,uint32_t d[8],const uint32_t a[8]) {
 switch(pc) {
 case 0x1950:case 0x1a14:case 0x1c5a4:case 0x1d7b6:case 0x1becc:case 0x1dd90:case 0x28f1a:case 0x28f2e:
 case 0x3a38:case 0x6f9a:case 0x4913e:case 0x58ef2:
 case 0x1db7e:case 0x1db88:case 0x1dcec:case 0x1dd1a:case 0x1dce4:
 case 0x1bd6c:case 0x1bd7c:case 0x1c9ce:case 0x1caf6:case 0x1cb0c:
 case 0x49708:case 0x49908:case 0x49926:break;
 default:return;
 }
 if(ready){
  DynaProgressReplayStatus before=replay.status;
  if(test_index>=0 && !test_queued && pc==(test_queue_title?0x1c5a4u:0x1d7b6u)){
   unsigned i=(unsigned)test_index;size_t offset=0x1e87e + i*4;
   DynaProgressCode c={0};c.mode=(uint16_t)(i>=80?4:i/20);
   c.stage=(uint16_t)(i>=80?i-79:i<8?i+1:i%20+1);c.length=4;
   test_queued=1;
   if(offset+4<=replay.rom_size){
    memcpy(c.code,replay.rom+offset,4);dyna_progress_replay_request(&replay,&c);
   }
   test_log(pc);
  }
  dyna_progress_session_before(&session,pc,ram,size,a);dyna_progress_replay_before(&replay,pc,ram,size,d,a);
  if(store.game==1){
   const DynaProgressCode *c=dyna_progress_session_current(&session);
   uint32_t expected=pc==0x28f1a?0x28f1au:0x1beccu;
   dyna_progress_ui_before(&prompt,pc,c && c->source_pc==expected);
  }
  if(save_test_mode==1 && !save_test_done && pc==0x1becc &&
     dyna_progress_session_current(&session))save_test_pending=1;
  if(replay.status!=before){
   if(replay.status==DYNA_REPLAY_LOADED)dyna_achievement_session_save_loaded();
   dyna_extras_ui_notice(dyna_progress_replay_status_text(&replay));
   if((test_index>=0 || trace_progress) && size>=0x3a){
    test_log(pc);
    fprintf(stderr,"[progress-test] stage=%02X%02X ptr=%02X%02X%02X%02X d4=%08X code=%02X%02X%02X%02X\n",
     ram[0x34],ram[0x35],ram[0x2e],ram[0x2f],ram[0x30],ram[0x31],d[4],ram[0x36],ram[0x37],ram[0x38],ram[0x39]);
   }
  }
 }
}
static int save_current(void) {
 int ok=ready && dyna_progress_session_current(&session) &&
  dyna_progress_session_save(&session,path);
 snprintf(status,sizeof status,"%s",ok?"PROGRESS SAVED":"SAVE FAILED");
 fprintf(stderr,"[progress] %s\n",status);return ok;
}
int dyna_progress_runtime_action(const char *key) {
 if(!key || strcmp(key,"dyna.save"))return 0;
 if(!ready || !dyna_progress_session_current(&session)){snprintf(status,sizeof status,"SAVE AT A PASSWORD SCREEN");dyna_extras_ui_notice(status);dyna_extras_ui_open();return 1;}
 int ok=save_current();
 if(!ok)snprintf(status,sizeof status,"SAVE FAILED - PREVIOUS SAVE KEPT");
 dyna_extras_ui_notice(status);dyna_extras_ui_open();
 return 1;
}
const char *dyna_progress_runtime_status(void){return status;}
int dyna_progress_runtime_prompt(unsigned *selected,const char **feedback) {
 return dyna_progress_ui_prompt(&prompt,selected,feedback);
}
int dyna_progress_runtime_prompt_kind(void){return dyna_progress_ui_kind(&prompt);}

/* Invoked only at the host frame input boundary, never from guest retirement.
 * Public compositor/callback coverage, not physical SDL menu-event coverage. */
static void save_test_frame(void) {
 if(!save_test_mode || save_test_done || (save_test_mode==1 && !save_test_pending))return;
 save_test_done=1;
 int ok=1;
 if(save_test_mode==1){
  const DynaProgressCode *current=dyna_progress_session_current(&session);
  if(!current || current->source_pc!=0x1becc)ok=0;
  else if(!dyna_progress_runtime_action("dyna.save") || strcmp(status,"PROGRESS SAVED"))ok=0;
 }
 unsigned n=dyna_progress_session_count(&session);
 if(save_test_mode==3){
  fprintf(stderr,"[progress-save-test] %s fresh-profile count=%u\n",n?"FAIL":"PASS",n);
  return;
 }
 DynaProgressStore loaded;DynaProgressSession checked;
 if(!n || !dyna_progress_init(&loaded,store.game,store.rom_sha256) ||
    !dyna_progress_load(&loaded,path) ||
    !dyna_progress_session_init(&checked,&loaded,session.rom,session.rom_size) ||
    dyna_progress_session_count(&checked)!=n)ok=0;
 const DynaProgressCode *first=dyna_progress_session_item(&session,0);
 DynaProgressCode expected={0};
 if(first)expected=*first;else ok=0;
 if(ok){
  const DynaProgressCode *disk=dyna_progress_session_item(&checked,0);
  if(!disk || disk->mode!=expected.mode || disk->stage!=expected.stage ||
     disk->length!=expected.length || memcmp(disk->code,expected.code,expected.length))ok=0;
 }
 dyna_extras_ui_reset();dyna_extras_ui_open();
 dyna_extras_ui_input(6,1,0);
 DynaExtrasUiState ui=dyna_extras_ui_state();
 if(ui.screen!=1 || ui.selected!=0)ok=0;
 uint32_t *pixels=calloc(640u*360u,sizeof *pixels);
 char capture[2048];
 if(!pixels || snprintf(capture,sizeof capture,"%sprogress-save-levels.ppm",folder)>=(int)sizeof capture)ok=0;
 else {
  dyna_extras_ui_render(pixels,640,360,640*4);
  FILE *f=fopen(capture,"wb");
  if(!f)ok=0;
  else {
   int wrote=fprintf(f,"P6\n640 360\n255\n")>0;
   for(unsigned i=0;i<640u*360u && wrote;i++){
    unsigned char rgb[3]={(unsigned char)(pixels[i]>>16),(unsigned char)(pixels[i]>>8),(unsigned char)pixels[i]};
    wrote=fwrite(rgb,1,3,f)==3;
   }
   if(fclose(f) || !wrote)ok=0;
  }
 }
 free(pixels);
 /* Catalogue order differs from saved-store order. Select by identity. */
 unsigned target_index=0;int found=0;
 for(unsigned i=0;i<count(NULL,store.game);i++){
  Db1LevelCatalogItem entry;
  if(db1_level_catalog_item(&session,i,&entry) &&
     entry.mode==expected.mode && entry.stage==expected.stage){target_index=i;found=1;break;}
 }
 if(!found)ok=0;
 for(unsigned i=0;i<target_index;i++)dyna_extras_ui_input(3,1,0);
 if(dyna_extras_ui_state().selected!=target_index)ok=0;
 dyna_extras_ui_input(6,1,0); /* Real provider launch callback. */
 if(dyna_extras_ui_is_open() || replay.requested.source_pc!=0)ok=0;
 if(replay.status!=DYNA_REPLAY_WAIT_PASSWORD || replay.requested.mode!=expected.mode ||
    replay.requested.stage!=expected.stage || replay.requested.length!=expected.length ||
    memcmp(replay.requested.code,expected.code,expected.length))ok=0;
 dyna_progress_replay_cancel(&replay);
 if(dyna_extras_ui_is_open())dyna_extras_ui_input(0,1,0);
 if(dyna_extras_ui_is_open())ok=0;
 fprintf(stderr,"[progress-save-test] %s mode=%d count=%u source=%06X target=%u/%u persisted=%s\n",
  ok?"PASS":"FAIL",save_test_mode,n,expected.source_pc,expected.mode,expected.stage,path);
}

int dyna_progress_runtime_take_navigation_request(void){
 int take=navigation_requested && ready && store.game==1 && replay.status==DYNA_REPLAY_WAIT_PASSWORD;
 navigation_requested=0;return take;
}
void dyna_progress_runtime_navigation_status(const char *text,int cancel){
 if(cancel && replay.status>=DYNA_REPLAY_WAIT_PASSWORD && replay.status<=DYNA_REPLAY_ACCEPTED)
  dyna_progress_replay_cancel(&replay);
 snprintf(status,sizeof status,"%s",text?text:"");dyna_extras_ui_notice(status);
}
uint16_t dyna_progress_runtime_input(unsigned player,uint16_t mask) {
 if(!ready || player)return mask;
 save_test_frame();
 int replace=0;unsigned buttons=0;
 DynaProgressReplayStatus before=replay.status;
 if(test_index>=0 && test_queued && test_cancel_after && replay.elapsed>=test_cancel_after &&
    replay.status>=DYNA_REPLAY_WAIT_PASSWORD && replay.status<=DYNA_REPLAY_ACCEPTED)
  dyna_progress_replay_cancel(&replay);
 dyna_progress_replay_frame(&replay,&replace,&buttons);
 if(test_index>=0 && (replace || replay.status!=before)){
  test_log(0);fprintf(stderr,"[progress-test] override=%d buttons=%u\n",replace,buttons);
 }
 if(store.game==1){
  unsigned input=0;int block=0,start=0;
  if(mask&GPAD_UP)input|=DYNA_PROGRESS_UI_UP;
  if(mask&GPAD_DOWN)input|=DYNA_PROGRESS_UI_DOWN;
  if(mask&(GPAD_A|GPAD_C))input|=DYNA_PROGRESS_UI_CONFIRM;
  if(mask&GPAD_START)input|=DYNA_PROGRESS_UI_START;
  if(mask&~(GPAD_UP|GPAD_DOWN|GPAD_A|GPAD_C|GPAD_START))input|=16u;
  if(dyna_progress_ui_frame(&prompt,input,&block,&start))
   dyna_progress_ui_saved(&prompt,save_current());
  if(block)return start?GPAD_START:0;
 }
 if(!replace)return mask;
 return (buttons&DYNA_REPLAY_PAD_A?GPAD_A:0) |
        (buttons&DYNA_REPLAY_PAD_B?GPAD_B:0) |
        (buttons&DYNA_REPLAY_PAD_C?GPAD_C:0);
}
