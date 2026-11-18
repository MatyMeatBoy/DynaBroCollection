#include "../games/launcher_discovery.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
static void write_file(const wchar_t *p,const char *bytes){HANDLE f=CreateFileW(p,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);DWORD n;assert(f!=INVALID_HANDLE_VALUE);assert(WriteFile(f,bytes,(DWORD)strlen(bytes),&n,NULL));CloseHandle(f);}
#endif
int main(void){
 ld_inventory *v=calloc(1,sizeof(*v));assert(v);
 assert(ld_crc32(0,"123456789",9)==0xcbf43926u);
 assert(ld_crc32(ld_crc32(0,"1234",4),"56789",5)==0xcbf43926u);
 assert(ld_classify(L"folder with spaces/test.MD")==LD_ROM);
 assert(ld_classify(L"test.zip")==LD_UNKNOWN);
 assert(ld_classify(L"test.md.zip")==LD_UNKNOWN);
 assert(ld_identify(1048576,0x360c1b20,NULL,0)==1);
 assert(ld_identify(2621440,0xba3554bc,NULL,0)==2);
 assert(!ld_identify(1048575,0x360c1b20,NULL,0));
 assert(!ld_identify(1048576,0,NULL,0));
 v->count=3;v->items[0].kind=v->items[1].kind=v->items[2].kind=LD_ROM;
 v->items[0].game=v->items[1].game=v->items[2].game=1;
 wcscpy(v->items[0].path,L"z.md");wcscpy(v->items[1].path,L"a.md");wcscpy(v->items[2].path,L"cached.md");
 v->items[0].priority=v->items[1].priority=10;v->items[2].priority=0;
 assert(ld_best_rom(v,1)==&v->items[2]);v->count=2;assert(ld_best_rom(v,1)==&v->items[1]);assert(!ld_best_rom(v,2));v->truncated=1;assert(!ld_best_rom(v,1));v->items[1].priority=0;assert(ld_best_rom(v,1));
#ifdef _WIN32
 {
 wchar_t temp[LD_PATH_CAP],base[LD_PATH_CAP],p[LD_PATH_CAP],mods[LD_PATH_CAP],cached[LD_PATH_CAP],cfg[LD_PATH_CAP],custom[LD_PATH_CAP],link[LD_PATH_CAP];
 char utf[128];wchar_t wide[128];ld_options o={0};ld_revision fixture[2]={{9,0xcbf43926u,1},{3,0x352441c2u,2}};DWORD id=GetCurrentProcessId();
 assert(ld_utf8_to_wide("with spaces \xc3\xb1",wide,128));assert(ld_wide_to_utf8(wide,utf,128));assert(!strcmp(utf,"with spaces \xc3\xb1"));
 assert(GetTempPathW(LD_PATH_CAP,temp));swprintf(base,LD_PATH_CAP,L"%lsdiscovery test \u00f1 %lu",temp,(unsigned long)id);assert(CreateDirectoryW(base,NULL));
 swprintf(p,LD_PATH_CAP,L"%ls\\a.md",base);write_file(p,"123456789");
 swprintf(cached,LD_PATH_CAP,L"%ls\\z.gen",base);write_file(cached,"123456789");
 swprintf(p,LD_PATH_CAP,L"%ls\\unknown.bin",base);write_file(p,"unknown");
 swprintf(p,LD_PATH_CAP,L"%ls\\archive.zip",base);write_file(p,"123456789");
 swprintf(mods,LD_PATH_CAP,L"%ls\\mods",base);assert(CreateDirectoryW(mods,NULL));
 swprintf(p,LD_PATH_CAP,L"%ls\\patch.ips",mods);write_file(p,"PATCHEOF");
 swprintf(p,LD_PATH_CAP,L"%ls\\invalid.bps",mods);write_file(p,"invalid");
 swprintf(custom,LD_PATH_CAP,L"%ls\\custom",base);assert(CreateDirectoryW(custom,NULL));
 swprintf(p,LD_PATH_CAP,L"%ls\\second.68k",custom);write_file(p,"abc");
 swprintf(link,LD_PATH_CAP,L"%ls\\roms",base);
 {BOOL linked=CreateSymbolicLinkW(link,custom,3);if(!linked)puts("symlink fixture skipped: OS privilege unavailable");}
 swprintf(cfg,LD_PATH_CAP,L"%ls\\paths.ini",base);write_file(cfg,"[paths]\nroms=custom\nmods=mods\n");
 o.base_dir=base;o.cached_rom[0]=cached;o.revisions=fixture;o.revision_count=2;
 assert(ld_scan(&o,v));assert(v->count==5);assert(ld_best_rom(v,1));assert(!wcscmp(ld_best_rom(v,1)->path,cached));assert(ld_best_rom(v,2));assert(ld_best_rom(v,2)->priority==40);
 {size_t i;int unknown=0,patch=0;for(i=0;i<v->count;i++){if(v->items[i].kind==LD_IPS){patch++;assert(!v->items[i].compatible);}if(v->items[i].kind==LD_ROM&&!v->items[i].game)unknown++;}assert(unknown==1&&patch==1);}
 assert(ld_requested_rom(v,1,cached));assert(!ld_requested_rom(v,2,cached));
 swprintf(p,LD_PATH_CAP,L"%ls\\missing.md",base);assert(!ld_requested_rom(v,1,p));
 swprintf(p,LD_PATH_CAP,L"%ls\\unknown.bin",base);assert(!ld_requested_rom(v,1,p));
 /* Remove only files this fixture created. */
 swprintf(p,LD_PATH_CAP,L"%ls\\a.md",base);DeleteFileW(p);DeleteFileW(cached);
 swprintf(p,LD_PATH_CAP,L"%ls\\unknown.bin",base);DeleteFileW(p);swprintf(p,LD_PATH_CAP,L"%ls\\archive.zip",base);DeleteFileW(p);
 swprintf(p,LD_PATH_CAP,L"%ls\\patch.ips",mods);DeleteFileW(p);swprintf(p,LD_PATH_CAP,L"%ls\\invalid.bps",mods);DeleteFileW(p);DeleteFileW(cfg);RemoveDirectoryW(mods);RemoveDirectoryW(link);swprintf(p,LD_PATH_CAP,L"%ls\\second.68k",custom);DeleteFileW(p);RemoveDirectoryW(custom);RemoveDirectoryW(base);
 }
#endif
 free(v);puts("launcher discovery tests passed");return 0;
}



