#include "progress_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
/* Public paths are UTF-8, including Windows profiles outside the ANSI codepage. */
#ifdef _WIN32
static wchar_t *wide_path(const char *path) {
    if(!path||!*path||strlen(path)>30064)return NULL;
    int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,NULL,0);
    if(!count)return NULL;
    wchar_t *wide=malloc((size_t)count*sizeof *wide);
    if(!wide)return NULL;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path,-1,wide,count)){
        free(wide);return NULL;
    }
    return wide;
}
static FILE *open_file(const char *path,int writing) {
    wchar_t *wide=wide_path(path);if(!wide)return NULL;
    FILE *f=_wfopen(wide,writing?L"wbx":L"rb");free(wide);return f;
}
static int replace_file(const char *from,const char *to) {
    wchar_t *a=wide_path(from),*b=wide_path(to);
    int ok=a&&b&&MoveFileExW(a,b,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
    free(a);free(b);return ok;
}
static void remove_file(const char *path) {
    wchar_t *wide=wide_path(path);if(wide)DeleteFileW(wide);free(wide);
}
int dyna_progress_file_exists(const char *path) {
    wchar_t *wide=wide_path(path);if(!wide)return 1;
    DWORD attrs=GetFileAttributesW(wide),error=GetLastError();free(wide);
    /* Unknown/access errors fail closed: never silently replace unreadable saves. */
    return attrs!=INVALID_FILE_ATTRIBUTES ||
           (error!=ERROR_FILE_NOT_FOUND && error!=ERROR_PATH_NOT_FOUND);
}
#else
#include <errno.h>
#include <sys/stat.h>
static FILE *open_file(const char *path,int writing){return fopen(path,writing?"wbx":"rb");}
static int replace_file(const char *from,const char *to){return rename(from,to)==0;}
static void remove_file(const char *path){remove(path);}
int dyna_progress_file_exists(const char *path){
    struct stat info;if(!path||!*path)return 1;
    return stat(path,&info)==0 || (errno!=ENOENT && errno!=ENOTDIR);
}
#endif
#define HEADER 88u
#define RECORD 17u
static int hexhash(const char *s) {
    if(!s||strlen(s)!=64)return 0;
    for(unsigned i=0;i<64;i++)if(!((s[i]>='0'&&s[i]<='9')||
        (s[i]>='a'&&s[i]<='f')||(s[i]>='A'&&s[i]<='F')))return 0;
    return 1;
}
int dyna_progress_init(DynaProgressStore *s,unsigned game,const char *sha) {
    if(!s||(game!=1&&game!=2)||!hexhash(sha))return 0;
    char identity[65];memcpy(identity,sha,65);
    memset(s,0,sizeof *s);s->game=game;
    for(unsigned i=0;i<64;i++)s->rom_sha256[i]=(identity[i]>='A'&&identity[i]<='F')?
        (char)(identity[i]+('a'-'A')):identity[i];
    return 1;
}
static int valid(const DynaProgressStore *s) {
    return s&&(s->game==1||s->game==2)&&s->count<=DYNA_PROGRESS_LIMIT&&
        s->rom_sha256[64]==0&&hexhash(s->rom_sha256);
}
static int codevalid(const DynaProgressCode *c) {
    return c&&c->length>0&&c->length<=sizeof c->code&&c->source_pc<=0xffffffu;
}
int dyna_progress_add(DynaProgressStore *s,const DynaProgressCode *c) {
    if(!valid(s)||!codevalid(c))return 0;
    for(unsigned i=0;i<s->count;i++)if(s->codes[i].mode==c->mode&&
        s->codes[i].stage==c->stage&&s->codes[i].length==c->length&&
        !memcmp(s->codes[i].code,c->code,c->length))return 1;
    if(s->count==DYNA_PROGRESS_LIMIT)return 0;
    DynaProgressCode *out=&s->codes[s->count++];memset(out,0,sizeof *out);
    out->mode=c->mode;out->stage=c->stage;out->length=c->length;
    memcpy(out->code,c->code,c->length);out->source_pc=c->source_pc;return 1;
}
static void put(uint8_t *p,uint32_t n) {
    p[0]=(uint8_t)(n>>24);p[1]=(uint8_t)(n>>16);p[2]=(uint8_t)(n>>8);p[3]=(uint8_t)n;
}
static uint32_t get(const uint8_t *p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static uint32_t checksum(const uint8_t *p,size_t n) {
    uint32_t h=2166136261u;
    for(size_t i=0;i<n;i++){h^=p[i];h*=16777619u;}return h;
}
static size_t encode(const DynaProgressStore *s,uint8_t *p) {
    if(!valid(s))return 0;
    memset(p,0,HEADER+RECORD*s->count);memcpy(p,"DYNAPRG1",8);
    put(p+8,1);put(p+12,s->game);memcpy(p+16,s->rom_sha256,64);put(p+80,s->count);
    for(unsigned i=0;i<s->count;i++) {
        const DynaProgressCode *c=&s->codes[i];if(!codevalid(c))return 0;
        uint8_t *q=p+HEADER+RECORD*i;
        q[0]=(uint8_t)(c->mode>>8);q[1]=(uint8_t)c->mode;
        q[2]=(uint8_t)(c->stage>>8);q[3]=(uint8_t)c->stage;q[4]=c->length;
        memcpy(q+5,c->code,c->length);put(q+13,c->source_pc);
    }
    size_t n=HEADER+RECORD*s->count;
    /* Include identity/header as well as payload; checksum slot is zero. */
    put(p+84,checksum(p,n));return n;
}
int dyna_progress_load(DynaProgressStore *s,const char *path) {
    if(!valid(s)||!path)return 0;
    FILE *f=open_file(path,0);if(!f)return 0;
    uint8_t bytes[HEADER+RECORD*DYNA_PROGRESS_LIMIT];
    size_t n=fread(bytes,1,sizeof bytes,f);int extra=fgetc(f),ok=!ferror(f);
    if(fclose(f))ok=0;
    if(!ok||extra!=EOF||n<HEADER||memcmp(bytes,"DYNAPRG1",8)||get(bytes+8)!=1||
       get(bytes+12)!=s->game||memcmp(bytes+16,s->rom_sha256,64))return 0;
    unsigned count=get(bytes+80);
    if(count>DYNA_PROGRESS_LIMIT||n!=HEADER+RECORD*count)return 0;
    uint32_t saved=get(bytes+84);put(bytes+84,0);
    if(saved!=checksum(bytes,n))return 0;
    DynaProgressStore copy;
    if(!dyna_progress_init(&copy,s->game,s->rom_sha256))return 0;
    for(unsigned i=0;i<count;i++) {
        const uint8_t *q=bytes+HEADER+RECORD*i;DynaProgressCode c={0};
        c.mode=(uint16_t)((q[0]<<8)|q[1]);c.stage=(uint16_t)((q[2]<<8)|q[3]);
        c.length=q[4];memcpy(c.code,q+5,8);c.source_pc=get(q+13);
        if(!dyna_progress_add(&copy,&c))return 0;
    }
    if(copy.count!=count)return 0;
    *s=copy;return 1;
}
int dyna_progress_save(const DynaProgressStore *s,const char *path) {
    uint8_t bytes[HEADER+RECORD*DYNA_PROGRESS_LIMIT];size_t n=encode(s,bytes);
    if(!n||!path||!*path)return 0;
    size_t plen=strlen(path);if(plen>30000)return 0;
    char *temp=(char *)malloc(plen+64);if(!temp)return 0;
    FILE *f=NULL;
    for(unsigned attempt=0;attempt<100&&!f;attempt++) {
#ifdef _WIN32
        snprintf(temp,plen+64,"%s.tmp.%lu.%u",path,(unsigned long)GetCurrentProcessId(),attempt);
#else
        snprintf(temp,plen+64,"%s.tmp.%lu.%u",path,(unsigned long)getpid(),attempt);
#endif
        f=open_file(temp,1);
    }
    if(!f){free(temp);return 0;}
    int ok=fwrite(bytes,1,n,f)==n;
    if(ok&&fflush(f))ok=0;
#ifdef _WIN32
    if(ok&&_commit(_fileno(f)))ok=0;
#else
    if(ok&&fsync(fileno(f)))ok=0;
#endif
    if(fclose(f))ok=0;
    if(ok) {
        ok=replace_file(temp,path);
    }
    if(!ok)remove_file(temp);
    free(temp);return ok;
}
