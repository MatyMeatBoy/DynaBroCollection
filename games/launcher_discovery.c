#include "launcher_discovery.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>
static const ld_revision revisions[] = {
 {1048576,0x360C1B20,1},{1048576,0x950A8FC3,1},
 {1048576,0xC1E5C0DA,1},{1048576,0x3BE23E09,1},{1048576,0x38B20F8F,1},{1048576,0x2740C64E,1},
 {2621440,0xBA3554BC,2},{2621440,0xDAC9BAFC,2},{2621440,0x062E9800,2}
};
uint32_t ld_crc32(uint32_t prior,const void *data,size_t n) {
 const unsigned char *p=(const unsigned char*)data; uint32_t c=~prior; size_t i; int j;
 for(i=0;i<n;i++){c^=p[i];for(j=0;j<8;j++)c=(c>>1)^((0u-(c&1u))&0xEDB88320u);} return ~c;
}
static int eq(const wchar_t *a,const wchar_t *b){while(*a&&*b){wchar_t x=*a++,y=*b++;if(x>=L'A'&&x<=L'Z')x+=32;if(y>=L'A'&&y<=L'Z')y+=32;if(x!=y)return 0;}return *a==*b;}
ld_kind ld_classify(const wchar_t *p){const wchar_t *e=wcsrchr(p,L'.');if(!e)return LD_UNKNOWN;if(eq(e,L".md")||eq(e,L".bin")||eq(e,L".gen")||eq(e,L".68k"))return LD_ROM;if(eq(e,L".bps"))return LD_BPS;if(eq(e,L".ips"))return LD_IPS;return LD_UNKNOWN;}
int ld_identify(uint64_t s,uint32_t c,const ld_revision *r,size_t n){size_t i;if(!r){r=revisions;n=sizeof(revisions)/sizeof(*revisions);}for(i=0;i<n;i++)if(r[i].size==s&&r[i].crc32==c)return r[i].game;return 0;}
const ld_item *ld_best_rom(const ld_inventory *v,int game){size_t i;const ld_item *best=NULL;for(i=0;i<v->count;i++){const ld_item *x=&v->items[i];if(x->kind!=LD_ROM||x->game!=game)continue;if(!best||x->priority<best->priority||(x->priority==best->priority&&wcscmp(x->path,best->path)<0))best=x;}return v->truncated&&best&&best->priority!=0?NULL:best;}
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
int ld_utf8_to_wide(const char *s,wchar_t *d,size_t n){return s&&d&&n<=INT_MAX&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,s,-1,d,(int)n)>0;}
int ld_wide_to_utf8(const wchar_t *s,char *d,size_t n){return s&&d&&n<=INT_MAX&&WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,s,-1,d,(int)n,NULL,NULL)>0;}
static int join(wchar_t *out,const wchar_t *a,const wchar_t *b){size_t n=wcslen(a),m=wcslen(b);if(n+m+2>LD_PATH_CAP)return 0;wcscpy(out,a);if(n&&out[n-1]!=L'\\'&&out[n-1]!=L'/')out[n++]=L'\\';wcscpy(out+n,b);return 1;}
static int canonical(wchar_t *out,const wchar_t *p){DWORD n=GetFullPathNameW(p,LD_PATH_CAP,out,NULL);return n&&n<LD_PATH_CAP;}
/* Reject reparse points in every existing ancestor, not only the leaf. */
static int safe_path(const wchar_t *p){wchar_t q[LD_PATH_CAP];size_t i,n=wcslen(p);if(n>=LD_PATH_CAP)return 0;wcscpy(q,p);for(i=3;i<=n;i++)if(!q[i]||q[i]==L'\\'||q[i]==L'/'){wchar_t save=q[i];DWORD a;q[i]=0;a=GetFileAttributesW(q);q[i]=save;if(a!=INVALID_FILE_ATTRIBUTES&&(a&FILE_ATTRIBUTE_REPARSE_POINT))return 0;}return 1;}
static void inspect(const wchar_t *p,unsigned priority,const ld_options *o,ld_inventory *v){
 HANDLE f;DWORD got;unsigned char buf[32768];uint32_t crc=0;LARGE_INTEGER size;ld_item x;size_t i;ld_kind k=ld_classify(p);
 if(k==LD_UNKNOWN||!safe_path(p))return;
 for(i=0;i<v->count;i++)if(eq(v->items[i].path,p)){if(priority<v->items[i].priority)v->items[i].priority=priority;return;}
 f=CreateFileW(p,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);if(f==INVALID_HANDLE_VALUE)return;
 if(!GetFileSizeEx(f,&size)||size.QuadPart<0||size.QuadPart>16*1024*1024){CloseHandle(f);return;}
 memset(&x,0,sizeof(x));x.kind=k;x.size=(uint64_t)size.QuadPart;x.priority=priority;wcscpy(x.path,p);
 if(k==LD_ROM){uint64_t total=0;for(;;){if(!ReadFile(f,buf,sizeof(buf),&got,NULL)){CloseHandle(f);return;}if(!got)break;total+=got;crc=ld_crc32(crc,buf,got);}if(total!=x.size){CloseHandle(f);return;}x.crc32=crc;x.game=ld_identify(x.size,crc,o->revisions,o->revision_count);x.compatible=x.game!=0;}
 else { /* Inventory only: a magic header identifies a patch, never authorizes it. */
 if(!ReadFile(f,buf,5,&got,NULL)||got<5||(k==LD_BPS?memcmp(buf,"BPS1",4):memcmp(buf,"PATCH",5))){CloseHandle(f);return;}
 }
 CloseHandle(f);if(v->count==LD_MAX_ITEMS){v->truncated=1;return;}v->items[v->count++]=x;
}
static void walk(const wchar_t *dir,unsigned priority,unsigned depth,const ld_options *o,ld_inventory *v){wchar_t pattern[LD_PATH_CAP],p[LD_PATH_CAP];WIN32_FIND_DATAW data;HANDLE h;if(!safe_path(dir)||!join(pattern,dir,L"*"))return;h=FindFirstFileW(pattern,&data);if(h==INVALID_HANDLE_VALUE)return;do{if(++v->examined>4096){v->truncated=1;break;}if(eq(data.cFileName,L".")||eq(data.cFileName,L"..")||(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))continue;if(!join(p,dir,data.cFileName))continue;if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY){if(depth)walk(p,priority,depth-1,o,v);}else inspect(p,priority,o,v);if(v->examined>4096)break;}while(FindNextFileW(h,&data));FindClose(h);}
int ld_scan(const ld_options *options,ld_inventory *v){ld_options zero;const ld_options *o;wchar_t base[LD_PATH_CAP],p[LD_PATH_CAP],cfg[LD_PATH_CAP],values[8192];const wchar_t *sections[]={L"roms",L"mods"};size_t i,j;unsigned roots=0;
 if(!v)return 0;
 memset(v,0,sizeof(*v));memset(&zero,0,sizeof(zero));o=options?options:&zero;
 if(o->base_dir){if(!canonical(base,o->base_dir))return 0;}else{DWORD n=GetModuleFileNameW(NULL,base,LD_PATH_CAP);wchar_t *slash;if(!n||n>=LD_PATH_CAP)return 0;slash=wcsrchr(base,L'\\');if(!slash)return 0;*slash=0;}
 for(i=0;i<2;i++)if(o->cached_rom[i]&&canonical(p,o->cached_rom[i]))inspect(p,0,o,v);
 walk(base,10,0,o,v);if(join(p,base,L"roms"))walk(p,20,2,o,v);if(join(p,base,L"mods"))walk(p,30,2,o,v);
 if(o->config_file){if(!canonical(cfg,o->config_file))return 0;}else if(!join(cfg,base,L"paths.ini"))return 0;
 if(!safe_path(cfg))return 1;
 for(i=0;i<2;i++){DWORD n=GetPrivateProfileStringW(L"paths",sections[i],L"",values,8192,cfg);wchar_t *start=values;if(n>=8190)v->truncated=1;
 for(j=0;j<=n;j++)if(values[j]==L';'||!values[j]){wchar_t *end;values[j]=0;while(*start==L' '||*start==L'\t')start++;end=start+wcslen(start);while(end>start&&(end[-1]==L' '||end[-1]==L'\t'))*--end=0;if(*start){if(++roots>LD_MAX_ROOTS){v->truncated=1;break;}if((start[0]&&start[1]==L':')||start[0]==L'\\'){if(canonical(p,start))walk(p,40+(unsigned)i,2,o,v);}else if(join(p,base,start)){wchar_t full[LD_PATH_CAP];if(canonical(full,p))walk(full,40+(unsigned)i,2,o,v);}}start=values+j+1;}}
 return 1;
}
#else
int ld_scan(const ld_options *o,ld_inventory *v){(void)o;if(v)memset(v,0,sizeof(*v));return 0;}
int ld_utf8_to_wide(const char *s,wchar_t *d,size_t n){(void)s;(void)d;(void)n;return 0;}
int ld_wide_to_utf8(const wchar_t *s,char *d,size_t n){(void)s;(void)d;(void)n;return 0;}
#endif


const ld_item *ld_requested_rom(const ld_inventory *v,int game,const wchar_t *path) {
 if(!v||!path||!*path)return NULL;
#ifdef _WIN32
 wchar_t full[LD_PATH_CAP];if(!canonical(full,path))return NULL;path=full;
#endif
 for(size_t i=0;i<v->count && i<LD_MAX_ITEMS;i++)
  if(v->items[i].kind==LD_ROM && v->items[i].game==game && eq(v->items[i].path,path))return &v->items[i];
 return NULL;
}
