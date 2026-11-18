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
 dyna_visor_settings_load(utf8settings,2);assert(last==0);
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",4)&&last==4);
 dyna_visor_settings_load(utf8settings,1);assert(last==-3);
 dyna_visor_settings_load(utf8settings,2);assert(last==4);
 assert(DeleteFileW(wdisplay)&&RemoveDirectoryW(wdir));
#endif
 /* Both games share one file, but own only their selected key. */
 writefile(display,"# unknown\r\ndb1.visor_offset_tiles=+7\r\ndb2.visor_offset_tiles=-3\nextra=unchanged");
 dyna_visor_settings_load(settings,2);assert(last==-3);
 assert(dyna_visor_runtime_get("db2.visor_offset_tiles",&value)&&value==-3);
 assert(dyna_visor_mods(&base)!=&base);
 assert(!dyna_visor_runtime_set("db1.visor_offset_tiles",4));
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",2)&&last==2);
 char combined[1200];readfile(display,combined,sizeof combined);
#ifdef _WIN32
 char original_both[1200];strcpy(original_both,combined);
 int callbacks_before_lock=callbacks;
 HANDLE both_lock=CreateFileA(display,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
 assert(both_lock!=INVALID_HANDLE_VALUE);
 assert(!dyna_visor_runtime_set("db2.visor_offset_tiles",-8)&&callbacks==callbacks_before_lock&&last==2);
 assert(CloseHandle(both_lock));readfile(display,combined,sizeof combined);assert(!strcmp(combined,original_both));
 both_lock=CreateFileA(display,GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);assert(both_lock!=INVALID_HANDLE_VALUE);
 assert(!dyna_visor_runtime_set("db2.visor_offset_tiles",-8)&&callbacks==callbacks_before_lock&&last==2);
 assert(CloseHandle(both_lock));readfile(display,combined,sizeof combined);assert(!strcmp(combined,original_both));
#endif
 assert(!strcmp(combined,"# unknown\r\ndb1.visor_offset_tiles=+7\r\ndb2.visor_offset_tiles=2\nextra=unchanged"));
 dyna_visor_settings_load(settings,1);assert(last==7);
 /* External edits made after load survive the immediate set. */
 writefile(display,"# external\ndb2.visor_offset_tiles=bad\r\ndb1.visor_offset_tiles=4junk\nopaque=\xff\r\n");
 assert(dyna_visor_runtime_set("db1.visor_offset_tiles",-5));
 readfile(display,combined,sizeof combined);
 assert(!strcmp(combined,"# external\ndb2.visor_offset_tiles=bad\r\ndb1.visor_offset_tiles=-5\nopaque=\xff\r\n"));
 dyna_visor_settings_load(settings,2);assert(last==0);
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",6));
 dyna_visor_settings_load(settings,1);assert(last==-5);
 writefile(display,"db1.visor_offset_tiles=bad\r\ndb2.visor_offset_tiles=8\ndb1.visor_offset_tiles=2\nlast");
 dyna_visor_settings_load(settings,1);assert(last==0);
 assert(dyna_visor_runtime_set("db1.visor_offset_tiles",3));
 readfile(display,combined,sizeof combined);
 assert(!strcmp(combined,"db1.visor_offset_tiles=3\ndb2.visor_offset_tiles=8\nlast"));
 dyna_visor_settings_load(settings,2);assert(last==8);
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",-64));
 dyna_visor_settings_load(settings,2);assert(last==-64);
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",64));
 /* Missing own key appends after a separator without changing old bytes. */
 writefile(display,"other=literal");
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",1));
 readfile(display,combined,sizeof combined);assert(!strcmp(combined,"other=literal\ndb2.visor_offset_tiles=1\n"));
 writefile(display,"db2.visor_offset_tiles =bad\ndb2.visor_offset_tiles_extra=literal\n");
 dyna_visor_settings_load(settings,2);assert(last==0);
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",1));
 readfile(display,combined,sizeof combined);
 assert(!strcmp(combined,"db2.visor_offset_tiles=1\ndb2.visor_offset_tiles_extra=literal\n"));
 /* Existing NUL, >1024 bytes, and unavailable reads fail closed on save. */
 char badbytes[1026];memset(badbytes,'x',sizeof badbytes);
 for(unsigned kind=0;kind<3;kind++){
  size_t n=kind==0?8:kind==1?1025:1024;
  memset(badbytes,'x',sizeof badbytes);if(!kind)badbytes[3]=0;
  FILE *f=fopen(display,"wb");assert(f&&fwrite(badbytes,1,n,f)==n&&!fclose(f));
  int prior_callbacks=callbacks;
  assert(!dyna_visor_runtime_set("db2.visor_offset_tiles",2));
  assert(callbacks==prior_callbacks&&last==1);
  f=fopen(display,"rb");assert(f);char afterbytes[1026];assert(fread(afterbytes,1,sizeof afterbytes,f)==n&&!fclose(f));
  assert(!memcmp(afterbytes,badbytes,n));
 }
 /* Exactly1024 bytes is readable when replacing its own line keeps bounds. */
 const char own[]="db2.visor_offset_tiles=1\n";
 memcpy(badbytes,own,sizeof own-1);memset(badbytes+sizeof own-1,'z',1024-(sizeof own-1));
 FILE *bounded_file=fopen(display,"wb");assert(bounded_file&&fwrite(badbytes,1,1024,bounded_file)==1024&&!fclose(bounded_file));
 assert(dyna_visor_runtime_set("db2.visor_offset_tiles",2));
 readfile(display,combined,sizeof combined);assert(strlen(combined)==1024&&!memcmp(combined+sizeof own-1,badbytes+sizeof own-1,1024-(sizeof own-1)));
 int prior=callbacks;dyna_visor_settings_load(settings,3);assert(callbacks==prior&&dyna_visor_mods(&base)==&base);
 assert(!dyna_visor_runtime_get("db1.visor_offset_tiles",&value)&&!dyna_visor_runtime_set("db1.visor_offset_tiles",1));
 dyna_visor_settings_bind(NULL);assert(!remove(display)&&!RMDIR(dir));
 puts("Both-game visor strict settings, byte-preserving shared persistence, provider forwarding and rollback passed");
}
