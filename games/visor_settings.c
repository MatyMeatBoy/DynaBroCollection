#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif
#include "visor_settings.h"
#include "../upstream/recomp-ui/src/recomp_launcher.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
static int active,requested;
static char path[2048];
static void (*apply)(int);
static RecompLauncherCModProvider base,provider;
static const char key[]="db1.visor_offset_tiles";
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
static int loadvalue(void){
 FILE *f=openfile(path,0);if(!f)return 0;
 char text[1025];size_t n=fread(text,1,sizeof text-1,f);
 int extra=fgetc(f),ok=!ferror(f);if(fclose(f))ok=0;
 if(!ok||extra!=EOF||memchr(text,0,n))return 0;
 text[n]=0;int value=0,seen=0;
 char *line=text;
 while(*line){
  char *next=strchr(line,'\n');if(next)*next++=0;
  size_t length=strlen(line);if(length&&line[length-1]=='\r')line[length-1]=0;
  if(!strncmp(line,key,sizeof key-1)&&line[sizeof key-1]=='='){
   if(seen++||!parse(line+sizeof key,&value))return 0;
  }
  if(!next)break;
  line=next;
 }
 return value;
}
void dyna_visor_settings_load(const char *settings,unsigned game){
 active=game==1;requested=0;path[0]=0;
 if(active&&settings&&*settings){
  const char *a=strrchr(settings,'/'),*b=strrchr(settings,'\\');
  const char *sep=!a?b:!b?a:a>b?a:b;
  size_t n=sep?(size_t)(sep+1-settings):0;
  const char name[]="dynabrothers-display.ini";
  if(n+sizeof name<=sizeof path){memcpy(path,settings,n);memcpy(path+n,name,sizeof name);requested=loadvalue();}
 }
 if(active&&apply)apply(requested);
}
void dyna_visor_settings_bind(void (*setter)(int)){
 apply=setter;if(active&&apply)apply(requested);
}
static int savevalue(int value){
 if(!path[0])return 0;
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
 int ok=fprintf(f,"%s=%d\n",key,value)>0;
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
 if(!active||!k||strcmp(k,key)||!value)return 0;
 *value=requested;return 1;
}
int dyna_visor_runtime_set(const char *k,int value){
 if(!active||!k||strcmp(k,key)||value< -64||value>64||!savevalue(value))return 0;
 requested=value;if(apply)apply(value);return 1;
}
static int owns(const char *p,const char *f){
 return active&&p&&!strcmp(p,"custom-widescreen")&&(!f||!strcmp(f,"widescreen"));
}
static int pget(void *ctx,int index,RecompLauncherCModPackage *out){
 int ok=base.package_get?base.package_get(ctx,index,out):0;
 if(ok&&out&&owns(out->id,NULL)&&out->option_count>=0&&out->option_count<INT_MAX)++out->option_count;
 return ok;
}
static int fget(void *ctx,int index,RecompLauncherCModFeature *out){
 int ok=base.feature_get?base.feature_get(ctx,index,out):0;
 if(ok&&out&&owns(out->package_id,out->id)&&out->option_count>=0&&out->option_count<INT_MAX){
  ++out->option_count;
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
static int oget(void *ctx,const char *p,int index,RecompLauncherCModOption *out){
 int n=owns(p,NULL)?optioncount(ctx,0):-1;
 if(n>=0&&n<INT_MAX&&index==n)return our_option(out);
 return base.option_get?base.option_get(ctx,p,index,out):0;
}
static int foget(void *ctx,const char *p,const char *f,int index,RecompLauncherCModOption *out){
 int n=f&&owns(p,f)?optioncount(ctx,1):-1;
 if(n>=0&&n<INT_MAX&&index==n)return our_option(out);
 return base.feature_option_get?base.feature_option_get(ctx,p,f,index,out):0;
}
static int ownset(const char *value){int v;return parse(value,&v)&&dyna_visor_runtime_set(key,v);}
static int oset(void *ctx,const char *p,const char *option,const char *value){
 if(owns(p,NULL)&&option&&!strcmp(option,"visor_offset_tiles"))return ownset(value);
 return base.set_option?base.set_option(ctx,p,option,value):0;
}
static int foset(void *ctx,const char *p,const char *f,const char *option,const char *value){
 if(f&&owns(p,f)&&option&&!strcmp(option,"visor_offset_tiles"))return ownset(value);
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
