#include "db1_progress_migration.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <errno.h>
#endif
static const char *canonical="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
static const char *english="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
static const char *english2="cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
static unsigned char rom[0x20000],rom_copy[0x20000];
static char path[4096],legacy_path[4096],legacy2_path[4096];
#ifdef _WIN32
static void wide(const char *s,wchar_t *w){assert(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,w,4096));}
#endif
static void erase(const char *s){
#ifdef _WIN32
 wchar_t w[4096];wide(s,w);DeleteFileW(w);
#else
 remove(s);
#endif
}
static void corrupt(const char *s){
#ifdef _WIN32
 wchar_t w[4096];wide(s,w);FILE *f=_wfopen(w,L"wb");
#else
 FILE *f=fopen(s,"wb");
#endif
 assert(f);assert(fwrite("bad",1,3,f)==3);assert(!fclose(f));
}
static DynaProgressCode code(unsigned mode,unsigned stage){
 DynaProgressCode c={0};c.mode=(uint16_t)mode;c.stage=(uint16_t)stage;c.length=4;c.source_pc=0x1becc;
 unsigned index=mode?20*mode+stage-1:stage-1;
 memcpy(c.code,rom+0x1e87eu+4*index,4);return c;
}
int main(int argc,char **argv){
 assert(argc==2);char unicode_folder[3000];
 snprintf(unicode_folder,sizeof unicode_folder,"%s/perfil-\xc3\xb1-\xe6\xbc\xa2\xe5\xad\x97",argv[1]);
 const char *folder=unicode_folder;
#ifdef _WIN32
 wchar_t directory[4096];wide(folder,directory);assert(CreateDirectoryW(directory,NULL)||GetLastError()==ERROR_ALREADY_EXISTS);
#else
 assert(mkdir(folder,0700)==0||errno==EEXIST);
#endif
 snprintf(path,sizeof path,"%s/dyna1-progress-%.16s.bin",folder,canonical);
 snprintf(legacy_path,sizeof legacy_path,"%s/dyna1-progress-%.16s.bin",folder,english);
 snprintf(legacy2_path,sizeof legacy2_path,"%s/dyna1-progress-%.16s.bin",folder,english2);
 erase(path);erase(legacy_path);erase(legacy2_path);
 for(unsigned i=0;i<97;i++)for(unsigned j=0;j<4;j++)rom[0x1e87eu+4*i+j]=(unsigned char)(4*((i+j)%26));
 memcpy(rom_copy,rom,sizeof rom);
 const char *hashes[]={canonical,english,english2,english};
 DynaProgressStore destination,legacy,old;Db1ProgressMigrationReport report;
 assert(dyna_progress_init(&destination,1,canonical));old=destination;
 assert(db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));
 assert(!report.written&&!report.imported&&!dyna_progress_file_exists(path));
 assert(!memcmp(&old,&destination,sizeof old));
 assert(dyna_progress_init(&legacy,1,english));
 DynaProgressCode normal=code(1,4),hard=code(2,8),bad=code(0,3);bad.code[0]=255;
 assert(dyna_progress_add(&legacy,&normal));assert(dyna_progress_add(&legacy,&bad));
 assert(dyna_progress_save(&legacy,legacy_path));DynaProgressStore original_legacy=legacy;
 assert(dyna_progress_init(&legacy,1,english2));assert(dyna_progress_add(&legacy,&normal));assert(dyna_progress_add(&legacy,&hard));
 assert(dyna_progress_save(&legacy,legacy2_path));
 assert(db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));
 assert(report.imported==2&&report.invalid_codes==1&&report.inspected==2&&report.written&&destination.count==2);
 assert(!memcmp(rom,rom_copy,sizeof rom));
 assert(dyna_progress_init(&legacy,1,english));assert(dyna_progress_load(&legacy,legacy_path));
 assert(!memcmp(&legacy,&original_legacy,sizeof legacy));
 old=destination;
 assert(db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));
 assert(!report.imported&&!report.written&&!memcmp(&old,&destination,sizeof old));
#ifdef _WIN32
 /* Reading a valid canonical into fresh RAM never needs replacement rights. */
 wchar_t read_path[4096];wide(path,read_path);
 HANDLE read_lock=CreateFileW(read_path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
 assert(read_lock!=INVALID_HANDLE_VALUE);
 FILETIME before_write,after_write;assert(GetFileTime(read_lock,NULL,NULL,&before_write));
 unsigned char before_bytes[8192],after_bytes[8192];DWORD before_size=0,after_size=0;
 assert(ReadFile(read_lock,before_bytes,sizeof before_bytes,&before_size,NULL));
 assert(dyna_progress_init(&destination,1,canonical));
 assert(db1_progress_migration_run(&destination,folder,canonical,NULL,0,rom,sizeof rom,&report));
 assert(!report.written&&destination.count==2&&!memcmp(&old,&destination,sizeof old));
 assert(GetFileTime(read_lock,NULL,NULL,&after_write));
 assert(before_write.dwLowDateTime==after_write.dwLowDateTime&&before_write.dwHighDateTime==after_write.dwHighDateTime);
 assert(SetFilePointer(read_lock,0,NULL,FILE_BEGIN)!=INVALID_SET_FILE_POINTER);
 assert(ReadFile(read_lock,after_bytes,sizeof after_bytes,&after_size,NULL));
 assert(before_size==after_size&&!memcmp(before_bytes,after_bytes,before_size));assert(CloseHandle(read_lock));
#endif
 /* Empty caller loads the canonical union without rewriting disk. */
 assert(dyna_progress_init(&destination,1,canonical));
 assert(db1_progress_migration_run(&destination,folder,canonical,NULL,0,rom,sizeof rom,&report));assert(destination.count==2);
 /* JA/EN payload edits outside the native code table preserve validated codes. */
 rom[0x100]=42;old=destination;
 assert(db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));assert(!memcmp(&old,&destination,sizeof old));
 /* Corrupt legacy is reported and never replaced or removed. */
 corrupt(legacy2_path);
 assert(db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));assert(report.corrupt_legacy==1&&destination.count==2);
 assert(!dyna_progress_load(&legacy,legacy2_path));
#ifdef _WIN32
 /* Deny replacement of existing canonical while introducing a valid new code. */
 DynaProgressCode extra=code(0,2);assert(dyna_progress_add(&legacy,&extra));
 assert(dyna_progress_save(&legacy,legacy_path));
 wchar_t w[4096];wide(path,w);HANDLE lock=CreateFileW(w,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);assert(lock!=INVALID_HANDLE_VALUE);
 old=destination;
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));
 assert(report.status==DB1_MIGRATION_WRITE_FAILED&&!report.imported&&!memcmp(&old,&destination,sizeof old));
 assert(CloseHandle(lock));DynaProgressStore disk;assert(dyna_progress_init(&disk,1,canonical));assert(dyna_progress_load(&disk,path));assert(!memcmp(&old,&disk,sizeof old));
#endif
 /* Corrupt canonical blocks all imports and preserves both RAM and file. */
 corrupt(path);old=destination;
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));assert(report.status==DB1_MIGRATION_CANONICAL_CORRUPT&&!memcmp(&old,&destination,sizeof old));
 assert(!dyna_progress_load(&destination,path));
 erase(path);
 DynaProgressStore invalid_canonical;assert(dyna_progress_init(&invalid_canonical,1,canonical));
 assert(dyna_progress_add(&invalid_canonical,&bad));assert(dyna_progress_save(&invalid_canonical,path));
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));
 assert(report.status==DB1_MIGRATION_CANONICAL_CORRUPT&&!memcmp(&old,&destination,sizeof old));
 erase(path);
 DynaProgressCode invalid_input=code(0,1);invalid_input.code[0]=255;
 assert(dyna_progress_add(&destination,&invalid_input));old=destination;
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));
 assert(report.status==DB1_MIGRATION_INVALID&&!memcmp(&old,&destination,sizeof old)&&!dyna_progress_file_exists(path));
 destination.count=257;old=destination;
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));assert(!memcmp(&old,&destination,sizeof old)&&!dyna_progress_file_exists(path));
 assert(dyna_progress_init(&destination,2,canonical));old=destination;
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,sizeof rom,&report));assert(!memcmp(&old,&destination,sizeof old));
 assert(dyna_progress_init(&destination,1,canonical));
 const char *collision[]={"aaaaaaaaaaaaaaaa111111111111111111111111111111111111111111111111"};
 assert(!db1_progress_migration_run(&destination,folder,canonical,collision,1,rom,sizeof rom,&report));
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,9,rom,sizeof rom,&report));
 assert(!db1_progress_migration_run(&destination,folder,canonical,hashes,4,rom,8,&report));
 erase(path);erase(legacy_path);erase(legacy2_path);
 puts("DB1 migration fresh/noIO/union/dedup/Unicode/locked replacement/corruption/DB2/immutability passed");return 0;
}
