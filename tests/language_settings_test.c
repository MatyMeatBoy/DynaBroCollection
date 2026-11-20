#include "language_settings.h"
#include "recomp_launcher.h"
#include "game_spec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
const GameSpec g_game_spec={0};
static int replaying;
int genesis_replay_playing(void){return replaying;}

int main(int argc,char **argv) {
    assert(argc==2);
    char filename[1024];snprintf(filename,sizeof filename,"%s/dynabrothers-language.ini",argv[1]);
    remove(filename); /* CTest-owned isolated fixture only. */
    char settings[1024];snprintf(settings,sizeof settings,"%s/settings.ini",argv[1]);
    dyna_language_settings_load(settings);
    assert(dyna_language_english()==1);
    assert(dyna_language_runtime_set("dyna.english",0));
    int value=-1;
    assert(dyna_language_runtime_get("dyna.english",&value)&&value==0);
    dyna_language_settings_load(settings);
    assert(dyna_language_english()==0);
    assert(!dyna_language_runtime_set("dyna.english",2));
    assert(!dyna_language_runtime_set("unknown",1));
    assert(dyna_language_runtime_set("dyna.english",1));
    RecompLauncherCModProvider empty={0};
    const RecompLauncherCModProvider *provider=dyna_language_mods(&empty);
    assert(provider->feature_count(NULL)==3);
    assert(!provider->feature_enable(NULL,"dyna-controls","player-1",0));
    replaying=1;
    assert(provider->feature_enable(NULL,"dyna-controls","player-1",0));
    assert(provider->feature_enable(NULL,"dyna-controls","player-2",1));
    assert(provider->feature_set_option(NULL,"dyna-controls","player-1","layout","4"));
    assert(!provider->feature_set_option(NULL,"dyna-controls","player-1","layout","9"));
    replaying=0;
    RecompLauncherCModFeature feature;
    assert(provider->feature_get(NULL,0,&feature)&&feature.enabled);
    assert(provider->feature_enable(NULL,"dyna-language","english",0));
    assert(provider->commit(NULL,"unused-test-rom"));
    dyna_language_settings_load(settings);
    assert(dyna_language_english()==0);
    dyna_language_started();
    assert(provider->feature_enable(NULL,"dyna-language","english",1));
    assert(provider->feature_get(NULL,0,&feature)&&!strcmp(feature.status,"Restart required"));
    assert(provider->commit(NULL,"unused-test-rom"));
    dyna_language_settings_load(settings);
    assert(dyna_language_english()==1);
    /* RecompLauncher writes spaced assignments when it creates an INI. */
    FILE *launcher_settings=fopen(filename,"w");
    assert(launcher_settings);
    fputs("english = 0\n",launcher_settings);
    assert(fclose(launcher_settings)==0);
    dyna_language_settings_load(settings);
    assert(dyna_language_english()==0);
    snprintf(settings,sizeof settings,"%s/absent/settings.ini",argv[1]);
    dyna_language_settings_load(settings);
    assert(!provider->commit(NULL,"unused-test-rom"));
    assert(provider->last_error(NULL)[0]);
    remove(filename);
    return 0;
}
