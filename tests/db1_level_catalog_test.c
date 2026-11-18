#include "db1_level_catalog.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char rom[0x20000],rom_copy[sizeof rom];
static const char hash[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
static unsigned index_for(unsigned mode,unsigned stage){return mode==0?stage-1:mode==4?79+stage:20*mode+stage-1;}
static DynaProgressCode code(unsigned mode,unsigned stage){
 DynaProgressCode c={0};c.mode=(uint16_t)mode;c.stage=(uint16_t)stage;c.length=4;c.source_pc=0x1becc;
 memcpy(c.code,rom+0x1e87eu+4*index_for(mode,stage),4);return c;
}
static void saved(DynaProgressStore *store,unsigned mode,unsigned stage){DynaProgressCode c=code(mode,stage);assert(dyna_progress_add(store,&c));}
static void check_code(DynaProgressSession *s,unsigned mode,unsigned stage){
 DynaProgressCode c;assert(db1_level_catalog_request_code(s,mode,stage,&c));
 assert(c.mode==mode&&c.stage==stage&&c.length==4&&c.source_pc==0);
 assert(!memcmp(c.code,rom+0x1e87eu+4*index_for(mode,stage),4));
}
static void synthetic(void){
 for(unsigned i=0;i<97;i++){unsigned n=i;for(unsigned j=0;j<4;j++){rom[0x1e87eu+4*i+j]=(uint8_t)(2+4*(n%26));n/=26;}}
 DynaProgressStore store;DynaProgressSession s;Db1LevelCatalogItem item;
 assert(dyna_progress_init(&store,1,hash)&&dyna_progress_session_init(&s,&store,rom,sizeof rom));
 assert(db1_level_catalog_count(&s)==3);
 const char *names[]={"Practice","Normal","Hard","Back","Alien"};
 for(unsigned mode=0;mode<5;mode++)assert(!strcmp(db1_level_catalog_mode_name(mode),names[mode]));
 assert(!db1_level_catalog_mode_name(5));
 for(unsigned i=0;i<3;i++){
  assert(db1_level_catalog_item(&s,i,&item)&&item.mode==i&&item.stage==1&&!item.has_saved_code);
  assert(!strcmp(item.mode_name,names[i]));char expected[32];snprintf(expected,sizeof expected,"%s 1",names[i]);assert(!strcmp(item.label,expected));check_code(&s,i,1);
 }
 DynaProgressCode no={0},before;memset(&no,0xa5,sizeof no);before=no;
 assert(!db1_level_catalog_request_code(&s,3,1,&no)&&!memcmp(&no,&before,sizeof no));
 s.current=code(3,1);s.current.source_pc=0x28f1a;s.has_current=1;
 assert(db1_level_catalog_count(&s)==3); /* An unsaved MHRI unlocks nothing. */
 saved(&store,2,20);assert(db1_level_catalog_count(&s)==22);
 assert(!db1_level_catalog_request_code(&s,3,1,&no)); /* Hard20 is not Back. */
 saved(&store,1,4);saved(&store,0,8);assert(db1_level_catalog_count(&s)==32);
 saved(&store,3,5);assert(db1_level_catalog_count(&s)==37);check_code(&s,3,1);check_code(&s,3,5);
 assert(db1_level_catalog_item(&s,32,&item)&&item.mode==3&&item.stage==1&&!item.has_saved_code);
 assert(db1_level_catalog_item(&s,36,&item)&&item.mode==3&&item.stage==5&&item.has_saved_code);
 saved(&store,4,7);assert(db1_level_catalog_count(&s)==38);check_code(&s,4,7);
 assert(!db1_level_catalog_request_code(&s,4,6,&no));
 saved(&store,4,17);saved(&store,4,1);assert(db1_level_catalog_count(&s)==40);
 assert(db1_level_catalog_item(&s,37,&item)&&item.mode==4&&item.stage==1);
 assert(db1_level_catalog_item(&s,38,&item)&&item.stage==7);
 assert(db1_level_catalog_item(&s,39,&item)&&item.stage==17);
 DynaProgressStore copy=store;DynaProgressSession session_copy=s;memcpy(rom_copy,rom,sizeof rom);
 for(unsigned i=0;i<40;i++){assert(db1_level_catalog_item(&s,i,&item));check_code(&s,item.mode,item.stage);}
 assert(!memcmp(&store,&copy,sizeof store)&&!memcmp(&s,&session_copy,sizeof s)&&!memcmp(rom,rom_copy,sizeof rom));
 Db1LevelCatalogItem untouched;memset(&untouched,0xa5,sizeof untouched);item=untouched;
 assert(!db1_level_catalog_item(&s,40,&item)&&!memcmp(&item,&untouched,sizeof item));
 assert(!db1_level_catalog_item(&s,UINT_MAX,&item)&&!memcmp(&item,&untouched,sizeof item));
 const unsigned bad[][2]={{0,0},{0,9},{1,5},{2,21},{3,6},{4,2},{4,18},{5,1}};
 for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++)assert(!db1_level_catalog_request_code(&s,bad[i][0],bad[i][1],&no)&&!memcmp(&no,&before,sizeof no));
 /* Corrupt stored bytes, target metadata and length never produce access. */
 assert(dyna_progress_init(&store,1,hash));saved(&store,1,20);store.codes[0].code[0]^=4;
 assert(db1_level_catalog_count(&s)==3&&!db1_level_catalog_request_code(&s,1,20,&no));
 store.codes[0]=code(0,8);store.codes[0].stage=9;assert(db1_level_catalog_count(&s)==3);
 store.codes[0]=code(3,1);store.codes[0].length=8;assert(db1_level_catalog_count(&s)==3);
 store.codes[0]=code(4,7);store.codes[0].mode=9;assert(db1_level_catalog_count(&s)==3);
 store.codes[0]=code(3,1);store.codes[0].source_pc=0x1000000u;assert(db1_level_catalog_count(&s)==3);
 store.count=DYNA_PROGRESS_LIMIT+1;assert(!db1_level_catalog_count(&s));store.count=0;
 store.rom_sha256[0]='x';assert(!db1_level_catalog_count(&s));store.rom_sha256[0]='0';
 s.rom_size=0x1e87eu+97*4-1;assert(!db1_level_catalog_count(&s));s.rom_size=sizeof rom;
 store.game=2;assert(!db1_level_catalog_count(&s)&&!db1_level_catalog_request_code(&s,0,1,&no));store.game=1;
 assert(!db1_level_catalog_count(NULL)&&!db1_level_catalog_item(&s,0,NULL)&&!db1_level_catalog_request_code(&s,0,1,NULL));
 /* A bad ROM target is never synthesized, even as an otherwise free base. */
 rom[0x1e87eu]=0xff;assert(db1_level_catalog_count(&s)==2&&!db1_level_catalog_request_code(&s,0,1,&no));
 memcpy(rom,rom_copy,sizeof rom);
 assert(dyna_progress_init(&store,1,hash));DynaProgressCode mhri=code(3,1);mhri.source_pc=0x28f1a;assert(dyna_progress_add(&store,&mhri));
 assert(db1_level_catalog_count(&s)==4&&db1_level_catalog_item(&s,3,&item)&&item.mode==3&&item.stage==1&&item.has_saved_code);
 assert(!db1_level_catalog_request_code(&s,3,2,&no));
 puts("DB1 catalogue bases/frontiers/secret gates/ROM validation/immutability passed");
}
static void original_table(const char *path){
 FILE *f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long length=ftell(f);assert(length>0);rewind(f);
 unsigned char *bytes=malloc((size_t)length);assert(bytes);assert(fread(bytes,1,(size_t)length,f)==(size_t)length);assert(!fclose(f));
 DynaProgressStore store;DynaProgressSession s;DynaProgressCode c;char text[9];
 assert(dyna_progress_init(&store,1,hash)&&dyna_progress_session_init(&s,&store,bytes,(size_t)length));
 assert(db1_level_catalog_count(&s)==3);
 for(unsigned mode=0;mode<3;mode++)assert(db1_level_catalog_request_code(&s,mode,1,&c)&&c.source_pc==0);
 c=(DynaProgressCode){0};c.mode=3;c.stage=1;c.length=4;c.source_pc=0x28f1a;memcpy(c.code,bytes+0x1e87eu+60*4,4);
 assert(dyna_progress_session_text(&s,&c,text)&&!strcmp(text,"MHRI"));assert(dyna_progress_add(&store,&c));
 assert(db1_level_catalog_count(&s)==4&&db1_level_catalog_request_code(&s,3,1,&c));
 c.mode=4;c.stage=7;c.length=4;c.source_pc=0x1dd90;memcpy(c.code,bytes+0x1e87eu+86*4,4);assert(dyna_progress_add(&store,&c));
 assert(db1_level_catalog_count(&s)==5&&db1_level_catalog_request_code(&s,4,7,&c));
 assert(!db1_level_catalog_request_code(&s,4,1,&c));
 free(bytes);puts("Read-only real-ROM table smoke passed; saved entries are RAM-only fixtures, not earned progress");
}
int main(int argc,char **argv){synthetic();if(argc>1)original_table(argv[1]);return 0;}
