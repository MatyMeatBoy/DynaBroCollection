#include "db1_level_catalog.h"
#include <stdio.h>
#include <string.h>
#define PASSWORD_TABLE 0x1e87eu
#define PASSWORD_ENTRIES 97u
static const unsigned stages[5]={8,20,20,20,17};
static const char *const names[5]={"Practice","Normal","Hard","Back","Alien"};
typedef struct {unsigned frontier[4],start_modes;uint32_t saved[5];} Access;
const char *db1_level_catalog_mode_name(unsigned mode){return mode<5?names[mode]:NULL;}
static int session_valid(const DynaProgressSession *s){
 if(!s||!s->store||!s->rom||s->store->game!=1||s->store->count>DYNA_PROGRESS_LIMIT||
    s->rom_size<PASSWORD_TABLE+PASSWORD_ENTRIES*4||s->store->rom_sha256[64])return 0;
 for(unsigned i=0;i<64;i++){
  char c=s->store->rom_sha256[i];
  if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return 0;
 }
 return 1;
}
static int target_valid(unsigned mode,unsigned stage){return mode<5&&stage>=1&&stage<=stages[mode];}
static int collect(const DynaProgressSession *s,Access *a){
 if(!session_valid(s))return 0;
 memset(a,0,sizeof *a);
 for(unsigned mode=0;mode<4;mode++)
  if(s->db1_cleared_modes&(1u<<mode))a->start_modes=(2u<<mode)-1;
 for(unsigned i=0;i<s->store->count;i++){
  const DynaProgressCode *c=&s->store->codes[i];char text[9];
  /* Public session validator rechecks exact mode/stage/length/table bytes.
   * One bounded pass avoids repeatedly enumerating earlier saved entries. */
  if(c->source_pc>0xffffffu||!target_valid(c->mode,c->stage)||!dyna_progress_session_text(s,c,text))continue;
  a->saved[c->mode]|=UINT32_C(1)<<(c->stage-1);
  if(c->mode<4&&a->frontier[c->mode]<c->stage)a->frontier[c->mode]=c->stage;
 }
 return 1;
}
static int allowed(const Access *a,unsigned mode,unsigned stage){
 if(!target_valid(mode,stage))return 0;
 if(mode==4)return (a->saved[4]&(UINT32_C(1)<<(stage-1)))!=0;
 if(stage==1)return (a->start_modes&(1u<<mode))!=0 || (mode==3&&a->frontier[3]!=0);
 return stage<=a->frontier[mode];
}
static int native_code(const DynaProgressSession *s,unsigned mode,unsigned stage,DynaProgressCode *out){
 if(!target_valid(mode,stage))return 0;
 unsigned index=mode==0?stage-1:mode==4?79+stage:20*mode+stage-1;
 DynaProgressCode c={0};c.mode=(uint16_t)mode;c.stage=(uint16_t)stage;c.length=4;
 memcpy(c.code,s->rom+PASSWORD_TABLE+4*index,4);
 char text[9];if(!dyna_progress_session_text(s,&c,text))return 0;
 if(out)*out=c;
 return 1;
}
unsigned db1_level_catalog_count(const DynaProgressSession *s){
 Access a;if(!collect(s,&a))return 0;
 unsigned count=0;
 for(unsigned mode=0;mode<5;mode++)for(unsigned stage=1;stage<=stages[mode];stage++)
  if(allowed(&a,mode,stage)&&native_code(s,mode,stage,NULL))++count;
 return count;
}
int db1_level_catalog_item(const DynaProgressSession *s,unsigned index,Db1LevelCatalogItem *out){
 Access a;if(!out||!collect(s,&a))return 0;
 for(unsigned mode=0;mode<5;mode++)for(unsigned stage=1;stage<=stages[mode];stage++){
  if(!allowed(&a,mode,stage)||!native_code(s,mode,stage,NULL))continue;
  if(index){--index;continue;}
  Db1LevelCatalogItem item={0};item.mode=mode;item.stage=stage;item.mode_name=names[mode];
  item.has_saved_code=(a.saved[mode]&(UINT32_C(1)<<(stage-1)))!=0;
  snprintf(item.label,sizeof item.label,"%s %u",names[mode],stage);*out=item;return 1;
 }
 return 0;
}
int db1_level_catalog_request_code(const DynaProgressSession *s,unsigned mode,unsigned stage,DynaProgressCode *out){
 Access a;if(!out||!collect(s,&a)||!allowed(&a,mode,stage))return 0;
 return native_code(s,mode,stage,out);
}
