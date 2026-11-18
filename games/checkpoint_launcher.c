#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <wchar.h>
#include <shellapi.h>
#include <string.h>
#include "launcher_options.h"
#include "launcher_discovery.h"
#include "hub_settings.h"
#include "hub_navigation.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {UINT32 version;void *callback;BOOL no_thread,no_codecs;} DynaGdiStartup;
static HMODULE image_library;static ULONG_PTR image_token;static void *logo,*game_art[2];
static int (WINAPI *image_load)(const WCHAR *,void **);
static int (WINAPI *graphics_create)(HDC,void **);
static int (WINAPI *graphics_delete)(void *);
static int (WINAPI *image_draw)(void *,void *,int,int,int,int);
static int (WINAPI *image_dispose)(void *);
static int (WINAPI *image_width)(void *,UINT *), (WINAPI *image_height)(void *,UINT *);
static void (WINAPI *image_shutdown)(ULONG_PTR);
static void images_init(void) {
 image_library=LoadLibraryExW(L"gdiplus.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!image_library)return;
 int (WINAPI *start)(ULONG_PTR *,const DynaGdiStartup *,void *)=(void *)GetProcAddress(image_library,"GdiplusStartup");
 image_load=(void *)GetProcAddress(image_library,"GdipLoadImageFromFile");graphics_create=(void *)GetProcAddress(image_library,"GdipCreateFromHDC");
 graphics_delete=(void *)GetProcAddress(image_library,"GdipDeleteGraphics");image_draw=(void *)GetProcAddress(image_library,"GdipDrawImageRectI");
 image_width=(void *)GetProcAddress(image_library,"GdipGetImageWidth");image_height=(void *)GetProcAddress(image_library,"GdipGetImageHeight");
 image_dispose=(void *)GetProcAddress(image_library,"GdipDisposeImage");image_shutdown=(void *)GetProcAddress(image_library,"GdiplusShutdown");
 DynaGdiStartup input={1,NULL,FALSE,FALSE};
 if(!start||!image_load||!graphics_create||!graphics_delete||!image_draw||!image_dispose||!image_shutdown||start(&image_token,&input,NULL))return;
 wchar_t directory[2048],path[2200];DWORD n=GetModuleFileNameW(NULL,directory,2048);if(!n||n>=2048)return;
 wchar_t *slash=wcsrchr(directory,L'\\');if(!slash)return;slash[1]=0;
 swprintf(path,2200,L"%lsassets\\img\\dyna-logo-v1.png",directory);image_load(path,&logo);
 for(int i=0;i<2;i++){swprintf(path,2200,L"%lsassets\\img\\db%d-gameplay.png",directory,i+1);image_load(path,&game_art[i]);}
}
static void image_paint(HDC dc,void *image,RECT r) {
 if(!image||!image_token)return;void *graphics=NULL;
 if(!graphics_create(dc,&graphics)){
  UINT w=0,h=0;int dw=r.right-r.left,dh=r.bottom-r.top;
  if(image_width && image_height && !image_width(image,&w) && !image_height(image,&h) && w && h){
   int fit_h=(int)((double)dw*h/w);
   if(fit_h<=dh){r.top+=(dh-fit_h)/2;dh=fit_h;}
   else {int fit_w=(int)((double)dh*w/h);r.left+=(dw-fit_w)/2;dw=fit_w;}
  }
  image_draw(graphics,image,r.left,r.top,dw,dh);graphics_delete(graphics);
 }
}
static HWND cards[2],settings_buttons[2],status;
static int settings_open,closing_hub;
static DynaHubNavigation hub_navigation;
static wchar_t exe_dir[LD_PATH_CAP],profile_dir[LD_PATH_CAP],cached_rom[2][LD_PATH_CAP],selected_rom[2][LD_PATH_CAP];
static wchar_t config_path[LD_PATH_CAP],cache_path[2][LD_PATH_CAP];
static ld_inventory discovered;
static unsigned detected_patches;
static HFONT title_font,body_font,card_font;
static HBRUSH background_brush;
#define DYNA_WIDEN_INNER(x) L##x
#define DYNA_WIDEN(x) DYNA_WIDEN_INNER(x)
static HANDLE game_process;
static DynaLauncherOptions options;
static int direct_launch;
static UINT dpi=96;
static int scale(int x){return MulDiv(x,(int)dpi,96);}
static COLORREF background=RGB(16,47,40),text_color=RGB(255,249,214),muted=RGB(191,219,174);
static const wchar_t *game_names[]={L"Dyna Brothers",L"Dyna Brothers 2 Special"};
static void frame_colors(HWND window) {
 HMODULE dll=LoadLibraryExW(L"dwmapi.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!dll)return;
 HRESULT (WINAPI *set)(HWND,DWORD,LPCVOID,DWORD)=(void *)GetProcAddress(dll,"DwmSetWindowAttribute");
 if(set){COLORREF bg=RGB(16,47,40),fg=RGB(255,249,214);BOOL dark=TRUE;int corners=2;
  set(window,20,&dark,sizeof dark);set(window,35,&bg,sizeof bg);set(window,36,&fg,sizeof fg);set(window,33,&corners,sizeof corners);
 }FreeLibrary(dll);
}
static void fonts(void) {
    if(title_font)DeleteObject(title_font);if(body_font)DeleteObject(body_font);if(card_font)DeleteObject(card_font);
    title_font=CreateFontW(-scale(34),0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    card_font=CreateFontW(-scale(25),0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    body_font=CreateFontW(-scale(16),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
}
static void layout(HWND window) {
    RECT r;GetClientRect(window,&r);int margin=scale(28),gap=scale(20),width=(r.right-margin*2-gap)/2;
    MoveWindow(cards[0],margin,scale(238),width,scale(310),TRUE);
    MoveWindow(cards[1],margin+width+gap,scale(238),width,scale(310),TRUE);
    for(int i=0;i<2;i++)MoveWindow(settings_buttons[i],margin+i*(width+gap)+width-scale(150),scale(499),scale(125),scale(37),TRUE);
    MoveWindow(status,margin,r.bottom-scale(45),r.right-margin*2,scale(25),TRUE);
    SendMessageW(status,WM_SETFONT,(WPARAM)body_font,TRUE);InvalidateRect(window,NULL,TRUE);
}
static void draw_text(HDC dc,const wchar_t *text,RECT r,HFONT font,COLORREF color,UINT flags) {
    SelectObject(dc,font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,text,-1,&r,flags|DT_NOPREFIX);
}
static void find_files(HWND window) {
 for(int i=0;i<2;i++)if(!options.rom){
  FILE *file=_wfopen(cache_path[i],L"rb");if(file){char value[8192];if(fgets(value,sizeof value,file)){value[strcspn(value,"\r\n")]=0;ld_utf8_to_wide(value,cached_rom[i],LD_PATH_CAP);}fclose(file);}
 }
 ld_options scan={0};scan.base_dir=exe_dir;scan.cached_rom[0]=cached_rom[0];scan.cached_rom[1]=cached_rom[1];
 ld_scan(&scan,&discovered);detected_patches=0;
 for(size_t i=0;i<discovered.count;i++)if(discovered.items[i].kind==LD_BPS||discovered.items[i].kind==LD_IPS)detected_patches++;
 for(int i=0;i<2;i++){const ld_item *rom=options.rom?ld_requested_rom(&discovered,i+1,options.rom):ld_best_rom(&discovered,i+1);selected_rom[i][0]=0;if(rom)wcscpy(selected_rom[i],rom->path);}
 wchar_t message[256];swprintf(message,256,L"DB1: %ls   |   DB2: %ls   |   %u patches found",selected_rom[0][0]?L"ready":L"locate ROM",selected_rom[1][0]?L"ready":L"locate ROM",detected_patches);
 if(status)SetWindowTextW(status,message);if(window)InvalidateRect(window,NULL,TRUE);
}
static int configure_game(HWND window,int index) {
 char config[8192],cache[8192],initial[8192],picked[8192];
 if(!ld_wide_to_utf8(config_path,config,sizeof config)||!ld_wide_to_utf8(cache_path[index],cache,sizeof cache))return 2;
 if(!ld_wide_to_utf8(selected_rom[index],initial,sizeof initial))initial[0]=0;
 if(!getenv("LNG_TEST_HIDDEN")){ShowWindow(window,SW_SHOW);UpdateWindow(window);}
 settings_open=1;for(int i=0;i<2;i++){ShowWindow(cards[i],SW_HIDE);ShowWindow(settings_buttons[i],SW_HIDE);}ShowWindow(status,SW_HIDE);
 SDL_Window *surface=SDL_CreateWindowFrom(window);SDL_Renderer *renderer=surface?SDL_CreateRenderer(surface,-1,SDL_RENDERER_ACCELERATED):NULL;
 if(surface&&!renderer)renderer=SDL_CreateRenderer(surface,-1,SDL_RENDERER_SOFTWARE);
 int result=2;
 Uint32 surface_id=surface?SDL_GetWindowID(surface):0;
 if(renderer)result=dyna_hub_settings_run(surface,renderer,(unsigned)index+1,config,cache,initial,picked,sizeof picked,&discovered);
 if(result==0&&picked[0]){options.rom=NULL;ld_utf8_to_wide(picked,cached_rom[index],LD_PATH_CAP);find_files(NULL);}
 if(getenv("DYNA_HUB_SELFTEST")){
  if(!surface||!renderer||SDL_GetWindowFromID(surface_id)!=surface||SDL_GetRenderer(surface)!=renderer||!IsWindow(window))abort();
  int dw=0,dh=0;SDL_GetRendererOutputSize(renderer,&dw,&dh);
  fprintf(stderr,"[hub-test] PASS settings used original HWND and renderer (%dx%d)\n",dw,dh);
 }
 if(renderer)SDL_DestroyRenderer(renderer);if(surface)SDL_DestroyWindow(surface);
 if(getenv("DYNA_HUB_SELFTEST")&&!IsWindow(window))abort();
 settings_open=0;dyna_hub_controls_start(config);
 if(closing_hub){if(IsWindow(window))DestroyWindow(window);return 1;}
 if(IsWindow(window)){
  for(int i=0;i<2;i++){ShowWindow(cards[i],SW_SHOW);ShowWindow(settings_buttons[i],SW_SHOW);}ShowWindow(status,SW_SHOW);
  layout(window);find_files(window);RedrawWindow(window,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
 }
 return result;
}
static void launch(HWND window,int index) {
    if(game_process)return;
    find_files(window);
    if(options.rom && !selected_rom[index][0] && !options.setup){
        fprintf(stderr,"[hub] Explicit ROM is missing, unsupported, or belongs to another game\n");
        if(!options.no_launcher)MessageBoxW(window,L"The requested ROM is missing, unsupported, or belongs to another game.",L"Dyna Brothers",MB_OK|MB_ICONERROR);
        return;
    }
    if(options.setup || !selected_rom[index][0]) {
        if(configure_game(window,index)!=0 || !selected_rom[index][0])return;
        options.setup=0;
    }
    DynaLauncherOptions child_options=options;
    child_options.setup=0;child_options.no_launcher=1;child_options.rom=selected_rom[index];
    wchar_t path[2048],directory[2048];DWORD length=GetModuleFileNameW(NULL,directory,2048);
    if(!length||length>=2048)return;
    wchar_t *slash=wcsrchr(directory,L'\\');if(!slash)return;slash[1]=0;
    if(swprintf(path,2048,L"%ls%ls",directory,index?L"DynaBrothers2AOT.exe":L"DynaBrothers1AOT.exe")<0)return;
    wchar_t arguments[8192];
    if(!dyna_launcher_options_command(&child_options,path,arguments,8192))return;
    STARTUPINFOW startup={0};startup.cb=sizeof startup;PROCESS_INFORMATION process={0};
    if(!CreateProcessW(path,arguments,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,directory,&startup,&process)) {
        MessageBoxW(window,L"The game could not be started. Keep the checkpoint files and runtime libraries together.",L"Dyna Brothers",MB_OK|MB_ICONERROR);return;
    }
    CloseHandle(process.hThread);game_process=process.hProcess;
    ShowWindow(window,SW_HIDE);
    EnableWindow(cards[0],FALSE);EnableWindow(cards[1],FALSE);
    SetWindowTextW(status,L"Game running. Close its window to return here.");SetTimer(window,1,250,NULL);
}
static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_CREATE:
        background_brush=CreateSolidBrush(background);
        dpi=GetDpiForWindow(window);frame_colors(window);fonts();images_init();
        for(int i=0;i<2;++i)cards[i]=CreateWindowW(L"BUTTON",game_names[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            0,0,0,0,window,(HMENU)(INT_PTR)(101+i),GetModuleHandleW(NULL),NULL);
        for(int i=0;i<2;i++)settings_buttons[i]=CreateWindowW(L"BUTTON",L"Settings",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            0,0,0,0,window,(HMENU)(INT_PTR)(201+i),GetModuleHandleW(NULL),NULL);
        status=CreateWindowW(L"STATIC",L"Choose a game",WS_CHILD|WS_VISIBLE,0,0,0,0,window,NULL,GetModuleHandleW(NULL),NULL);
        char settings_utf8[8192];if(ld_wide_to_utf8(config_path,settings_utf8,sizeof settings_utf8))dyna_hub_controls_start(settings_utf8);
        layout(window);find_files(window);SetTimer(window,2,16,NULL);SetFocus(cards[0]);return 0;
    case WM_CLOSE:
        if(settings_open){SDL_Event quit;SDL_zero(quit);quit.type=SDL_QUIT;closing_hub=1;SDL_PushEvent(&quit);return 0;}
        DestroyWindow(window);return 0;
    case WM_SIZE:layout(window);return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)l)->ptMinTrackSize.x=scale(800);((MINMAXINFO *)l)->ptMinTrackSize.y=scale(700);return 0;
    case WM_DPICHANGED:
        dpi=HIWORD(w);fonts();{RECT *r=(RECT *)l;SetWindowPos(window,NULL,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);}layout(window);return 0;
    case WM_COMMAND:
        if(LOWORD(w)==201||LOWORD(w)==202){int game=LOWORD(w)-201;if(configure_game(window,game)==0){options.setup=0;launch(window,game);}return 0;}
        if(LOWORD(w)==101||LOWORD(w)==102){launch(window,LOWORD(w)-101);return 0;}break;
    case WM_TIMER:
        if(w==2){
            if(!settings_open&&!game_process&&GetForegroundWindow()==window){
                HWND focused=GetFocus();int id=GetDlgCtrlID(focused);
                if(id==101||id==102)hub_navigation.focus=(unsigned)(id-101);
                else if(id==201||id==202)hub_navigation.focus=(unsigned)(id-199);
                unsigned old_focus=hub_navigation.focus;
                int command=dyna_hub_navigation_tick(&hub_navigation,dyna_hub_controls_poll(),SDL_GetTicks64());
                if(hub_navigation.focus!=old_focus)SetFocus(hub_navigation.focus<2?cards[hub_navigation.focus]:settings_buttons[hub_navigation.focus-2]);
                if(command)SendMessageW(window,WM_COMMAND,(WPARAM)command,0);
            }return 0;
        }
        if(game_process&&WaitForSingleObject(game_process,0)==WAIT_OBJECT_0) {
            CloseHandle(game_process);game_process=NULL;KillTimer(window,1);
            if(direct_launch){DestroyWindow(window);return 0;}
            ShowWindow(window,SW_SHOW);
            EnableWindow(cards[0],TRUE);EnableWindow(cards[1],TRUE);SetWindowTextW(status,L"Choose a game");SetFocus(cards[0]);
        }return 0;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *item=(DRAWITEMSTRUCT *)l;
        if(item->CtlID==201 || item->CtlID==202){
            RECT box=item->rcItem;HBRUSH fill=CreateSolidBrush(RGB(48,100,65));HPEN edge=CreatePen(PS_SOLID,scale(1),RGB(238,213,100));
            HGDIOBJ oldb=SelectObject(item->hDC,fill),oldp=SelectObject(item->hDC,edge);RoundRect(item->hDC,box.left,box.top,box.right,box.bottom,scale(18),scale(18));
            SelectObject(item->hDC,oldb);SelectObject(item->hDC,oldp);DeleteObject(fill);DeleteObject(edge);
            draw_text(item->hDC,L"Settings",box,body_font,text_color,DT_CENTER|DT_VCENTER|DT_SINGLELINE);return TRUE;
        }
int index=(int)item->CtlID-101;if(index<0||index>1)break;
        RECT r=item->rcItem;int selected=(item->itemState&(ODS_FOCUS|ODS_SELECTED))!=0;
        FillRect(item->hDC,&r,background_brush);
        HBRUSH brush=CreateSolidBrush(selected?RGB(42,91,65):RGB(26,66,50));
        HPEN pen=CreatePen(PS_SOLID,scale(selected?2:1),selected?RGB(255,221,94):RGB(90,141,89));
        HGDIOBJ old_brush=SelectObject(item->hDC,brush),old_pen=SelectObject(item->hDC,pen);
        RoundRect(item->hDC,r.left+2,r.top+2,r.right-2,r.bottom-2,scale(30),scale(30));
        SelectObject(item->hDC,old_brush);SelectObject(item->hDC,old_pen);DeleteObject(brush);DeleteObject(pen);
        RECT art={scale(18),scale(18),r.right-scale(18),scale(183)};image_paint(item->hDC,game_art[index],art);
        r.left+=scale(24);r.right-=scale(20);r.top=scale(197);r.bottom=r.top+scale(60);
        draw_text(item->hDC,index?L"Dyna Brothers 2\nSpecial":L"Dyna Brothers",r,card_font,text_color,DT_LEFT|DT_WORDBREAK);

        r.top=scale(270);r.bottom=scale(298);draw_text(item->hDC,L"Open game  >",r,body_font,RGB(90,205,245),DT_LEFT);return TRUE;
    }
    case WM_CTLCOLORSTATIC:SetTextColor((HDC)w,muted);SetBkColor((HDC)w,background);return (LRESULT)background_brush;
    case WM_ERASEBKGND:if(settings_open)return 1;{RECT r;GetClientRect(window,&r);FillRect((HDC)w,&r,background_brush);return 1;}
    case WM_PAINT: {
        if(settings_open){ValidateRect(window,NULL);return 0;}
        PAINTSTRUCT p;HDC dc=BeginPaint(window,&p);RECT r;GetClientRect(window,&r);r.left=scale(28);r.top=scale(24);r.bottom=scale(75);
        RECT banner={scale(160),scale(12),r.right-scale(160),scale(202)};
        if(logo)image_paint(dc,logo,banner);else draw_text(dc,L"Dyna Brothers",r,title_font,text_color,DT_LEFT);
        r.top=scale(204);r.bottom=scale(229);draw_text(dc,L"Checkpoint v" DYNA_WIDEN(DYNA_VERSION),r,body_font,RGB(90,205,245),DT_LEFT);
        r.top=scale(568);r.bottom=scale(627);r.right-=scale(28);
        draw_text(dc,L"English is enabled by default. Language and controls are in each game's settings.\nTranslations are still in progress. Use your own Japanese game dumps.",r,body_font,muted,DT_LEFT|DT_WORDBREAK);
        EndPaint(window,&p);return 0;
    }
    case WM_DESTROY:if(logo)image_dispose(logo);for(int i=0;i<2;i++)if(game_art[i])image_dispose(game_art[i]);if(image_token)image_shutdown(image_token);if(image_library)FreeLibrary(image_library);if(game_process)CloseHandle(game_process);DeleteObject(title_font);DeleteObject(body_font);DeleteObject(card_font);DeleteObject(background_brush);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR command,int show) {
    (void)previous;(void)command;
    int argc=0;wchar_t **argv=CommandLineToArgvW(GetCommandLineW(),&argc);wchar_t error[256];
    if(!argv || !dyna_launcher_options_parse(argc,(const wchar_t *const *)argv,&options,error,256)) {
        MessageBoxW(NULL,argv?error:L"Unable to read launch options",L"Dyna Brothers",MB_OK|MB_ICONERROR);if(argv)LocalFree(argv);return 2;
    }
    if(options.integration_info){
        const char *json=dyna_launcher_integration_info();DWORD written;
        HANDLE output=GetStdHandle(STD_OUTPUT_HANDLE);
        int ok=output && output!=INVALID_HANDLE_VALUE && WriteFile(output,json,(DWORD)strlen(json),&written,NULL);
        LocalFree(argv);return ok?0:1;
    }
    if(options.help){MessageBoxW(NULL,dyna_launcher_options_help(),L"Dyna Brothers",MB_OK);LocalFree(argv);return 0;}
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0){MessageBoxW(NULL,L"Could not initialize display and controllers",L"Dyna Brothers",MB_OK);LocalFree(argv);return 1;}
    DWORD dir_len=GetModuleFileNameW(NULL,exe_dir,LD_PATH_CAP);if(!dir_len||dir_len>=LD_PATH_CAP){LocalFree(argv);return 1;}
    wchar_t *last=wcsrchr(exe_dir,L'\\');if(!last){LocalFree(argv);return 1;}*last=0;
    DWORD pn=GetEnvironmentVariableW(L"GENESIS_USER_DIR",profile_dir,LD_PATH_CAP);if(!pn||pn>=LD_PATH_CAP)wcscpy(profile_dir,exe_dir);
    swprintf(config_path,LD_PATH_CAP,L"%ls\\settings.ini",profile_dir);
    for(int i=0;i<2;i++){
        swprintf(cache_path[i],LD_PATH_CAP,L"%ls\\rom-DynaBrothers%dAOT.cfg",profile_dir,i+1);
        FILE *file=_wfopen(cache_path[i],L"rb");if(file){char value[8192];if(fgets(value,sizeof value,file)){value[strcspn(value,"\r\n")]=0;ld_utf8_to_wide(value,cached_rom[i],LD_PATH_CAP);}fclose(file);}
    }
    if(options.rom){if(options.game)wcsncpy(cached_rom[options.game-1],options.rom,LD_PATH_CAP-1);else for(int i=0;i<2;i++)wcsncpy(cached_rom[i],options.rom,LD_PATH_CAP-1);}
    direct_launch=options.game!=0;
    if(options.rom && !options.setup)options.no_launcher=1;
    WNDCLASSW type={0};type.lpfnWndProc=procedure;type.hInstance=instance;
    type.hCursor=LoadCursorW(NULL,IDC_ARROW);type.lpszClassName=L"DynaBrothersCheckpoint";
    if(!RegisterClassW(&type))return 1;
    dpi=GetDpiForSystem();
    HWND window=CreateWindowW(type.lpszClassName,L"Dyna Brothers - v" DYNA_WIDEN(DYNA_VERSION),WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,scale(960),scale(740),NULL,NULL,instance,NULL);if(!window)return 1;
    if(direct_launch){launch(window,options.game-1);if(!game_process){DestroyWindow(window);LocalFree(argv);return 1;}}
    else {ShowWindow(window,show);UpdateWindow(window);}
    MSG message;while(GetMessageW(&message,NULL,0,0)>0){
        if(!settings_open&&message.message==WM_KEYDOWN&&message.wParam==VK_RETURN){
            int id=GetDlgCtrlID(GetFocus());if(id==101||id==102||id==201||id==202){SendMessageW(window,WM_COMMAND,id,0);continue;}
        }
        if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}
    }
    SDL_Quit();LocalFree(argv);return 0;
}
