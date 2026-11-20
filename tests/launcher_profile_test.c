#include "launcher_profile.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#define TEST_MKDIR(path) _mkdir(path)
#define TEST_RMDIR _rmdir
#define TEST_PID _getpid
#else
#include <sys/stat.h>
#include <unistd.h>
#define TEST_MKDIR(path) mkdir(path,0700)
#define TEST_RMDIR rmdir
#define TEST_PID getpid
#endif

static void join(char out[512],const char *directory,const char *suffix)
{
    size_t length=strlen(directory);
    assert(length+strlen(suffix)<512);
    memcpy(out,directory,length);
    strcpy(out+length,suffix);
}

int main(int argc,char **argv)
{
    assert(argc==2);
    char root[512],portable[512],fallback[512],override[512],selected[512],logs[512],path[512];
    assert(snprintf(root,sizeof root,"%s/profile-%lu",argv[1],(unsigned long)TEST_PID())>0);
    assert(TEST_MKDIR(root)==0);
    join(portable,root,"/portable");
    join(fallback,root,"/per-user");
    join(override,root,"/override/nested");
    assert(TEST_MKDIR(portable)==0);
    assert(dyna_profile_select(portable,NULL,fallback,selected,sizeof selected)==DYNA_PROFILE_PORTABLE);
    assert(!strcmp(selected,portable));
    assert(dyna_profile_select(portable,override,fallback,selected,sizeof selected)==DYNA_PROFILE_OVERRIDE);
    assert(!strcmp(selected,override));
    join(path,portable,"/settings.ini");
    assert(TEST_MKDIR(path)==0);
    assert(dyna_profile_select(portable,NULL,fallback,selected,sizeof selected)==DYNA_PROFILE_FALLBACK);
    assert(TEST_RMDIR(path)==0);
    assert(TEST_RMDIR(portable)==0);
    FILE *blocked=fopen(portable,"wb");
    assert(blocked && fclose(blocked)==0);
    assert(dyna_profile_select(portable,NULL,fallback,selected,sizeof selected)==DYNA_PROFILE_FALLBACK);
    assert(!strcmp(selected,fallback));
    assert(dyna_profile_select(fallback,portable,fallback,selected,sizeof selected)==DYNA_PROFILE_UNAVAILABLE);

    join(logs,fallback,"/logs");
    assert(TEST_MKDIR(logs)==0);
    join(path,logs,"/keep.log");
    FILE *unrelated=fopen(path,"wb");
    assert(unrelated && fclose(unrelated)==0);
    char history[DYNA_LOG_KEEP+2][512];
    for(int i=0;i<DYNA_LOG_KEEP+2;i++){
        FILE *log=dyna_log_open(fallback,"launcher",history[i],sizeof history[i]);
        assert(log);
        assert(fprintf(log,"run %d\n",i)>0 && fclose(log)==0);
    }
    assert(!fopen(history[0],"rb") && !fopen(history[1],"rb"));
    for(int i=2;i<DYNA_LOG_KEEP+2;i++)assert(remove(history[i])==0);
    assert(remove(path)==0);
    assert(TEST_RMDIR(logs)==0);
    assert(TEST_RMDIR(fallback)==0);
    assert(TEST_RMDIR(override)==0);
    join(path,root,"/override");
    assert(TEST_RMDIR(path)==0);
    assert(remove(portable)==0);
    assert(TEST_RMDIR(root)==0);
    puts("launcher profile and log rolling tests passed");
    return 0;
}
