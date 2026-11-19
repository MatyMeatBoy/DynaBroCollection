/* DynaBrothers.exe: the only executable at the package root. It starts the
 * real launcher in data\ (with the same arguments) and exits, keeping the
 * root to one exe plus the data\ and roms\ folders. No runtime DLLs. */
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR command,int show){
    (void)instance;(void)previous;(void)show;
    wchar_t dir[2048],exe[2200],line[8192],work[2200];
    DWORD n=GetModuleFileNameW(NULL,dir,2048);if(!n||n>=2048)return 1;
    wchar_t *slash=wcsrchr(dir,L'\\');if(!slash)return 1;*slash=0;
    swprintf(exe,2200,L"%ls\\data\\DynaBrothersLauncher.exe",dir);
    swprintf(work,2200,L"%ls\\data",dir);
    swprintf(line,8192,L"\"%ls\" %ls",exe,command?command:L"");
    STARTUPINFOW si={0};si.cb=sizeof si;PROCESS_INFORMATION pi={0};
    if(!CreateProcessW(exe,line,NULL,NULL,FALSE,0,NULL,work,&si,&pi)){
        MessageBoxW(NULL,L"data\\DynaBrothersLauncher.exe was not found. Keep DynaBrothers.exe next to the data and roms folders.",L"Dyna Brothers",MB_OK|MB_ICONERROR);
        return 1;
    }
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return 0;
}
