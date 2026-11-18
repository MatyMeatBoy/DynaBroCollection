#include "progress_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
static const char hash[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
int main(int argc,char **argv) {
    const char *path=argc>1?argv[1]:"progress-test.bin";
    DynaProgressStore s,loaded,before;DynaProgressCode c={0};
    assert(!dyna_progress_init(&s,1,"bad"));assert(dyna_progress_init(&s,1,hash));
    c.mode=2;c.stage=41;c.length=4;c.source_pc=0x1bee0;
    c.code[0]=2;c.code[1]=6;c.code[2]=10;c.code[3]=14;
    assert(dyna_progress_add(&s,&c));assert(dyna_progress_add(&s,&c));assert(s.count==1);
    assert(dyna_progress_save(&s,path));assert(dyna_progress_init(&loaded,1,hash));
    assert(dyna_progress_load(&loaded,path));assert(!memcmp(&s,&loaded,sizeof s));
    assert(dyna_progress_init(&loaded,2,hash));before=loaded;
    assert(!dyna_progress_load(&loaded,path));assert(!memcmp(&before,&loaded,sizeof before));
    assert(dyna_progress_init(&loaded,1,hash));
    loaded.rom_sha256[0]='f';before=loaded;
    assert(!dyna_progress_load(&loaded,path));assert(!memcmp(&before,&loaded,sizeof before));
#ifdef _WIN32
    HANDLE lock=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    assert(lock!=INVALID_HANDLE_VALUE);
    c.stage=42;assert(dyna_progress_add(&s,&c));
    assert(!dyna_progress_save(&s,path));CloseHandle(lock);
    assert(dyna_progress_init(&loaded,1,hash));assert(dyna_progress_load(&loaded,path));
    assert(loaded.count==1); /* Failed replacement preserved prior valid data. */
    assert(dyna_progress_save(&s,path));assert(dyna_progress_load(&loaded,path));
    assert(loaded.count==2);
#endif
    before=s;c.length=9;assert(!dyna_progress_add(&s,&c));assert(!memcmp(&before,&s,sizeof s));
    s.codes[0].length=9;assert(!dyna_progress_save(&s,path));
    assert(dyna_progress_init(&loaded,1,hash));assert(dyna_progress_load(&loaded,path));
    FILE *f=fopen(path,"r+b");assert(f);assert(fseek(f,90,SEEK_SET)==0);assert(fputc(255,f)!=EOF);assert(fclose(f)==0);
    before=loaded;assert(!dyna_progress_load(&loaded,path));assert(!memcmp(&before,&loaded,sizeof before));
    assert(remove(path)==0);puts("Progress identity, corruption, atomic replacement and failure preservation passed");
    return 0;
}
