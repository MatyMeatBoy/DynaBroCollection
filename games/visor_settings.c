#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
#include "visor_settings.h"
#include "../upstream/recomp-ui/src/recomp_launcher.h"
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
static int active,requested,db2,free_hud;
static void (*apply_free_hud)(int);
static const char *const free_hud_key="db2.free_hud";
static char path[2048];
static void (*apply)(int);
static RecompLauncherCModProvider base,provider;
static const char *key="db1.visor_offset_tiles";
enum { FILE_LIMIT=1024 };
static int parse(const char *s,int *out){
 if(!s||!*s)return 0;
 int sign=1,n=0;
 if(*s=='-'||*s=='+'){if(*s=='-')sign=-1;++s;}
 if(!*s)return 0;
 for(;*s;s++){
  if(*s<'0'||*s>'9')return 0;
  n=n*10+(*s-'0');if(n>64)return 0;
 }
 *out=sign*n;return 1;
}
#ifdef _WIN32
static int wide(const char *s,wchar_t out[2200]){
 return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,out,2200)>0;
}
static FILE *openfile(const char *s,int write){
 wchar_t w[2200];return wide(s,w)?_wfopen(w,write?L"wbx":L"rb"):NULL;
}
static void cleanup(const char *s){wchar_t w[2200];if(wide(s,w))DeleteFileW(w);}
static int replace(const char *from,const char *to){
 wchar_t a[2200],b[2200];return wide(from,a)&&wide(to,b)&&MoveFileExW(a,b,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
}
#else
static FILE *openfile(const char *s,int write){return fopen(s,write?"wbx":"rb");}
static void cleanup(const char *s){remove(s);}
static int replace(const char *from,const char *to){return rename(from,to)==0;}
#endif
/* Read raw bytes every save so edits to other keys are not overwritten.
 * A missing file is distinct from an unreadable or malformed existing file. */
static int readtext(char text[FILE_LIMIT+1],size_t *length){
 errno=0;FILE *f=openfile(path,0);
 if(!f){if(errno!=ENOENT)return 0;*length=0;text[0]=0;return 1;}
 size_t n=fread(text,1,FILE_LIMIT,f);
 int extra=fgetc(f),ok=!ferror(f);if(fclose(f))ok=0;
 if(!ok||extra!=EOF||memchr(text,0,n))return 0;
 text[n]=0;*length=n;return 1;
}
/* Only exact keys are owned. Prefix-neighbor and unknown lines are opaque. */
static int ownline(const char *key,const char *line,size_t length){
 size_t k=strlen(key);
 return length>=k&&!memcmp(line,key,k)&&
        (length==k||line[k]=='='||line[k]==' '||line[k]=='\t'||line[k]=='\r');
}
static int loadvalue(const char *key,int fallback,int minimum,int maximum){
 char text[FILE_LIMIT+1];size_t n;if(!readtext(text,&n))return fallback;
 int value=fallback,seen=0;
 for(size_t i=0;i<n;){
  size_t end=i;while(end<n&&text[end]!='\n')++end;
  if(ownline(key,text+i,end-i)){
   if(seen++)return fallback;
   size_t last=end;if(last>i&&text[last-1]=='\r')--last;
   text[last]=0;size_t k=strlen(key);
   if(last-i<=k||text[i+k]!='='||!parse(text+i+k+1,&value)||value<minimum||value>maximum)return fallback;
  }
  i=end<n?end+1:end;
 }
 return value;
}
void dyna_visor_settings_load(const char *settings,unsigned game){
 active=game==1||game==2;db2=game==2;free_hud=db2;key=db2?"db2.visor_offset_tiles":"db1.visor_offset_tiles";requested=0;path[0]=0;
 if(active&&settings&&*settings){
  const char *a=strrchr(settings,'/'),*b=strrchr(settings,'\\');
  const char *sep=!a?b:!b?a:a>b?a:b;
  size_t n=sep?(size_t)(sep+1-settings):0;
  const char name[]="dynabrothers-display.ini";
  if(n+sizeof name<=sizeof path){memcpy(path,settings,n);memcpy(path+n,name,sizeof name);requested=loadvalue(key,0,-64,64);free_hud=db2?loadvalue(free_hud_key,1,0,1):0;}
 }
 if(active&&apply)apply(requested);
 if(apply_free_hud)apply_free_hud(free_hud);
}
void dyna_visor_settings_bind(void (*setter)(int)){
 apply=setter;if(active&&apply)apply(requested);
}
void dyna_free_hud_bind(void (*setter)(int)){
 apply_free_hud=setter;if(active&&apply_free_hud)apply_free_hud(free_hud);
}
static int savevalue(const char *key,int value){
 if(!path[0])return 0;
 char original[FILE_LIMIT+1],result[FILE_LIMIT+1],replacement[64];size_t n,used=0;
 if(!readtext(original,&n))return 0;
 int replacement_length=snprintf(replacement,sizeof replacement,"%s=%d\n",key,value);
 if(replacement_length<=0||replacement_length>=(int)sizeof replacement)return 0;
 int inserted=0;
 for(size_t i=0;i<n;){
  size_t end=i;while(end<n&&original[end]!='\n')++end;
  size_t next=end<n?end+1:end;
  const char *bytes=original+i;size_t count=next-i;
  if(ownline(key,bytes,end-i)){
   if(inserted){i=next;continue;}
   bytes=replacement;count=(size_t)replacement_length;inserted=1;
  }
  if(count>FILE_LIMIT-used)return 0;
  memcpy(result+used,bytes,count);used+=count;i=next;
 }
 if(!inserted){
  if(used&&result[used-1]!='\n'){if(used==FILE_LIMIT)return 0;result[used++]='\n';}
  if((size_t)replacement_length>FILE_LIMIT-used)return 0;
  memcpy(result+used,replacement,(size_t)replacement_length);used+=(size_t)replacement_length;
 }
 char temp[2140];FILE *f=NULL;
#ifdef _WIN32
 unsigned long pid=(unsigned long)GetCurrentProcessId();
#else
 unsigned long pid=(unsigned long)getpid();
#endif
 for(unsigned i=0;i<100&&!f;i++){
  int n=snprintf(temp,sizeof temp,"%s.tmp.%lu.%u",path,pid,i);
  if(n<0||n>=(int)sizeof temp)return 0;
  f=openfile(temp,1);
 }
 if(!f)return 0;
 int ok=fwrite(result,1,used,f)==used;
 if(fflush(f))ok=0;
#ifdef _WIN32
 if(ok&&_commit(_fileno(f)))ok=0;
#else
 if(ok&&fsync(fileno(f)))ok=0;
#endif
 if(fclose(f))ok=0;
 if(ok)ok=replace(temp,path);
 if(!ok)cleanup(temp);
 return ok;
}
int dyna_visor_runtime_get(const char *k,int *value){
 if(db2&&k&&value&&!strcmp(k,free_hud_key)){*value=free_hud;return 1;}
 if(!active||!k||strcmp(k,key)||!value)return 0;
 *value=requested;return 1;
}
int dyna_visor_runtime_set(const char *k,int value){
 if(db2&&k&&!strcmp(k,free_hud_key)){
  if(value<0||value>1||!savevalue(free_hud_key,value))return 0;
  free_hud=value;if(apply_free_hud)apply_free_hud(value);return 1;
 }
 if(!active||!k||strcmp(k,key)||value< -64||value>64||!savevalue(key,value))return 0;
 requested=value;if(apply)apply(value);return 1;
}
static int owns(const char *p,const char *f){
 return active&&p&&!strcmp(p,"custom-widescreen")&&(!f||!strcmp(f,"widescreen"));
}
static int pget(void *ctx,int index,RecompLauncherCModPackage *out){
 int ok=base.package_get?base.package_get(ctx,index,out):0;
 if(ok&&out&&owns(out->id,NULL)&&out->option_count>=0&&out->option_count<=INT_MAX-2)out->option_count+=1+db2;
 return ok;
}
static int fget(void *ctx,int index,RecompLauncherCModFeature *out){
 int ok=base.feature_get?base.feature_get(ctx,index,out):0;
 if(ok&&out&&owns(out->package_id,out->id)&&out->option_count>=0&&out->option_count<=INT_MAX-2){
  out->option_count+=1+db2;
  snprintf(out->status,sizeof out->status,"Visor %d tiles (8 px each); border limits auto-clamp; no fractional pixels",requested);
 }
 return ok;
}
static int optioncount(void *ctx,int feature){
 int count=feature?(base.feature_count?base.feature_count(ctx):0):(base.package_count?base.package_count(ctx):0);
 for(int i=0;i<count;i++){
  if(feature){RecompLauncherCModFeature f;
   if(base.feature_get&&base.feature_get(ctx,i,&f)&&owns(f.package_id,f.id))return f.option_count;
  }else {RecompLauncherCModPackage p;
   if(base.package_get&&base.package_get(ctx,i,&p)&&owns(p.id,NULL))return p.option_count;
  }
 }
 return -1;
}
static int our_option(RecompLauncherCModOption *out){
 if(!out)return 0;
 memset(out,0,sizeof *out);strcpy(out->id,"visor_offset_tiles");strcpy(out->label,"Visor position (tiles)");
 strcpy(out->description,"Moves the visor in 8 px tiles. Border limits auto-clamp the visible position. No fractional pixels.");
 strcpy(out->group,"Widescreen");out->type=RECOMP_MOD_OPTION_INTEGER;out->min_value=-64;out->max_value=64;out->step=1;
 snprintf(out->value,sizeof out->value,"%d",requested);strcpy(out->default_value,"0");return 1;
}
static int free_hud_option(RecompLauncherCModOption *out){
 if(!out)return 0;
 memset(out,0,sizeof *out);strcpy(out->id,"free_hud");strcpy(out->label,"Free HUD");
 strcpy(out->description,"Show the battlefield behind the Enhanced HUD.");
 strcpy(out->group,"Widescreen");out->type=RECOMP_MOD_OPTION_BOOLEAN;
 strcpy(out->value,free_hud?"true":"false");strcpy(out->default_value,"true");return 1;
}
static int oget(void *ctx,const char *p,int index,RecompLauncherCModOption *out){
 int n=owns(p,NULL)?optioncount(ctx,0):-1;
 if(n>=0&&n<INT_MAX&&index==n)return our_option(out);
 if(db2&&n>=0&&n<INT_MAX&&index==n+1)return free_hud_option(out);
 return base.option_get?base.option_get(ctx,p,index,out):0;
}
static int foget(void *ctx,const char *p,const char *f,int index,RecompLauncherCModOption *out){
 int n=f&&owns(p,f)?optioncount(ctx,1):-1;
 if(n>=0&&n<INT_MAX&&index==n)return our_option(out);
 if(db2&&n>=0&&n<INT_MAX&&index==n+1)return free_hud_option(out);
 return base.feature_option_get?base.feature_option_get(ctx,p,f,index,out):0;
}
static int ownset(const char *value){int v;return parse(value,&v)&&dyna_visor_runtime_set(key,v);}
static int set_free_hud(const char *value){
 if(!value)return 0;
 if(!strcmp(value,"true"))return dyna_visor_runtime_set(free_hud_key,1);
 if(!strcmp(value,"false"))return dyna_visor_runtime_set(free_hud_key,0);
 return 0;
}
static int oset(void *ctx,const char *p,const char *option,const char *value){
 if(owns(p,NULL)&&option&&!strcmp(option,"visor_offset_tiles"))return ownset(value);
 if(db2&&owns(p,NULL)&&option&&!strcmp(option,"free_hud"))return set_free_hud(value);
 return base.set_option?base.set_option(ctx,p,option,value):0;
}
static int foset(void *ctx,const char *p,const char *f,const char *option,const char *value){
 if(f&&owns(p,f)&&option&&!strcmp(option,"visor_offset_tiles"))return ownset(value);
 if(db2&&f&&owns(p,f)&&option&&!strcmp(option,"free_hud"))return set_free_hud(value);
 return base.feature_set_option?base.feature_set_option(ctx,p,f,option,value):0;
}
const RecompLauncherCModProvider *dyna_visor_mods(const RecompLauncherCModProvider *source){
 if(!active||!source)return source;
 if(source!=&provider)base=*source;
 provider=base;provider.package_get=pget;provider.feature_get=fget;
 provider.option_get=oget;provider.feature_option_get=foget;
 provider.set_option=oset;provider.feature_set_option=foset;
 return &provider;
}
