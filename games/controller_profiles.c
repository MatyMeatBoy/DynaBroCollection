#include "controller_profiles.h"
#include "input_map.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

typedef struct {char key[64];int profile;} Preference;
typedef struct {SDL_JoystickID id;char key[64],name[128];int choice,automatic;} Device;
static Device devices[INPUT_MAX_PLAYERS];
static Preference preferences[64];static unsigned preference_count;
static char settings_file[1024];
static const char *labels[]={"Automatic","Custom SDL bindings","M30 Bluetooth / DirectInput","M30 Mini 2.4GHz receiver","M30 XInput"};

/* Run before settings are loaded: saved source and custom bindings take precedence. */
void dyna_controller_defaults(void) {
    const int buttons[GB_COUNT]={-1,-1,-1,-1,SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,-1,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,SDL_CONTROLLER_BUTTON_BACK};
    for(unsigned p=0;p<INPUT_MAX_PLAYERS;++p) {
        PlayerInput *pi=&g_input_map.p[p];pi->pad_type=PAD_6BUTTON;pi->deadzone_pct=25;
        for(int b=0;b<GB_COUNT;++b) {
            pi->pad[b].kind=GP_BIND_BUTTON;pi->pad[b].code=buttons[b];pi->pad[b].axis_dir=0;
        }
        const GenesisButton directions[]={GB_UP,GB_DOWN,GB_LEFT,GB_RIGHT,GB_C};
        const int axes[]={SDL_CONTROLLER_AXIS_LEFTY,SDL_CONTROLLER_AXIS_LEFTY,SDL_CONTROLLER_AXIS_LEFTX,SDL_CONTROLLER_AXIS_LEFTX,SDL_CONTROLLER_AXIS_TRIGGERRIGHT};
        const int signs[]={-1,1,-1,1,1};
        for(unsigned b=0;b<5;++b) {
            GamepadBind *bind=&pi->pad[directions[b]];bind->kind=GP_BIND_AXIS;bind->code=axes[b];bind->axis_dir=signs[b];
        }
    }
    g_input_map.p[0].device=INPUT_DEV_BOTH;g_input_map.p[1].device=INPUT_DEV_NONE;
}

static void load_database(void) {
    char *base=SDL_GetBasePath();if(!base)return;
    char filename[1200];snprintf(filename,sizeof filename,"%sassets/gamecontrollerdb.txt",base);SDL_free(base);
    SDL_GameControllerAddMappingsFromFile(filename);
}
static void binding(char *mapping,size_t capacity,const char *name,char kind,int index) {
    size_t used=strlen(mapping);
    if(used<capacity)snprintf(mapping+used,capacity-used,"%s:%c%d,",name,kind,index);
}
void dyna_controller_prepare(int index) {
    if(index<0||index>=SDL_NumJoysticks())return;
    load_database();
    if(SDL_IsGameController(index))return;
    SDL_Joystick *stick=SDL_JoystickOpen(index);if(!stick)return;
    char guid[33],mapping[1200];SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(stick),guid,sizeof guid);
    snprintf(mapping,sizeof mapping,"%s,Generic SDL joystick,",guid);
    const char *buttons[]={"a","b","x","y","leftshoulder","rightshoulder","back","start","guide","leftstick","rightstick"};
    const char *axes[]={"leftx","lefty","rightx","righty","lefttrigger","righttrigger"};
    for(int b=0;b<SDL_JoystickNumButtons(stick)&&b<11;++b)binding(mapping,sizeof mapping,buttons[b],'b',b);
    for(int a=0;a<SDL_JoystickNumAxes(stick)&&a<6;++a)binding(mapping,sizeof mapping,axes[a],'a',a);
    if(SDL_JoystickNumHats(stick)>0) {
        size_t used=strlen(mapping);snprintf(mapping+used,sizeof mapping-used,"dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,");
    }
    if(SDL_JoystickNumButtons(stick)>0||SDL_JoystickNumAxes(stick)>0||SDL_JoystickNumHats(stick)>0) {
        if(SDL_GameControllerAddMapping(mapping)>=0)
            fprintf(stderr,"[controllers] Generic SDL/DirectInput mapping registered for %s; bindings are configurable\n",SDL_JoystickName(stick));
    }
    SDL_JoystickClose(stick);
}
static int contains_m30(const char *name) {
    if(!name)return 0;
    for(const char *p=name;*p;++p)if(tolower((unsigned char)p[0])=='m'&&p[1]=='3'&&p[2]=='0')return 1;
    return 0;
}
static int detect(SDL_Joystick *stick) {
    unsigned vendor=SDL_JoystickGetVendor(stick),product=SDL_JoystickGetProduct(stick);
    const char *name=SDL_JoystickName(stick);
    if(contains_m30(name)) {
        if(vendor==0x0ca3u&&product==0x24u)return DYNA_PAD_M30_MINI;
        if(strstr(name,"Xbox")||strstr(name,"XInput"))return DYNA_PAD_M30_XINPUT;
        return DYNA_PAD_M30_DINPUT;
    }
    if(vendor==0x2dc8u&&(product==0x5006u||product==0x5001u||product==0x5101u))return DYNA_PAD_M30_DINPUT;
    /* Generic Xbox and Sega Mini IDs are shared by different layouts. */
    return DYNA_PAD_CUSTOM;
}
static void remember(unsigned player,SDL_GameController *pad) {
    SDL_Joystick *stick=SDL_GameControllerGetJoystick(pad);Device *device=&devices[player];
    SDL_JoystickID id=SDL_JoystickInstanceID(stick);if(device->id==id)return;
    device->id=id;device->choice=DYNA_PAD_AUTO;device->automatic=detect(stick);
    const char *name=SDL_JoystickName(stick);snprintf(device->name,sizeof device->name,"%s",name?name:"SDL controller");
    char guid[33];SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(stick),guid,sizeof guid);
    const char *identity=SDL_JoystickGetSerial(stick);
    if(!identity||!*identity)identity=SDL_JoystickPath(stick);
    if(!identity||!*identity)identity=device->name;
    uint32_t hash=2166136261u;for(const unsigned char *p=(const unsigned char *)identity;*p;++p)hash=(hash^*p)*16777619u;
    snprintf(device->key,sizeof device->key,"%s-%08x",guid,hash);
    for(unsigned i=0;i<preference_count;++i)if(!strcmp(preferences[i].key,device->key))device->choice=preferences[i].profile;
    fprintf(stderr,"[controllers] P%u %s: %s%s%s; Genesis pad follows the selected 3/6-button protocol\n",player+1,device->name,
        labels[device->choice],device->choice==DYNA_PAD_AUTO?" -> ":"",
        device->choice==DYNA_PAD_AUTO?(device->automatic==DYNA_PAD_CUSTOM?"SDL bindings (physical model not identified)":labels[device->automatic]):"");
}
void dyna_controller_removed(unsigned player) {
    if(player<INPUT_MAX_PLAYERS){memset(&devices[player],0,sizeof devices[player]);devices[player].id=-1;}
}
void dyna_controllers_load(const char *path) {
    for(unsigned p=0;p<INPUT_MAX_PLAYERS;++p)dyna_controller_removed(p);
    preference_count=0;snprintf(settings_file,sizeof settings_file,"%s",path?path:"");
    char *a=strrchr(settings_file,'/'),*b=strrchr(settings_file,'\\'),*sep=!a?b:!b?a:a>b?a:b;
    size_t offset=sep?(size_t)(sep+1-settings_file):0;
    if(snprintf(settings_file+offset,sizeof settings_file-offset,"dynabrothers-controllers.ini")>=(int)(sizeof settings_file-offset))settings_file[0]=0;
    FILE *file=fopen(settings_file,"r");
    if(file) {
        char line[160],key[64];int value;
        while(preference_count<64&&fgets(line,sizeof line,file))if(sscanf(line,"%63[^=]=%d",key,&value)==2&&value>=0&&value<DYNA_PAD_PROFILE_COUNT) {
            snprintf(preferences[preference_count].key,64,"%s",key);preferences[preference_count++].profile=value;
        }
        fclose(file);
    }
    /* SDL retains its XInput, HID and DirectInput backends. Prepare unknown
     * joysticks before the launcher's own controller enumeration too. */
    SDL_SetHintWithPriority(SDL_HINT_XINPUT_ENABLED,"1",SDL_HINT_DEFAULT);
    SDL_SetHintWithPriority(SDL_HINT_DIRECTINPUT_ENABLED,"1",SDL_HINT_DEFAULT);
    if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)!=0)return;
    load_database();
    unsigned slot=0;
    for(int index=0;index<SDL_NumJoysticks()&&slot<INPUT_MAX_PLAYERS;++index) {
        dyna_controller_prepare(index);SDL_GameController *pad=SDL_GameControllerOpen(index);
        if(pad){remember(slot++,pad);SDL_GameControllerClose(pad);}
    }
}
static int pressed(SDL_Joystick *stick,int button){return button>=0&&button<SDL_JoystickNumButtons(stick)&&SDL_JoystickGetButton(stick,button);}
uint16_t dyna_controller_mask(unsigned player,SDL_GameController *pad,uint16_t normal) {
    if(player>=INPUT_MAX_PLAYERS||!pad)return normal;
    remember(player,pad);Device *device=&devices[player];int profile=device->choice?device->choice:device->automatic;
    /* Automatic/preset layouts accept the physical D-pad in addition to the
     * bound left axes. Custom leaves the user's direction table authoritative. */
    if(device->choice!=DYNA_PAD_CUSTOM) {
        const SDL_GameControllerButton buttons[]={SDL_CONTROLLER_BUTTON_DPAD_UP,SDL_CONTROLLER_BUTTON_DPAD_DOWN,SDL_CONTROLLER_BUTTON_DPAD_LEFT,SDL_CONTROLLER_BUTTON_DPAD_RIGHT};
        const GenesisButton logical_directions[]={GB_UP,GB_DOWN,GB_LEFT,GB_RIGHT};
        for(unsigned i=0;i<4;i++)if(SDL_GameControllerGetButton(pad,buttons[i]))normal|=input_button_bit(logical_directions[i]);
    }
    if(profile==DYNA_PAD_CUSTOM)return normal;
    SDL_Joystick *stick=SDL_GameControllerGetJoystick(pad);
    uint16_t mask=normal&(input_button_bit(GB_UP)|input_button_bit(GB_DOWN)|input_button_bit(GB_LEFT)|input_button_bit(GB_RIGHT));
    const GenesisButton logical[]={GB_A,GB_B,GB_C,GB_X,GB_Y,GB_Z,GB_START,GB_MODE};
    const int bluetooth[]={0,1,7,3,4,6,11,10},mini[]={2,1,5,3,0,4,9,8};
    const int xbox[]={SDL_CONTROLLER_BUTTON_A,SDL_CONTROLLER_BUTTON_B,-1,SDL_CONTROLLER_BUTTON_X,SDL_CONTROLLER_BUTTON_Y,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,SDL_CONTROLLER_BUTTON_START,SDL_CONTROLLER_BUTTON_BACK};
    int pct=g_input_map.p[player].deadzone_pct;if(pct<0)pct=0;if(pct>100)pct=100;
    int deadzone=pct*32767/100;
    for(unsigned i=0;i<8;++i) {
        if(g_input_map.p[player].pad_type!=PAD_6BUTTON&&(logical[i]==GB_X||logical[i]==GB_Y||logical[i]==GB_Z||logical[i]==GB_MODE))continue;
        int held=profile==DYNA_PAD_M30_XINPUT?(xbox[i]<0?SDL_GameControllerGetAxis(pad,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)>deadzone:SDL_GameControllerGetButton(pad,(SDL_GameControllerButton)xbox[i])):
            pressed(stick,(profile==DYNA_PAD_M30_MINI?mini:bluetooth)[i]);
        if(held)mask|=input_button_bit(logical[i]);
    }
    return mask;
}
static int player_key(const char *key){return key&&!strcmp(key,"controls.p1_profile")?0:key&&!strcmp(key,"controls.p2_profile")?1:-1;}
int dyna_controller_get(const char *key,int *value){int p=player_key(key);if(p<0||!value)return 0;*value=devices[p].choice;return 1;}
int dyna_controller_set(const char *key,int value) {
    int p=player_key(key);if(p<0||value<0||value>=DYNA_PAD_PROFILE_COUNT||devices[p].id<0||!settings_file[0])return 0;
    unsigned index=0;while(index<preference_count&&strcmp(preferences[index].key,devices[p].key))++index;
    if(index>=64)return 0;
    Preference previous=preferences[index];unsigned old_count=preference_count;
    snprintf(preferences[index].key,64,"%s",devices[p].key);preferences[index].profile=value;
    if(index==preference_count)++preference_count;
    char temporary[1060];snprintf(temporary,sizeof temporary,"%s.tmp",settings_file);FILE *file=fopen(temporary,"w");int ok=file!=NULL;
    if(file){for(unsigned i=0;i<preference_count;++i)if(fprintf(file,"%s=%d\n",preferences[i].key,preferences[i].profile)<0)ok=0;if(ferror(file))ok=0;if(fclose(file))ok=0;}
#ifdef _WIN32
    if(ok)ok=MoveFileExA(temporary,settings_file,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    if(ok)ok=rename(temporary,settings_file)==0;
#endif
    if(!ok){remove(temporary);preferences[index]=previous;preference_count=old_count;return 0;}
    devices[p].choice=value;return 1;
}
const char *dyna_controller_name(unsigned player){return player<INPUT_MAX_PLAYERS&&devices[player].id>=0?devices[player].name:"No controller connected";}
