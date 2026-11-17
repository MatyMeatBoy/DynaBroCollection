#include "language_settings.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

static int requested=1,active=1,started;
static char path[1024],error[160];
void dyna_language_settings_load(const char *settings_path) {
    requested=1;
    snprintf(path,sizeof path,"%s",settings_path?settings_path:"");
    char *a=strrchr(path,'/'),*b=strrchr(path,'\\');
    char *sep=!a?b:!b?a:a>b?a:b;
    size_t offset=sep?(size_t)(sep+1-path):0;
    if(snprintf(path+offset,sizeof path-offset,"dynabrothers-language.ini")>=(int)(sizeof path-offset)) {
        path[0]=0;snprintf(error,sizeof error,"Language settings path is too long");return;
    }
    FILE *file=fopen(path,"r");
    if(file) {
        char line[100];int value;
        while(fgets(line,sizeof line,file))
            if(sscanf(line,"english=%d",&value)==1&&(value==0||value==1))requested=value;
        fclose(file);
    }
}
int dyna_language_english(void){return requested;}
void dyna_language_started(void){active=requested;started=1;}

#if RECOMP_LAUNCHER
#include "recomp_launcher.h"
static const RecompLauncherCModProvider *base;
#define COPY(dst,src) snprintf(dst,sizeof(dst),"%s",src)
static int owns(const char *p){return p&&!strcmp(p,"dyna-language");}
static int count(int (*fn)(void *)){return fn?fn(base->ctx):0;}
static int package_count(void *ctx){(void)ctx;return count(base->package_count)+1;}
static int feature_count(void *ctx){(void)ctx;return count(base->feature_count)+1;}
static int package_get(void *ctx,int index,RecompLauncherCModPackage *out) {
    (void)ctx;
    if(index<count(base->package_count))return base->package_get(base->ctx,index,out);
    if(!out||index!=count(base->package_count))return 0;
    memset(out,0,sizeof *out);COPY(out->id,"dyna-language");COPY(out->name,"English translation");
    COPY(out->version,"Development");COPY(out->author,"Dyna Brothers translation contributors");
    COPY(out->description,"Partial English translation. The Japanese original remains available. Changes apply when starting a new session.");
    out->enabled=requested;return 1;
}
static int feature_get(void *ctx,int index,RecompLauncherCModFeature *out) {
    (void)ctx;
    if(index<count(base->feature_count))return base->feature_get(base->ctx,index,out);
    if(!out||index!=count(base->feature_count))return 0;
    memset(out,0,sizeof *out);COPY(out->id,"english");COPY(out->package_id,"dyna-language");
    COPY(out->package_name,"Game language");COPY(out->package_version,"Development");
    COPY(out->name,"English translation");COPY(out->group,"Language");
    COPY(out->author,"Dyna Brothers translation contributors");
    COPY(out->description,"Partial translation, enabled by default. Disable for Japanese. Applies at the next game start; restart the application after an in-game change.");
    out->enabled=requested;
    COPY(out->status,started&&requested!=active?"Restart required":requested?"English (partial)":"Japanese original");
    return 1;
}
static int feature_enable(void *ctx,const char *p,const char *f,int enabled) {
    (void)ctx;
    if(owns(p)&&f&&!strcmp(f,"english")){requested=!!enabled;return 1;}
    return base->feature_enable?base->feature_enable(base->ctx,p,f,enabled):0;
}
static int set_enabled(void *ctx,const char *p,int enabled) {
    (void)ctx;if(owns(p)){requested=!!enabled;return 1;}
    return base->set_enabled?base->set_enabled(base->ctx,p,enabled):0;
}
static int diagnostic_count(void *ctx,const char *p,const char *f) {
    (void)ctx;if(owns(p))return 0;
    return base->diagnostic_count?base->diagnostic_count(base->ctx,p,f):0;
}
static int save_requested(void) {
    error[0]=0;
    char temporary[1060];snprintf(temporary,sizeof temporary,"%s.tmp",path);
    if(!path[0]){COPY(error,"Language settings path is unavailable");return 0;}
    FILE *file=fopen(temporary,"w");
    if(!file){COPY(error,"Unable to write language settings");return 0;}
    int ok=fprintf(file,"english=%d\n",requested)>0;
    if(ferror(file))ok=0;
    if(fclose(file))ok=0;
#ifdef _WIN32
    if(ok)ok=MoveFileExA(temporary,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    if(ok)ok=rename(temporary,path)==0;
#endif
    if(!ok){remove(temporary);COPY(error,"Unable to save language settings");}
    return ok;
}
int dyna_language_runtime_get(const char *key,int *value) {
    if(!key||strcmp(key,"dyna.english")||!value)return 0;
    *value=requested;return 1;
}
int dyna_language_runtime_set(const char *key,int value) {
    if(!key||strcmp(key,"dyna.english")||(value!=0&&value!=1))return 0;
    int previous=requested;requested=value;
    if(save_requested())return 1;
    requested=previous;return 0;
}
static int commit(void *ctx,const char *image_path) {
    (void)ctx;error[0]=0;
    if(base->commit&&!base->commit(base->ctx,image_path))return 0;
    return save_requested();
}
static const char *last_error(void *ctx) {
    (void)ctx;return error[0]?error:base->last_error?base->last_error(base->ctx):"Unable to save settings";
}
const RecompLauncherCModProvider *dyna_language_mods(const RecompLauncherCModProvider *source) {
    static RecompLauncherCModProvider provider;
    if(!source)return NULL;
    base=source;provider=*source;
    provider.package_count=package_count;provider.package_get=package_get;
    provider.feature_count=feature_count;provider.feature_get=feature_get;
    provider.feature_enable=feature_enable;provider.set_enabled=set_enabled;
    provider.diagnostic_count=diagnostic_count;provider.commit=commit;provider.last_error=last_error;
    /* This provider has its own small persistence transaction. Keep the base
     * provider's prepared-commit shortcut from bypassing that transaction. */
    provider.commit_worker_safe=0;provider.try_commit=NULL;
    return &provider;
}
#else
int dyna_language_runtime_get(const char *key,int *value){(void)key;(void)value;return 0;}
int dyna_language_runtime_set(const char *key,int value){(void)key;(void)value;return 0;}
const struct RecompLauncherCModProvider *dyna_language_mods(const struct RecompLauncherCModProvider *source){return source;}
#endif
