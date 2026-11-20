#include "achievements.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct { DynaAchievementItem item; unsigned start,count; } CatalogEntry;
#include "achievement_catalog/db1.inc"
#include "achievement_catalog/db2.inc"
#define MAX_ITEMS 128
static DynaAchievementItem items[MAX_ITEMS];
static const CatalogEntry *catalog;
static const uint32_t *requires;
static size_t item_count;
static int current_game,dirty;
static char profile[1024];
static DynaAchievementUnlock last_unlock;
/* Every item unlocks at most once between init calls, so MAX_ITEMS bounds
 * all pending events, even if the UI has not consumed any notification. */
static DynaAchievementUnlock unlock_queue[MAX_ITEMS];
static size_t queue_head,queue_count;
static DynaAchievementLiveSink live_sink;
static uint8_t reported_live[MAX_ITEMS];
static int find_id(uint32_t id) {
    size_t i;for(i=0;i<item_count;++i) if(items[i].id==id)return (int)i;
    return -1;
}
static uint64_t timestamp(void) {
    time_t t=time(NULL);return t>0?(uint64_t)t:1;
}
static void unlock(size_t index,uint64_t when,int notify) {
    items[index].unlocked_at=when;dirty=1;
    if(notify) {
        last_unlock.id=items[index].id;++last_unlock.serial;
        if(queue_count<MAX_ITEMS) {
            unlock_queue[(queue_head+queue_count)%MAX_ITEMS]=last_unlock;
            ++queue_count;
        }
    }
}
static void milestones(int notify) {
    size_t i;
    for(i=0;i<item_count;++i) {
        unsigned j;uint64_t when=0;
        if(!catalog[i].count || items[i].unlocked_at || !items[i].qualified)continue;
        for(j=0;j<catalog[i].count;++j) {
            int index=find_id(requires[catalog[i].start+j]);
            if(index<0 || !items[index].unlocked_at)break;
            if(items[index].unlocked_at>when)when=items[index].unlocked_at;
        }
        if(j==catalog[i].count)unlock(i,when,notify);
    }
}
static int parse_uint(const char **cursor,uint64_t *value) {
    const char *p=*cursor;uint64_t result=0;
    if(*p<'0'||*p>'9')return 0;
    do {
        unsigned digit=(unsigned)(*p-'0');
        if(result>(UINT64_MAX-digit)/10)return 0;
        result=result*10+digit;++p;
    } while(*p>='0'&&*p<='9');
    *cursor=p;*value=result;return 1;
}
static int end_line(const char *p) {return !strcmp(p,"\n")||!strcmp(p,"\r\n")||!*p;}
static int load_profile(FILE *file) {
    char line[256];uint64_t saved[MAX_ITEMS]={0};size_t i;
    uint64_t game,version;const char *header;
    if(!fgets(line,sizeof line,file) || strncmp(line,"DYNA_ACHIEVEMENTS ",18))return 0;
    header=line+18;
    if(!parse_uint(&header,&version)||*header++!=' '||!parse_uint(&header,&game)||
       !end_line(header)||version!=1||game!=(unsigned)current_game)return 0;
    while(fgets(line,sizeof line,file)) {
        const char *p=line;uint64_t id,when;int index;
        if(!parse_uint(&p,&id)||*p++!=' '||!parse_uint(&p,&when)||!end_line(p)||
           !when||id>UINT32_MAX)return 0;
        index=find_id((uint32_t)id);
        if(index<0||!items[index].qualified||saved[index])return 0;
        saved[index]=when;
    }
    if(ferror(file))return 0;
    /* Milestone rows cannot establish progress; their prerequisites must all
     * exist in this same validated profile. No partial malformed-file loads. */
    for(i=0;i<item_count;++i) if(saved[i]&&catalog[i].count) {
        unsigned j;
        for(j=0;j<catalog[i].count;++j) {
            int index=find_id(requires[catalog[i].start+j]);
            if(index<0||!saved[index])return 0;
        }
    }
    for(i=0;i<item_count;++i)items[i].unlocked_at=saved[i];
    milestones(0);return 1;
}
int dyna_achievements_init(const char *settings_path,int game2) {
    const char *a,*b,*sep;size_t prefix,i;FILE *file;int ok;
    catalog=game2?db2_catalog:db1_catalog;
    requires=game2?db2_requires:db1_requires;
    item_count=game2?sizeof(db2_catalog)/sizeof(db2_catalog[0]):sizeof(db1_catalog)/sizeof(db1_catalog[0]);
    current_game=game2?2:1;dirty=0;profile[0]=0;
    dyna_achievements_set_live_sink(NULL);
    memset(&last_unlock,0,sizeof last_unlock);
    queue_head=queue_count=0;
    if(item_count>MAX_ITEMS) {item_count=0;return 0;}
    for(i=0;i<item_count;++i)items[i]=catalog[i].item;
    if(!settings_path||!*settings_path)return 0;
    a=strrchr(settings_path,'/');b=strrchr(settings_path,'\\');
    sep=!a?b:!b?a:a>b?a:b;prefix=sep?(size_t)(sep+1-settings_path):0;
    if(prefix>=sizeof profile)return 0;
    memcpy(profile,settings_path,prefix);
    if(snprintf(profile+prefix,sizeof profile-prefix,"dynabrothers-achievements-db%d.txt",current_game)>=(int)(sizeof profile-prefix)) {
        profile[0]=0;return 0;
    }
    file=fopen(profile,"r");if(!file)return errno==ENOENT;
    ok=load_profile(file);if(fclose(file)!=0)ok=0;
    if(!ok) {for(i=0;i<item_count;++i)items[i].unlocked_at=0;dirty=0;}
    return ok;
}
void dyna_achievements_set_live_sink(DynaAchievementLiveSink sink) {
    live_sink=sink;
    memset(reported_live,0,sizeof reported_live);
}
static void report_live(size_t index) {
    if(!live_sink || reported_live[index] || !live_sink(items[index].id))return;
    reported_live[index]=1;
    for(size_t i=0;i<item_count;++i) {
        if(!catalog[i].count || reported_live[i] || !items[i].qualified)continue;
        unsigned j;
        for(j=0;j<catalog[i].count;++j) {
            int dependency=find_id(requires[catalog[i].start+j]);
            if(dependency<0 || !reported_live[dependency])break;
        }
        if(j==catalog[i].count)
            reported_live[i]=(uint8_t)(live_sink(items[i].id)!=0);
    }
}
void dyna_achievements_emit(void *user,uint32_t id) {
    int index;(void)user;
    index=find_id(id);
    if(index<0||!items[index].qualified||catalog[index].count)return;
    report_live((size_t)index);
    if(items[index].unlocked_at)return;
    unlock((size_t)index,timestamp(),1);milestones(1);
}
int dyna_achievements_flush(void) {
    char temporary[1060];FILE *file;size_t i;int ok;
    if(!dirty)return 1;
    if(!profile[0])return 0;
    if(snprintf(temporary,sizeof temporary,"%s.tmp",profile)>=(int)sizeof temporary)return 0;
    file=fopen(temporary,"w");if(!file)return 0;
    ok=fprintf(file,"DYNA_ACHIEVEMENTS 1 %d\n",current_game)>0;
    for(i=0;i<item_count&&ok;++i)if(items[i].unlocked_at)
        ok=fprintf(file,"%" PRIu32 " %" PRIu64 "\n",items[i].id,items[i].unlocked_at)>0;
    if(fflush(file)!=0)ok=0;
    if(fclose(file)!=0)ok=0;
#ifdef _WIN32
    if(ok)ok=MoveFileExA(temporary,profile,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    if(ok)ok=rename(temporary,profile)==0;
#endif
    if(!ok) {remove(temporary);return 0;}
    dirty=0;return 1;
}
size_t dyna_achievements_count(void) {return item_count;}
const DynaAchievementItem *dyna_achievements_item(size_t index) {return index<item_count?items+index:NULL;}
DynaAchievementUnlock dyna_achievements_last_unlock(void) {return last_unlock;}

int dyna_achievements_pop_unlock(DynaAchievementUnlock *out) {
    if(!out||!queue_count)return 0;
    *out=unlock_queue[queue_head];queue_head=(queue_head+1)%MAX_ITEMS;--queue_count;
    return 1;
}
