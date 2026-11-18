#include "achievements.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#ifdef _WIN32
#include <direct.h>
#define make_directory(p) _mkdir(p)
#define remove_directory(p) _rmdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define make_directory(p) mkdir(p,0700)
#define remove_directory(p) rmdir(p)
#endif
static char settings[1024],db1[1024],db2[1024];
static const DynaAchievementItem *item(uint32_t id) {
    size_t i;for(i=0;i<dyna_achievements_count();++i) {
        const DynaAchievementItem *p=dyna_achievements_item(i);if(p->id==id)return p;
    }
    assert(0);return NULL;
}
static void write_profile(const char *text) {
    FILE *file=fopen(db2,"w");assert(file);assert(fputs(text,file)>=0);assert(!fclose(file));
}
static void all_locked(void) {
    size_t i;for(i=0;i<dyna_achievements_count();++i)assert(!dyna_achievements_item(i)->unlocked_at);
}
int main(int argc,char **argv) {
    uint64_t when,serial;unsigned n;DynaAchievementUnlock event;char pending[1024],pending_settings[1100],pending_profile[1100];
    const char *bad[]={
        "DYNA_ACHIEVEMENTS 1 1\n2000001 123\n",
        "DYNA_ACHIEVEMENTS 2 2\n2000001 123\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000001 123\n4294967296 4\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000001 123\n999 4\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000001 123\n2000001 124\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000001 0\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000001 -3\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000001 18446744073709551616\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000001 123junk\n",
        "DYNA_ACHIEVEMENTS 1 2\n2000900 123\n",
        "DYNA_ACHIEVEMENTS 1 2garbage\n",
        "bad profile\n"
    };
    assert(argc==2);
    assert(snprintf(settings,sizeof settings,"%s/settings.ini",argv[1])<(int)sizeof settings);
    assert(snprintf(db1,sizeof db1,"%s/dynabrothers-achievements-db1.txt",argv[1])<(int)sizeof db1);
    assert(snprintf(db2,sizeof db2,"%s/dynabrothers-achievements-db2.txt",argv[1])<(int)sizeof db2);
    remove(db1);remove(db2);
    assert(dyna_achievements_init(settings,1));assert(dyna_achievements_count()==99);
    assert(!dyna_achievements_item(99));assert(!dyna_achievements_item((size_t)-1));all_locked();
    dyna_achievements_emit(NULL,0);dyna_achievements_emit(NULL,UINT32_MAX);
    dyna_achievements_emit(NULL,193201);dyna_achievements_emit(NULL,2000900);all_locked();
    dyna_achievements_emit(NULL,2000001);when=item(2000001)->unlocked_at;assert(when);
    assert(dyna_achievements_last_unlock().id==2000001);serial=dyna_achievements_last_unlock().serial;
    dyna_achievements_emit(NULL,2000001);assert(dyna_achievements_last_unlock().serial==serial);
    assert(dyna_achievements_flush());assert(dyna_achievements_flush());
    assert(dyna_achievements_init(settings,1));assert(item(2000001)->unlocked_at==when);
    assert(!dyna_achievements_last_unlock().serial);
    assert(!dyna_achievements_pop_unlock(&event));
    for(n=2;n<=7;++n)dyna_achievements_emit(NULL,2000000+n);
    assert(!item(2000900)->unlocked_at);dyna_achievements_emit(NULL,2000008);
    assert(item(2000900)->unlocked_at);assert(dyna_achievements_last_unlock().id==2000900);
    assert(!item(2000901)->unlocked_at);
    assert(!dyna_achievements_pop_unlock(NULL)); /* NULL does not consume */
    for(n=2;n<=8;++n) {
        assert(dyna_achievements_pop_unlock(&event));
        assert(event.id==2000000+n && event.serial==n-1);
    }
    assert(dyna_achievements_pop_unlock(&event));assert(event.id==2000900 && event.serial==8);
    assert(!dyna_achievements_pop_unlock(&event));assert(dyna_achievements_flush());
    assert(dyna_achievements_init(settings,1));assert(item(2000900)->unlocked_at);
    assert(dyna_achievements_init(settings,0));assert(dyna_achievements_count()==76);all_locked();
    assert(item(205017)->qualified && item(193205)->qualified);
    dyna_achievements_emit(NULL,2000001);all_locked();
    dyna_achievements_emit(NULL,183303);assert(item(183303)->unlocked_at);
    assert(dyna_achievements_pop_unlock(&event));assert(event.id==183303);
    dyna_achievements_emit(NULL,183311);dyna_achievements_emit(NULL,183313);
    assert(dyna_achievements_pop_unlock(&event));assert(event.id==183311 && event.serial==2);
    assert(dyna_achievements_pop_unlock(&event));assert(event.id==183313 && event.serial==3);
    assert(!dyna_achievements_pop_unlock(&event));assert(dyna_achievements_flush());
    assert(dyna_achievements_init(settings,1));assert(item(2000001)->unlocked_at==when);
    for(n=0;n<sizeof bad/sizeof bad[0];++n) {
        write_profile(bad[n]);assert(!dyna_achievements_init(settings,1));all_locked();
    }
    /* Existing stage-only profiles derive missing aggregates, then persist them. */
    write_profile("DYNA_ACHIEVEMENTS 1 2\n2000001 1\n2000002 2\n2000003 3\n2000004 4\n2000005 5\n2000006 6\n2000007 7\n2000008 8\n");
    assert(dyna_achievements_init(settings,1));assert(item(2000900)->unlocked_at==8);
    assert(!dyna_achievements_last_unlock().serial);assert(dyna_achievements_flush());
    assert(dyna_achievements_init(settings,1));assert(item(2000900)->unlocked_at==8);
    /* A missing destination is retryable without losing the pending unlock. */
    assert(snprintf(pending,sizeof pending,"%s/retry",argv[1])<(int)sizeof pending);
    assert(snprintf(pending_settings,sizeof pending_settings,"%s/settings.ini",pending)<(int)sizeof pending_settings);
    assert(snprintf(pending_profile,sizeof pending_profile,"%s/dynabrothers-achievements-db2.txt",pending)<(int)sizeof pending_profile);
    remove(pending_profile);remove_directory(pending);
    assert(dyna_achievements_init(pending_settings,1));dyna_achievements_emit(NULL,2000101);
    when=item(2000101)->unlocked_at;assert(!dyna_achievements_flush());assert(item(2000101)->unlocked_at==when);
    assert(!make_directory(pending));assert(dyna_achievements_flush());
    assert(dyna_achievements_init(pending_settings,1));assert(item(2000101)->unlocked_at==when);
    remove(pending_profile);remove_directory(pending);remove(db1);remove(db2);
    assert(!dyna_achievements_init(NULL,1));all_locked();
    puts("Local achievements persistence, duplicate, milestone, malformed profile and cross-game tests passed");
    return 0;
}
