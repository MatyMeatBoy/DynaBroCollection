#include "launcher_options.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d\n",__LINE__); return 1; } } while(0)
int main(void) {
    DynaLauncherOptions o; wchar_t error[256], command[2048];
    const wchar_t *args[]={L"hub.exe",L"--game",L"db2",L"--rom",L"C:\\My Games\\rom.bin",
        L"--no-launcher",L"--fullscreen",L"--widescreen",L"16:9"};
    CHECK(dyna_launcher_options_parse(9,args,&o,error,256));
    CHECK(o.game==2 && o.no_launcher && o.fullscreen && !o.setup);
    CHECK(dyna_launcher_options_command(&o,L"C:\\My App\\db2.exe",command,2048));
    CHECK(!wcscmp(command,L"\"C:\\My App\\db2.exe\" \"--no-launcher\" \"--fullscreen\" \"--widescreen\" \"16:9\" \"C:\\My Games\\rom.bin\""));
    const wchar_t *bad[]={L"hub",L"--game",L"db3"};
    CHECK(!dyna_launcher_options_parse(3,bad,&o,error,256));
    const wchar_t *unknown[]={L"hub",L"--exec",L"anything"};
    CHECK(!dyna_launcher_options_parse(3,unknown,&o,error,256));
    const wchar_t *conflict[]={L"hub",L"--setup",L"--no-launcher"};
    CHECK(!dyna_launcher_options_parse(3,conflict,&o,error,256));
    const wchar_t *missing[]={L"hub",L"--rom"};
    CHECK(!dyna_launcher_options_parse(2,missing,&o,error,256));
    const wchar_t *escape[]={L"hub",L"--rom",L"C:\\quoted\" name\\"};
    CHECK(dyna_launcher_options_parse(3,escape,&o,error,256));
    CHECK(dyna_launcher_options_command(&o,L"game.exe",command,2048));
    CHECK(!wcscmp(command,L"\"game.exe\" \"C:\\quoted\\\" name\\\\\""));
#ifdef _WIN32
    int parsed_count=0;
    wchar_t **parsed=CommandLineToArgvW(command,&parsed_count);
    CHECK(parsed && parsed_count==2 && !wcscmp(parsed[1],o.rom));
    LocalFree(parsed);
#endif
    CHECK(!dyna_launcher_options_command(&o,L"game.exe",command,4));
    CHECK(strstr(dyna_launcher_integration_info(),"\"schema_version\":1"));
    puts("ok: launcher allowlist, whitespace/quote escaping, conflicts, integration schema");
    return 0;
}
