#include "db1_progress_migration.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
static int same(const DynaProgressStore *a,const DynaProgressStore *b){
 if(a->game!=b->game||a->count!=b->count||strcmp(a->rom_sha256,b->rom_sha256))return 0;
 for(unsigned i=0;i<a->count;i++){
  const DynaProgressCode *x=&a->codes[i],*y=&b->codes[i];
  if(x->mode!=y->mode||x->stage!=y->stage||x->length!=y->length||
     x->source_pc!=y->source_pc||memcmp(x->code,y->code,x->length))return 0;
 }
 return 1;
}
static int code_valid(DynaProgressSession *session,const DynaProgressCode *c){
 char text[9];return c->source_pc<=0xffffffu&&dyna_progress_session_text(session,c,text);
}
static int path_for(char *path,size_t size,const char *folder,const char *hash){
 size_t n=strlen(folder);const char *sep=n&&(folder[n-1]=='/'||folder[n-1]=='\\')?"":"/";
 return snprintf(path,size,"%s%sdyna1-progress-%.16s.bin",folder,sep,hash)<(int)size;
}
int db1_progress_migration_run(DynaProgressStore *destination,const char *folder,
 const char *canonical_sha,const char *const *hashes,size_t count,
 const uint8_t *rom,size_t size,Db1ProgressMigrationReport *report){
 Db1ProgressMigrationReport r={0};r.status=DB1_MIGRATION_INVALID;
 DynaProgressStore initial,merged,disk;DynaProgressSession validation;
 char identities[DB1_PROGRESS_MIGRATION_MAX_HASHES][65];
 char *path=NULL,*canonical_path=NULL;int ok=0;
 if(!destination||!folder||!*folder||strlen(folder)>29800||!rom||size<0x1e87eu+97*4||
    count>DB1_PROGRESS_MIGRATION_MAX_HASHES||(count&&!hashes)||
    !dyna_progress_init(&initial,1,canonical_sha)||destination->game!=1||
    destination->count>DYNA_PROGRESS_LIMIT||memcmp(destination->rom_sha256,initial.rom_sha256,65))goto done;
 for(size_t i=0;i<count;i++){
  DynaProgressStore identity;if(!dyna_progress_init(&identity,1,hashes[i]))goto done;
  memcpy(identities[i],identity.rom_sha256,65);
  if(!strncmp(identities[i],initial.rom_sha256,16)&&strcmp(identities[i],initial.rom_sha256))goto done;
  for(size_t j=0;j<i;j++)if(!strncmp(identities[i],identities[j],16)&&strcmp(identities[i],identities[j]))goto done;
 }
 path=malloc(strlen(folder)+128);canonical_path=malloc(strlen(folder)+128);
 if(!path||!canonical_path)goto done;
 size_t path_size=strlen(folder)+128;
 if(!path_for(canonical_path,path_size,folder,initial.rom_sha256))goto done;
 disk=initial;
 int exists=dyna_progress_file_exists(canonical_path);
 if(exists&&!dyna_progress_load(&disk,canonical_path)){r.status=DB1_MIGRATION_CANONICAL_CORRUPT;goto done;}
 merged=disk;
 if(!dyna_progress_session_init(&validation,&merged,rom,size))goto done;
 for(unsigned i=0;i<disk.count;i++)if(!code_valid(&validation,&disk.codes[i])){
  r.status=DB1_MIGRATION_CANONICAL_CORRUPT;goto done;
 }
 /* Unsaved caller entries may be merged, but a malformed input is never hidden. */
 for(unsigned i=0;i<destination->count;i++){
  if(!code_valid(&validation,&destination->codes[i]))goto done;
  if(!dyna_progress_add(&merged,&destination->codes[i])){r.status=DB1_MIGRATION_CAPACITY;goto done;}
 }
 for(size_t i=0;i<count;i++){
  if(!strcmp(identities[i],initial.rom_sha256))continue;
  int duplicate=0;for(size_t j=0;j<i;j++)if(!strcmp(identities[i],identities[j]))duplicate=1;
  if(duplicate)continue;
  if(!path_for(path,path_size,folder,identities[i]))goto done;
  if(!dyna_progress_file_exists(path))continue;
  ++r.inspected;
  DynaProgressStore legacy;
  if(!dyna_progress_init(&legacy,1,identities[i])||!dyna_progress_load(&legacy,path)){++r.corrupt_legacy;continue;}
  for(unsigned j=0;j<legacy.count;j++){
   if(!code_valid(&validation,&legacy.codes[j])){++r.invalid_codes;continue;}
   unsigned before=merged.count;
   if(!dyna_progress_add(&merged,&legacy.codes[j])){r.status=DB1_MIGRATION_CAPACITY;goto done;}
   if(merged.count!=before)++r.imported;
  }
 }
 if(!same(&merged,&disk)||(!exists&&merged.count)){
  if(!dyna_progress_save(&merged,canonical_path)){r.status=DB1_MIGRATION_WRITE_FAILED;goto done;}
  r.written=1;
 }
 *destination=merged;r.status=DB1_MIGRATION_OK;ok=1;
 done:
 free(path);free(canonical_path);if(!ok)r.imported=0;
 if(report)*report=r;
 return ok;
}
