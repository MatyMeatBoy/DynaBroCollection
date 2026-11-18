#include "../games/visor_settings.h"
#include "../upstream/recomp-ui/src/recomp_launcher.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PID GetCurrentProcessId()
#define MKDIR(p) _mkdir(p)
#define RMDIR(p) _rmdir(p)
#else
#include <unistd.h>
#include <sys/stat.h>
#define PID ((unsigned long)getpid())
#define MKDIR(p) mkdir(p,0700)
#define RMDIR(p) rmdir(p)
#endif
static int token,callbacks,last,forwarded;
static void apply(int v){callbacks++;last=v;}
static int count(void *ctx){assert(ctx==&token);return 1;}
static int package(void *ctx,int i,RecompLauncherCModPackage *o){assert(ctx==&token);if(i||!o)return 0;memset(o,0,sizeof *o);strcpy(o->id,"custom-widescreen");o->option_count=1;return 1;}
static int feature(void *ctx,int i,RecompLauncherCModFeature *o){assert(ctx==&token);if(i||!o)return 0;memset(o,0,sizeof *o);strcpy(o->package_id,"custom-widescreen");strcpy(o->id,"widescreen");o->option_count=1;return 1;}
static int option(void *ctx,const char *p,int i,RecompLauncherCModOption *o){assert(ctx==&token);assert(p&&o);forwarded++;memset(o,0,sizeof *o);strcpy(o->id,"base-option");return i==0;}
static int foption(void *ctx,const char *p,const char *f,int i,RecompLauncherCModOption *o){assert(f);return option(ctx,p,i,o);}
static int set(void *ctx,const char *p,const char *o,const char *v){assert(ctx==&token);assert(p&&o&&v);forwarded++;return 7;}
static int fset(void *ctx,const char *p,const char *f,const char *o,const char *v){assert(f);return set(ctx,p,o,v);}
static int commit(void *ctx,const char *p){assert(ctx==&token);(void)p;return 9;}
static void writefile(const char *p,const char *v){FILE *f=fopen(p,"wb");assert(f);assert(fwrite(v,1,strlen(v),f)==strlen(v));assert(!fclose(f));}
static void readfile(const char *p,char *b,size_t n){FILE *f=fopen(p,"rb");assert(f);size_t z=fread(b,1,n-1,f);b[z]=0;assert(!fclose(f));}
int main(void){
 char dir[128],settings[200],display[200],before[200];int value;
 snprintf(dir,sizeof dir,"visor-settings-test-%lu",(unsigned long)PID);assert(!MKDIR(dir));
 snprintf(settings,sizeof settings,"%s/settings.ini",dir);snprintf(display,sizeof display,"%s/dynabrothers-display.ini",dir);
 dyna_visor_settings_load(settings,1);dyna_visor_settings_bind(apply);
 assert(callbacks==1&&last==0);
 assert(dyna_visor_runtime_get("db1.visor_offset_tiles",&value)&&value==0);
 assert(!dyna_visor_runtime_get("bad",&value)&&!dyna_visor_runtime_get("db1.visor_offset_tiles",NULL));
 RecompLauncherCModProvider base={0};base.ctx=&token;base.package_count=count;base.package_get=package;base.feature_count=count;base.feature_get=feature;base.option_get=option;base.feature_option_get=foption;base.set_option=set;base.feature_set_option=fset;base.commit=commit;base.commit_worker_safe=1;
 const RecompLauncherCModProvider *p=dyna_visor_mods(&base);assert(p&&p!=&base&&p->ctx==base.ctx&&p->commit==commit&&p->commit_worker_safe==1);
 RecompLauncherCModPackage pkg;RecompLauncherCModFeature feat;RecompLauncherCModOption opt;
 assert(p->package_get(p->ctx,0,&pkg)&&pkg.option_count==2);
 assert(p->feature_get(p->ctx,0,&feat)&&feat.option_count==2&&strstr(feat.status,"8")&&strstr(feat.status,"clamp"));
 p=dyna_visor_mods(p);assert(p->package_get(p->ctx,0,&pkg)&&pkg.option_count==2);
 assert(p->commit(p->ctx,"test-image")==9);
 assert(p->feature_option_get(p->ctx,"custom-widescreen","widescreen",1,&opt));
 assert(!strcmp(opt.id,"visor_offset_tiles")&&opt.type==RECOMP_MOD_OPTION_INTEGER&&opt.min_value==-64&&opt.max_value==64&&opt.step==1&&!strcmp(opt.default_value,"0"));
 assert(p->option_get(p->ctx,"custom-widescreen",1,&opt)&&!strcmp(opt.id,"visor_offset_tiles"));
 assert(p->feature_option_get(p->ctx,"custom-widescreen","widescreen",0,&opt)&&!strcmp(opt.id,"base-option"));
 assert(p->feature_set_option(p->ctx,"other","other","x","v")==7);
 assert(p->set_option(p->ctx,"custom-widescreen","other","v")==7);
 assert(forwarded==3);
 assert(p->feature_set_option(p->ctx,"custom-widescreen","widescreen","visor_offset_tiles","+2"));
 assert(last==2&&callbacks==2);readfile(display,before,sizeof before);assert(!strcmp(before,"db1.visor_offset_tiles=2\n"));
 const char *invalid[]={"","+","-"," 1","1 ","1x","1.0","65","-65","9999999999999999999999999"};
 for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++)assert(!p->feature_set_option(p->ctx,"custom-widescreen","widescreen","visor_offset_tiles",invalid[i]));
 assert(callbacks==2&&last==2);
 assert(p->set_option(p->ctx,"custom-widescreen","visor_offset_tiles","-64")&&last==-64);
 assert(dyna_visor_runtime_set("db1.visor_offset_tiles",64)&&last==64);
 assert(!dyna_visor_runtime_set("db1.visor_offset_tiles",65));
 readfile(display,before,sizeof before);
#ifdef _WIN32
 char after[200];
 HANDLE lock=CreateFileA(display,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);assert(lock!=INVALID_HANDLE_VALUE);
 int previous_callbacks=callbacks;assert(!dyna_visor_runtime_set("db1.visor_offset_tiles",3));
 assert(last==64&&callbacks==previous_callbacks);assert(CloseHandle(lock));
 readfile(display,after,sizeof after);assert(!strcmp(before,after));
#endif
 assert(dyna_visor_runtime_get("db1.visor_offset_tiles",&value)&&value==64);
 dyna_visor_settings_load(settings,1);assert(last==64);
 dyna_visor_settings_bind(NULL);int hub_callbacks=callbacks;
 assert(dyna_visor_runtime_set("db1.visor_offset_tiles",-2)&&callbacks==hub_callbacks);
 dyna_visor_settings_bind(apply);assert(callbacks==hub_callbacks+1&&last==-2);
 writefile(display,"db1.visor_offset_tiles=4junk\n");dyna_visor_settings_load(settings,1);assert(last==0);
 writefile(display,"db1.visor_offset_tiles=+1\r\n");dyna_visor_settings_load(settings,1);assert(last==1);
 writefile(display,"db1.visor_offset_tiles=1\ndb1.visor_offset_tiles=2\n");dyna_visor_settings_load(settings,1);assert(last==0);
#ifdef _WIN32
 char utf8dir[200],utf8settings[240],utf8display[240];wchar_t wdir[240],wdisplay[240];
 snprintf(utf8dir,sizeof utf8dir,"%s/visi\xc3\xb3n",dir);
 snprintf(utf8settings,sizeof utf8settings,"%s/settings.ini",utf8dir);
 snprintf(utf8display,sizeof utf8display,"%s/dynabrothers-display.ini",utf8dir);
 assert(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8dir,-1,wdir,240));
 assert(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8display,-1,wdisplay,240));
 assert(CreateDirectoryW(wdir,NULL));dyna_visor_settings_load(utf8settings,1);
 assert(dyna_visor_runtime_set("db1.visor_offset_tiles",-3)&&last==-3);
 dyna_visor_settings_load(utf8settings,1);assert(last==-3);
 assert(DeleteFileW(wdisplay)&&RemoveDirectoryW(wdir));
#endif
 int prior=callbacks;dyna_visor_settings_load(settings,2);assert(callbacks==prior&&dyna_visor_mods(&base)==&base);
 assert(!dyna_visor_runtime_get("db1.visor_offset_tiles",&value)&&!dyna_visor_runtime_set("db1.visor_offset_tiles",1));
 dyna_visor_settings_bind(NULL);assert(!remove(display)&&!RMDIR(dir));
 puts("Visor strict settings, provider forwarding, persistence and failure rollback passed");
}
