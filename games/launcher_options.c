#include "launcher_options.h"
#include <string.h>

static int fail(wchar_t *out,size_t count,const wchar_t *message) {
    if(out && count) { wcsncpy(out,message,count-1);out[count-1]=0; }
    return 0;
}
int dyna_launcher_options_parse(int argc,const wchar_t *const *argv,
    DynaLauncherOptions *out,wchar_t *error,size_t count) {
    if(!out || argc<1 || !argv)return fail(error,count,L"Invalid launch arguments.");
    memset(out,0,sizeof *out);if(error&&count)error[0]=0;
    for(int i=1;i<argc;++i) {
        const wchar_t *a=argv[i];
        if(!a)return fail(error,count,L"Invalid launch argument.");
        if(!wcscmp(a,L"--help") || !wcscmp(a,L"-h"))out->help=1;
        else if(!wcscmp(a,L"--integration-info"))out->integration_info=1;
        else if(!wcscmp(a,L"--no-launcher"))out->no_launcher=1;
        else if(!wcscmp(a,L"--setup"))out->setup=1;
        else if(!wcscmp(a,L"--fullscreen"))out->fullscreen=1;
        else if(!wcscmp(a,L"--game") || !wcscmp(a,L"--rom") || !wcscmp(a,L"--widescreen")) {
            if(i+1>=argc || !argv[i+1] || !*argv[i+1] || !wcsncmp(argv[i+1],L"--",2))
                return fail(error,count,L"A launch option is missing its value. Use --help for examples.");
            const wchar_t *v=argv[++i];
            if(!wcscmp(a,L"--game")) {
                if(out->game)return fail(error,count,L"Choose one game per launch.");
                if(!wcscmp(v,L"db1"))out->game=1;
                else if(!wcscmp(v,L"db2"))out->game=2;
                else return fail(error,count,L"Game must be db1 or db2.");
            } else if(!wcscmp(a,L"--rom")) {
                if(out->rom)return fail(error,count,L"Choose one ROM per launch.");
                out->rom=v;
            } else {
                if(out->widescreen)return fail(error,count,L"Choose one widescreen mode.");
                if(wcscmp(v,L"off") && wcscmp(v,L"16:9") && wcscmp(v,L"fit"))
                    return fail(error,count,L"Widescreen must be off, 16:9, or fit.");
                out->widescreen=v;
            }
        } else return fail(error,count,L"Unknown launch option. Use --help for supported options.");
    }
    if(out->setup && out->no_launcher)
        return fail(error,count,L"Choose --setup or --no-launcher for this launch.");
    return 1;
}

typedef struct Command { wchar_t *out;size_t count,used;int ok; } Command;
static void put(Command *c,wchar_t ch) {
    if(c->used+1>=c->count){c->ok=0;return;}
    c->out[c->used++]=ch;c->out[c->used]=0;
}
/* Windows CRT parsing: double backslashes before a quote and at closing quote. */
static void arg(Command *c,const wchar_t *value) {
    if(c->used)put(c,L' ');
    put(c,L'"');
    while(*value) {
        size_t slashes=0;
        while(*value==L'\\'){++slashes;++value;}
        size_t copies=(*value==L'"' || !*value)?slashes*2:slashes;
        for(size_t i=0;i<copies;++i)put(c,L'\\');
        if(!*value)break;
        if(*value==L'"')put(c,L'\\');
        put(c,*value++);
    }
    put(c,L'"');
}
int dyna_launcher_options_command(const DynaLauncherOptions *o,const wchar_t *exe,
    wchar_t *out,size_t count) {
    if(!o||!exe||!*exe||!out||!count)return 0;
    out[0]=0;Command c={out,count,0,1};arg(&c,exe);
    if(o->no_launcher)arg(&c,L"--no-launcher");
    if(o->setup)arg(&c,L"--launcher");
    if(o->fullscreen)arg(&c,L"--fullscreen");
    if(o->widescreen){arg(&c,L"--widescreen");arg(&c,o->widescreen);}
    if(o->rom)arg(&c,o->rom);
    if(!c.ok)out[0]=0;
    return c.ok;
}
const wchar_t *dyna_launcher_options_help(void) {
    return L"Dyna Brothers\n\n"
        L"--game db1|db2          Start the selected game\n"
        L"--rom PATH             Use your game ROM (quote paths containing spaces)\n"
        L"--no-launcher          Start playing without opening setup\n"
        L"--setup                Open game setup before playing\n"
        L"--fullscreen           Start in fullscreen\n"
        L"--widescreen off|16:9|fit\n"
        L"--integration-info     Print supported launch options as JSON\n"
        L"--help                 Show this help\n\n"
        L"Example: DynaBrothersLauncher.exe --game db2 --rom \"C:\\Games\\DB2.bin\" --no-launcher\n";
}
const char *dyna_launcher_integration_info(void) {
    return "{\"schema_version\":1,\"application\":\"Dyna Brothers\","
        "\"games\":[{\"id\":\"db1\",\"executable\":\"DynaBrothers1AOT.exe\"},"
        "{\"id\":\"db2\",\"executable\":\"DynaBrothers2AOT.exe\"}],"
        "\"options\":{\"--game\":[\"db1\",\"db2\"],\"--rom\":\"path\","
        "\"--no-launcher\":true,\"--setup\":true,\"--fullscreen\":true,"
        "\"--widescreen\":[\"off\",\"16:9\",\"fit\"]},"
        "\"rom_required\":\"user_supplied\",\"writes_rom\":false,"
        "\"paths_relative_to\":\"launcher_directory\","
        "\"launch_template\":[\"--game\",\"{game}\",\"--rom\",\"{rom}\",\"--no-launcher\"]}\n";
}
