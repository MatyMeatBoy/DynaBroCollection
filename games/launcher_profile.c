#define _POSIX_C_SOURCE 200809L
#include "launcher_profile.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#define make_directory(path) _mkdir(path)
#define check_access _access
#define open_new(path) _open(path,_O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY,_S_IREAD|_S_IWRITE)
#define close_file _close
#define file_stream _fdopen
#define process_id _getpid
#define READ_WRITE_ACCESS 6
#else
#include <dirent.h>
#include <unistd.h>
#define make_directory(path) mkdir(path,0700)
#define check_access access
#define open_new(path) open(path,O_WRONLY|O_CREAT|O_EXCL,0600)
#define close_file close
#define file_stream fdopen
#define process_id getpid
#define READ_WRITE_ACCESS (R_OK|W_OK)
#endif

enum { FILE_PATH_CAP = 512, LOG_NAME_CAP = 96 };

static int path_separator(char c)
{
#ifdef _WIN32
    return c=='/' || c=='\\';
#else
    return c=='/';
#endif
}

static int absolute_path(const char *path)
{
    if(!path || !*path)return 0;
#ifdef _WIN32
    return (path_separator(path[0]) && path_separator(path[1])) ||
           (isalpha((unsigned char)path[0]) && path[1]==':' && path_separator(path[2]));
#else
    return path[0]=='/';
#endif
}

static int append_path(char *out,size_t capacity,const char *dir,const char *name)
{
    if(!dir || !*dir)return 0;
    const char *separator=path_separator(dir[strlen(dir)-1])?"":"/";
    int n=snprintf(out,capacity,"%s%s%s",dir,separator,name);
    if(n>=0 && (size_t)n<capacity)return 1;
    if(capacity)out[0]=0;
    return 0;
}

static int is_directory(const char *path)
{
#ifdef _WIN32
    DWORD attrs=GetFileAttributesA(path);
    return attrs!=INVALID_FILE_ATTRIBUTES && (attrs&FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat info;
    return stat(path,&info)==0 && S_ISDIR(info.st_mode);
#endif
}

static int ensure_directory(const char *path)
{
    if(is_directory(path))return 1;
    if(make_directory(path)==0)return 1;
    if(errno!=ENOENT)return is_directory(path);
    char parent[FILE_PATH_CAP];
    size_t length=strlen(path);
    if(length>=sizeof parent)return 0;
    while(length && path_separator(path[length-1]))length--;
    while(length && !path_separator(path[length-1]))length--;
    if(!length || length==strlen(path))return 0;
    memcpy(parent,path,length);parent[length]=0;
    if(!ensure_directory(parent))return 0;
    return make_directory(path)==0 || is_directory(path);
}

static unsigned long long log_time(void)
{
#ifdef _WIN32
    FILETIME now;
    GetSystemTimePreciseAsFileTime(&now);
    return ((unsigned long long)now.dwHighDateTime<<32)|now.dwLowDateTime;
#else
    struct timespec now;
    if(clock_gettime(CLOCK_REALTIME,&now)!=0)return (unsigned long long)time(NULL)*10000000;
    return (unsigned long long)now.tv_sec*10000000+(unsigned long long)now.tv_nsec/100;
#endif
}

static int create_unique(const char *dir,const char *prefix,const char *suffix,
                         char *path,size_t capacity)
{
    const unsigned long long now=log_time();
    for(unsigned collision=0;collision<100;collision++){
        char name[LOG_NAME_CAP];
        int n=snprintf(name,sizeof name,"%s-%020llu-%010lu-%02u%s",prefix,now,
                       (unsigned long)process_id(),collision,suffix);
        if(n<0 || (size_t)n>=sizeof name || !append_path(path,capacity,dir,name))return -1;
        int handle=open_new(path);
        if(handle>=0 || errno!=EEXIST)return handle;
    }
    return -1;
}

static int usable_profile(const char *path,int create,char *out,size_t capacity)
{
    if(!absolute_path(path) || strlen(path)>=capacity)return 0;
    if(!(create?ensure_directory(path):is_directory(path)))return 0;
#ifdef _WIN32
    if(check_access(path,READ_WRITE_ACCESS)!=0)return 0;
#else
    if(check_access(path,R_OK|W_OK|X_OK)!=0)return 0;
#endif
    char probe[FILE_PATH_CAP];
    if(!append_path(probe,sizeof probe,path,"settings.ini"))return 0;
    if(check_access(probe,READ_WRITE_ACCESS)!=0 && errno!=ENOENT)return 0;
    if(is_directory(probe))return 0;
    int handle=create_unique(path,".profile-write",".tmp",probe,sizeof probe);
    if(handle<0)return 0;
    close_file(handle);
    if(remove(probe)!=0)return 0;
    strcpy(out,path);
    return 1;
}

DynaProfileChoice dyna_profile_select(const char *portable,const char *override,
                                     const char *fallback,char *out,size_t capacity)
{
    if(!out || !capacity)return DYNA_PROFILE_UNAVAILABLE;
    out[0]=0;
    if(override && *override)
        return usable_profile(override,1,out,capacity)?DYNA_PROFILE_OVERRIDE:DYNA_PROFILE_UNAVAILABLE;
    if(usable_profile(portable,0,out,capacity))return DYNA_PROFILE_PORTABLE;
    if(usable_profile(fallback,1,out,capacity))return DYNA_PROFILE_FALLBACK;
    return DYNA_PROFILE_UNAVAILABLE;
}

DynaProfileChoice dyna_profile_resolve(const char *portable,char *out,size_t capacity)
{
    char fallback[DYNA_PROFILE_PATH_CAP]={0};
#if defined(_WIN32)
    append_path(fallback,sizeof fallback,getenv("LOCALAPPDATA"),"DynaBroCollection");
#elif defined(__APPLE__)
    append_path(fallback,sizeof fallback,getenv("HOME"),"Library/Application Support/DynaBroCollection");
#elif !defined(__ANDROID__)
    const char *xdg=getenv("XDG_DATA_HOME");
    if(absolute_path(xdg))append_path(fallback,sizeof fallback,xdg,"DynaBroCollection");
    else append_path(fallback,sizeof fallback,getenv("HOME"),".local/share/DynaBroCollection");
#endif
    return dyna_profile_select(portable,getenv("GENESIS_USER_DIR"),fallback,out,capacity);
}

static int owned_log(const char *name,const char *component)
{
    size_t prefix=strlen(component);
    if(strncmp(name,component,prefix) || strlen(name)!=prefix+39)return 0;
    const char *suffix=name+prefix;
    for(size_t i=0;i<35;i++){
        if(i==0 || i==21 || i==32){if(suffix[i]!='-')return 0;}
        else if(!isdigit((unsigned char)suffix[i]))return 0;
    }
    return !strcmp(suffix+35,".log");
}

static void consider_log(const char *dir,const char *component,const char *name,
                         char recent[DYNA_LOG_KEEP][LOG_NAME_CAP],int prune)
{
    if(!owned_log(name,component))return;
    if(prune){
        char path[FILE_PATH_CAP];
        if(strcmp(name,recent[DYNA_LOG_KEEP-1])<0 && append_path(path,sizeof path,dir,name))remove(path);
        return;
    }
    for(size_t i=0;i<DYNA_LOG_KEEP;i++){
        if(strcmp(name,recent[i])<=0)continue;
        memmove(recent+i+1,recent+i,(DYNA_LOG_KEEP-i-1)*LOG_NAME_CAP);
        strcpy(recent[i],name);
        break;
    }
}

static void scan_logs(const char *dir,const char *component,
                      char recent[DYNA_LOG_KEEP][LOG_NAME_CAP],int prune)
{
#ifdef _WIN32
    char pattern[FILE_PATH_CAP];
    if(!append_path(pattern,sizeof pattern,dir,"*"))return;
    WIN32_FIND_DATAA entry;
    HANDLE list=FindFirstFileA(pattern,&entry);
    if(list==INVALID_HANDLE_VALUE)return;
    do {
        if(!(entry.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT)))
            consider_log(dir,component,entry.cFileName,recent,prune);
    } while(FindNextFileA(list,&entry));
    FindClose(list);
#else
    DIR *list=opendir(dir);
    if(!list)return;
    struct dirent *entry;
    while((entry=readdir(list))){
        char path[FILE_PATH_CAP];struct stat info;
        if(owned_log(entry->d_name,component) && append_path(path,sizeof path,dir,entry->d_name) &&
           lstat(path,&info)==0 && S_ISREG(info.st_mode))
            consider_log(dir,component,entry->d_name,recent,prune);
    }
    closedir(list);
#endif
}

FILE *dyna_log_open(const char *profile,const char *component,char *path,size_t capacity)
{
    char directory[FILE_PATH_CAP],created[FILE_PATH_CAP];
    if(!append_path(directory,sizeof directory,profile,"logs") || !ensure_directory(directory))return NULL;
    int handle=create_unique(directory,component,".log",created,sizeof created);
    if(handle<0)return NULL;
    FILE *log=file_stream(handle,"wb");
    if(!log){close_file(handle);remove(created);return NULL;}
    if(path && capacity)snprintf(path,capacity,"%s",created);
    char recent[DYNA_LOG_KEEP][LOG_NAME_CAP]={{0}};
    scan_logs(directory,component,recent,0);
    if(*recent[DYNA_LOG_KEEP-1])scan_logs(directory,component,recent,1);
    return log;
}
