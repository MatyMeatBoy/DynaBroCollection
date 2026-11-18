/* Host persistence bridge. Guest code capture remains immutable. */
#include "progress_runtime.h"
#include "progress_session.h"
#include "progress_replay.h"
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
static char folder[1800],path[2048],status[128];
static int ready;
/* Dormant unless explicitly requested by a native qualification run. */
static int test_index=-1,test_queued,test_queue_title;
static unsigned test_cancel_after;
static void test_setup(unsigned game) {
 const char *v=getenv("DYNA_DB1_REPLAY_TEST_INDEX");char *end;
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
 if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(alg,0);return ok;
#else
 (void)rom;(void)size;(void)text;return 0;
#endif
}
static size_t count(void *u,unsigned game){(void)u;return ready&&game==store.game?dyna_progress_session_count(&session):0;}
static int item(void *u,unsigned game,size_t i,DynaExtrasLevel *out) {
 (void)u;static char title[80],detail[120];
 if(!out || !ready || game!=store.game || i>255)return 0;
 const DynaProgressCode *c=dyna_progress_session_item(&session,(unsigned)i);char code[9];
 if(!c || !dyna_progress_session_text(&session,c,code))return 0;
 snprintf(title,sizeof title,"MODE %u / STAGE %u",c->mode,c->stage);
 snprintf(detail,sizeof detail,"PASSWORD %s - ENTER ON PASSWORD SCREEN",code);
 out->title=title;out->detail=detail;out->available=1;return 1;
}
static int launch(void *u,unsigned game,size_t i) {
 (void)u;if(!ready || game!=store.game || i>255)return 0;
 const DynaProgressCode *code=dyna_progress_session_item(&session,(unsigned)i);
 if(!code || !dyna_progress_replay_request(&replay,code))return 0;
 dyna_extras_ui_notice(dyna_progress_replay_status_text(&replay));
 return 0; /* Keep the guidance visible until the user closes Extras. */
}
int dyna_progress_runtime_start(unsigned game,const uint8_t *rom,size_t size) {
 test_setup(game);
 char hash[65];ready=0;snprintf(status,sizeof status,"SAVE UNAVAILABLE");
 if(!folder[0] || !rom || !identity(rom,size,hash) || !dyna_progress_init(&store,game,hash))return 0;
 if(snprintf(path,sizeof path,"%sdyna%u-progress-%.16s.bin",folder,game,hash)>=(int)sizeof path)return 0;
 FILE *f=fopen(path,"rb");if(f){fclose(f);if(!dyna_progress_load(&store,path)){snprintf(status,sizeof status,"SAVE FILE COULD NOT BE READ");path[0]=0;return 0;}}
 ready=dyna_progress_session_init(&session,&store,rom,size);
 dyna_progress_replay_init(&replay,game,rom,size);
 DynaExtrasLevelProvider provider={NULL,count,item,launch};dyna_extras_ui_set_level_provider(&provider);
 snprintf(status,sizeof status,"SAVE AT A PASSWORD SCREEN");return ready;
}
void dyna_progress_runtime_before(uint32_t pc,uint8_t *ram,size_t size,uint32_t d[8],const uint32_t a[8]) {
 switch(pc) {
 case 0x1950:case 0x1c5a4:case 0x1d7b6:case 0x1becc:case 0x1dd90:case 0x28f1a:
 case 0x3a38:case 0x6f9a:case 0x4913e:case 0x58ef2:
 case 0x1db7e:case 0x1db88:case 0x1dcec:case 0x1dd1a:case 0x1dce4:
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
  if(replay.status!=before){
   dyna_extras_ui_notice(dyna_progress_replay_status_text(&replay));
   if(test_index>=0 && size>=0x3a){
    test_log(pc);
    fprintf(stderr,"[progress-test] stage=%02X%02X ptr=%02X%02X%02X%02X d4=%08X code=%02X%02X%02X%02X\n",
     ram[0x34],ram[0x35],ram[0x2e],ram[0x2f],ram[0x30],ram[0x31],d[4],ram[0x36],ram[0x37],ram[0x38],ram[0x39]);
   }
  }
 }
}
int dyna_progress_runtime_action(const char *key) {
 if(!key || strcmp(key,"dyna.save"))return 0;
 if(!ready || !dyna_progress_session_current(&session)){snprintf(status,sizeof status,"SAVE AT A PASSWORD SCREEN");dyna_extras_ui_notice(status);dyna_extras_ui_open();return 1;}
 int ok=dyna_progress_session_save(&session,path);
 snprintf(status,sizeof status,"%s",ok?"PROGRESS SAVED":"SAVE FAILED - PREVIOUS SAVE KEPT");
 dyna_extras_ui_notice(status);dyna_extras_ui_open();
 fprintf(stderr,"[progress] %s\n",status);return 1;
}
const char *dyna_progress_runtime_status(void){return status;}

uint16_t dyna_progress_runtime_input(unsigned player,uint16_t mask) {
 if(!ready || player)return mask;
 int replace=0;unsigned buttons=0;
 DynaProgressReplayStatus before=replay.status;
 if(test_index>=0 && test_queued && test_cancel_after && replay.elapsed>=test_cancel_after &&
    replay.status>=DYNA_REPLAY_WAIT_PASSWORD && replay.status<=DYNA_REPLAY_ACCEPTED)
  dyna_progress_replay_cancel(&replay);
 dyna_progress_replay_frame(&replay,&replace,&buttons);
 if(test_index>=0 && (replace || replay.status!=before)){
  test_log(0);fprintf(stderr,"[progress-test] override=%d buttons=%u\n",replace,buttons);
 }
 if(!replace)return mask;
 return (buttons&DYNA_REPLAY_PAD_A?GPAD_A:0) |
        (buttons&DYNA_REPLAY_PAD_B?GPAD_B:0) |
        (buttons&DYNA_REPLAY_PAD_C?GPAD_C:0);
}
