#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <wchar.h>
#include <shellapi.h>
#include <string.h>
#include "launcher_options.h"
#include "launcher_discovery.h"
#include "launcher_profile.h"
#include "hub_settings.h"
#include "hub_navigation.h"
#include "hub_layout.h"
#include "manual_data.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {UINT32 version;void *callback;BOOL no_thread,no_codecs;} DynaGdiStartup;
static HMODULE image_library;static ULONG_PTR image_token;static void *logo,*game_art[2],*box_art[2],*cart_art[2];
static double card_reveal[2];
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
 swprintf(path,2200,L"%lsassets\\img\\dyna-logo-1plus2-v1.png",directory);image_load(path,&logo);
 for(int i=0;i<2;i++){swprintf(path,2200,L"%lsassets\\img\\db%d-gameplay.png",directory,i+1);image_load(path,&game_art[i]);}
 for(int i=0;i<2;i++){swprintf(path,2200,L"%lsassets\\img\\db%d-box.png",directory,i+1);image_load(path,&box_art[i]);}
 for(int i=0;i<2;i++){swprintf(path,2200,L"%lsassets\\img\\db%d-cart.png",directory,i+1);image_load(path,&cart_art[i]);}
 if(getenv("DYNA_HUB_ASSET_DEBUG"))for(int i=0;i<2;i++)fprintf(stderr,"[hub-asset] box%d=%p cart%d=%p\n",i,box_art[i],i,cart_art[i]);
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
  /* Scaled images are cached per size and background (GDI+ scaling of the
   * box, cartridge, gameplay and logo art on every repaint made the launcher
   * slow). Only used when the four corners show one uniform background. */
  COLORREF c0=GetPixel(dc,r.left,r.top);
  if(dw>0&&dh>0&&c0!=CLR_INVALID&&GetPixel(dc,r.left+dw-1,r.top)==c0&&GetPixel(dc,r.left,r.top+dh-1)==c0&&GetPixel(dc,r.left+dw-1,r.top+dh-1)==c0){
   static struct{void *image;int w,h;COLORREF bg;HBITMAP bitmap;}cache[24];static int next;
   HBITMAP hit=NULL;
   for(int i=0;i<24;i++)if(cache[i].bitmap&&cache[i].image==image&&cache[i].w==dw&&cache[i].h==dh&&cache[i].bg==c0){hit=cache[i].bitmap;break;}
   HDC m=CreateCompatibleDC(dc);
   if(!hit){
    hit=CreateCompatibleBitmap(dc,dw,dh);HGDIOBJ o=SelectObject(m,hit);
    HBRUSH b=CreateSolidBrush(c0);RECT all={0,0,dw,dh};FillRect(m,&all,b);DeleteObject(b);
    void *g2=NULL;if(!graphics_create(m,&g2)){image_draw(g2,image,0,0,dw,dh);graphics_delete(g2);}
    SelectObject(m,o);
    if(cache[next].bitmap)DeleteObject(cache[next].bitmap);
    cache[next].image=image;cache[next].w=dw;cache[next].h=dh;cache[next].bg=c0;cache[next].bitmap=hit;next=(next+1)%24;
   }
   HGDIOBJ o=SelectObject(m,hit);BitBlt(dc,r.left,r.top,dw,dh,m,0,0,SRCCOPY);SelectObject(m,o);DeleteDC(m);
   graphics_delete(graphics);return;
  }
  image_draw(graphics,image,r.left,r.top,dw,dh);graphics_delete(graphics);
 }
}
static HWND cards[2],settings_buttons[2],manual_buttons[2],status;
/* Opening intro: the 1+2 logo fades in and out before the game selector. */
enum { INTRO_MS=1800 };
static ULONGLONG intro_start;static int intro_active;
static void intro_controls(int show){
 for(int i=0;i<2;i++){if(cards[i])ShowWindow(cards[i],show?SW_SHOW:SW_HIDE);if(settings_buttons[i])ShowWindow(settings_buttons[i],show?SW_SHOW:SW_HIDE);if(manual_buttons[i])ShowWindow(manual_buttons[i],show?SW_SHOW:SW_HIDE);}
 if(status)ShowWindow(status,show?SW_SHOW:SW_HIDE);
}
static void intro_finish(HWND window){
 if(!intro_active)return;intro_active=0;intro_controls(1);
 if(cards[0])SetFocus(cards[0]);InvalidateRect(window,NULL,TRUE);
}
static HWND manual_prev,manual_next,manual_lang,manual_close;
static DynaManualBook manual_books[2];static int manual_books_loaded[2];
static int manuals_game=-1,manuals_page,manuals_show_translation=0;
static void *manual_page_image,*manual_page_image2;static wchar_t manual_page_image_path[LD_PATH_CAP];
/* PDF-reader-style pagination: page 0 (the cover) alone, then facing pairs
 * (1,2), (3,4), (5,6)... -- never an arbitrary two pages glued together. */
static int manuals_spread_start(int page){return page<=0?0:1+((page-1)/2)*2;}
static int manuals_next_spread(int page){return page==0?1:page+2;}
static int manuals_prev_spread(int page){return page<=1?0:page-2;}
static int settings_open,closing_hub;
static unsigned settings_visits[2];
static DynaHubLayout home_layout;
static int ui_scale_milli=1000;
static DynaHubNavigation hub_navigation;
static DynaMenuControls manuals_controls;
static wchar_t exe_dir[LD_PATH_CAP],root_dir[LD_PATH_CAP],profile_dir[LD_PATH_CAP],cached_rom[2][LD_PATH_CAP],selected_rom[2][LD_PATH_CAP];
static wchar_t config_path[LD_PATH_CAP],cache_path[2][LD_PATH_CAP];
static ld_inventory discovered;
static unsigned detected_patches;
static HFONT title_font,body_font,card_font;
static HBRUSH background_brush;
#define DYNA_WIDEN_INNER(x) L##x
#define DYNA_WIDEN(x) DYNA_WIDEN_INNER(x)
static HANDLE game_process;
static int running_game;
static DynaLauncherOptions options;
static ld_rom_result requested_rom_result;
static int direct_launch;
static DWORD direct_exit_code;
static FILE *launcher_log;
static UINT dpi=96;
static int scale(int x){return MulDiv(x,ui_scale_milli,1000);}
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
static DynaHubRect work_area(HWND window) {
 MONITORINFO mi={sizeof mi};
 HMONITOR monitor=window?MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST):
                         MonitorFromPoint((POINT){0,0},MONITOR_DEFAULTTOPRIMARY);
 if(!GetMonitorInfoW(monitor,&mi))SystemParametersInfoW(SPI_GETWORKAREA,0,&mi.rcWork,0);
 return (DynaHubRect){mi.rcWork.left,mi.rcWork.top,mi.rcWork.right-mi.rcWork.left,mi.rcWork.bottom-mi.rcWork.top};
}
static void frame_extents(unsigned density,int *w,int *h) {
 RECT r={0,0,0,0};AdjustWindowRectExForDpi(&r,WS_OVERLAPPEDWINDOW,FALSE,0,density?density:96);
 *w=r.right-r.left;*h=r.bottom-r.top;
}
static RECT native_rect(DynaHubRect r){return (RECT){r.x,r.y,r.x+r.w,r.y+r.h};}
static void move_control(HWND control,DynaHubRect r) {
 if(control)MoveWindow(control,r.x,r.y,r.w,r.h,TRUE);
}
static void layout(HWND window) {
 RECT r;GetClientRect(window,&r);
 dyna_hub_layout(r.right,r.bottom,dpi,&home_layout);
 if(home_layout.ui_scale_milli>0 && home_layout.ui_scale_milli!=ui_scale_milli){
  ui_scale_milli=home_layout.ui_scale_milli;fonts();
 }
 for(int i=0;i<2;i++){
  move_control(cards[i],home_layout.cards[i]);
  DynaHubRect sr=home_layout.settings[i];int gap=scale(8);
  DynaHubRect settings_part=sr;settings_part.w=sr.w*62/100;
  DynaHubRect manual_part=sr;manual_part.x=sr.x+settings_part.w+gap;manual_part.w=sr.w-settings_part.w-gap;
  move_control(settings_buttons[i],settings_part);move_control(manual_buttons[i],manual_part);
  if(settings_buttons[i])SetWindowPos(settings_buttons[i],HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
  if(manual_buttons[i])SetWindowPos(manual_buttons[i],HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
 }
 move_control(status,home_layout.status);
 if(status)SendMessageW(status,WM_SETFONT,(WPARAM)body_font,TRUE);
 if(manuals_game>=0){
  RECT r;GetClientRect(window,&r);int pad=scale(16),bar=scale(44);
  DynaHubRect bar_rect={pad,r.bottom-bar-pad,r.right-2*pad,bar};
  int gap=scale(10);int bw2=(bar_rect.w-3*gap)/4;
  move_control(manual_prev,(DynaHubRect){bar_rect.x,bar_rect.y,bw2,bar_rect.h});
  move_control(manual_lang,(DynaHubRect){bar_rect.x+bw2+gap,bar_rect.y,bw2,bar_rect.h});
  move_control(manual_next,(DynaHubRect){bar_rect.x+2*(bw2+gap),bar_rect.y,bw2,bar_rect.h});
  move_control(manual_close,(DynaHubRect){bar_rect.x+3*(bw2+gap),bar_rect.y,bw2,bar_rect.h});
 }
 InvalidateRect(window,NULL,TRUE);
}
static void draw_text(HDC dc,const wchar_t *text,RECT r,HFONT font,COLORREF color,UINT flags) {
    SelectObject(dc,font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,text,-1,&r,flags|DT_NOPREFIX);
}
static void find_files(HWND window) {
 for(int i=0;i<2;i++)if(!options.rom){
  FILE *file=_wfopen(cache_path[i],L"rb");if(file){char value[8192];if(fgets(value,sizeof value,file)){value[strcspn(value,"\r\n")]=0;ld_utf8_to_wide(value,cached_rom[i],LD_PATH_CAP);}fclose(file);}
 }
 ld_options scan={0};scan.base_dir=root_dir[0]?root_dir:exe_dir;scan.cached_rom[0]=cached_rom[0];scan.cached_rom[1]=cached_rom[1];
 ld_scan(&scan,&discovered);detected_patches=0;
 for(size_t i=0;i<discovered.count;i++)if(discovered.items[i].kind==LD_BPS||discovered.items[i].kind==LD_IPS)detected_patches++;
 if(options.rom){
  ld_item rom;
  requested_rom_result=ld_probe_rom(options.rom,0,NULL,&rom);
  for(int i=0;i<2;i++){
   selected_rom[i][0]=0;
   if(requested_rom_result==LD_ROM_OK&&rom.game==i+1)wcscpy(selected_rom[i],rom.path);
  }
 }else for(int i=0;i<2;i++){
  const ld_item *rom=ld_best_rom(&discovered,i+1);
  selected_rom[i][0]=0;if(rom)wcscpy(selected_rom[i],rom->path);
 }
 wchar_t message[256];swprintf(message,256,L"DB1: %ls   |   DB2: %ls   |   %u patches found",selected_rom[0][0]?L"ready":L"locate ROM",selected_rom[1][0]?L"ready":L"locate ROM",detected_patches);
 if(status)SetWindowTextW(status,message);if(window)InvalidateRect(window,NULL,TRUE);
}
static int configure_game(HWND window,int index) {
 settings_visits[index]++;
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
static void log_launch(const char *message)
{
    if(launcher_log){fprintf(launcher_log,"%s\n",message);fflush(launcher_log);}
}

static int report_launch_error(HWND window,int code,const char *message)
{
    log_launch(message);
    HANDLE output=GetStdHandle(STD_ERROR_HANDLE);
    DWORD written;
    char line[512];
    int length=snprintf(line,sizeof line,"Dyna Brothers: %s\n",message);
    BOOL reported=output&&output!=INVALID_HANDLE_VALUE&&length>0&&(size_t)length<sizeof line&&
                  WriteFile(output,line,(DWORD)length,&written,NULL);
    if(!options.no_launcher&&(!direct_launch||!reported)){
        wchar_t wide[512];
        if(ld_utf8_to_wide(message,wide,512))MessageBoxW(window,wide,L"Dyna Brothers",MB_OK|MB_ICONERROR);
    }
    return code;
}

static int launch(HWND window,int index) {
    if(game_process)return 0;
    find_files(window);
    if(options.rom && !selected_rom[index][0] && !options.setup){
        ld_rom_result result=requested_rom_result==LD_ROM_OK?LD_ROM_WRONG_GAME:requested_rom_result;
        return report_launch_error(window,result,ld_rom_error(result,index+1));
    }
    if(options.no_launcher&&!selected_rom[index][0])
        return report_launch_error(window,LD_ROM_MISSING,ld_rom_error(LD_ROM_MISSING,index+1));
    if(options.setup || !selected_rom[index][0]) {
        if(configure_game(window,index)!=0 || !selected_rom[index][0])return 1;
        options.setup=0;
    }
    DynaLauncherOptions child_options=options;
    child_options.setup=0;child_options.no_launcher=1;child_options.rom=selected_rom[index];
    wchar_t path[2048],directory[2048];DWORD length=GetModuleFileNameW(NULL,directory,2048);
    if(!length||length>=2048)return 1;
    wchar_t *slash=wcsrchr(directory,L'\\');if(!slash)return 1;slash[1]=0;
    /* Packages keep only the launcher at the root; the games live in game\.
     * Development builds keep them beside the launcher. Either way the games
     * save and read settings in the launcher's folder, as before. */
    const wchar_t *game=index?L"DynaBrothers2AOT.exe":L"DynaBrothers1AOT.exe";
    if(swprintf(path,2048,L"%lsgame\\%ls",directory,game)<0)return 1;
    if(GetFileAttributesW(path)==INVALID_FILE_ATTRIBUTES&&swprintf(path,2048,L"%ls%ls",directory,game)<0)return 1;
    wchar_t arguments[8192];
    if(!dyna_launcher_options_command(&child_options,path,arguments,8192))return 1;
    STARTUPINFOW startup={0};startup.cb=sizeof startup;PROCESS_INFORMATION process={0};
    if(!CreateProcessW(path,arguments,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,directory,&startup,&process)) {
        return report_launch_error(window,1,"The game could not be started. Keep the game files and runtime libraries together.");
    }
    CloseHandle(process.hThread);game_process=process.hProcess;running_game=index+1;
    ShowWindow(window,SW_HIDE);
    EnableWindow(cards[0],FALSE);EnableWindow(cards[1],FALSE);
    SetWindowTextW(status,L"Game running. Close its window to return here.");SetTimer(window,1,250,NULL);
    log_launch(index?"Started Dyna Brothers 2 Special":"Started Dyna Brothers");
    return 0;
}
static void *manuals_load_page_image(const DynaManualBook *book,int page,wchar_t *directory) {
 void *image=NULL;
 if(page<0||page>=(int)book->count)return NULL;
 wchar_t *slash=wcsrchr(directory,L'\\');
 if(!slash)return NULL;
 slash[1]=0;wchar_t rel[LD_PATH_CAP];
 if(ld_utf8_to_wide(book->pages[page].image,rel,LD_PATH_CAP)){
  for(wchar_t *p=rel;*p;p++)if(*p==L'/')*p=L'\\';
  wchar_t path[LD_PATH_CAP];swprintf(path,LD_PATH_CAP,L"%ls%ls",directory,rel);
  image_load(path,&image);
 }
 *slash=L'\\';
 return image;
}
/* PDF-reader pagination: `page` is the spread's FIRST page (0 = cover
 * alone; 1,3,5... = the left half of a facing pair). Loads both halves. */
static void manuals_set_page(HWND window,int page) {
 if(manuals_game<0)return;
 DynaManualBook *book=&manual_books[manuals_game];
 if(!book->count)return;
 if(page<0)page=0;if(page>=(int)book->count)page=(int)book->count-1;
 page=manuals_spread_start(page);
 manuals_page=page;
 if(manual_page_image){image_dispose(manual_page_image);manual_page_image=NULL;}
 if(manual_page_image2){image_dispose(manual_page_image2);manual_page_image2=NULL;}
 wchar_t directory[2048];DWORD n=GetModuleFileNameW(NULL,directory,2048);
 if(n&&n<2048){
  manual_page_image=manuals_load_page_image(book,page,directory);
  if(page>0)manual_page_image2=manuals_load_page_image(book,page+1,directory);
 }
 if(window)InvalidateRect(window,NULL,TRUE);
}
static void manuals_open(HWND window,int game) {
 if(game<0||game>1)return;
 memset(&manuals_controls,0,sizeof manuals_controls);
 if(!manual_books_loaded[game]){
  wchar_t directory[2048];DWORD n=GetModuleFileNameW(NULL,directory,2048);
  if(n&&n<2048){
   wchar_t *slash=wcsrchr(directory,L'\\');
   if(slash){
    slash[1]=0;wchar_t wpath[LD_PATH_CAP];char path[LD_PATH_CAP*2];
    swprintf(wpath,LD_PATH_CAP,L"%lsassets\\img\\manuals\\db%d-manual-en.json",directory,game+1);
    if(ld_wide_to_utf8(wpath,path,sizeof path))dyna_manual_load_file(path,&manual_books[game]);
   }
  }
  manual_books_loaded[game]=1;
 }
 manuals_game=game;manuals_show_translation=0;
 for(int i=0;i<2;i++){ShowWindow(cards[i],SW_HIDE);ShowWindow(settings_buttons[i],SW_HIDE);ShowWindow(manual_buttons[i],SW_HIDE);}
 ShowWindow(status,SW_HIDE);
 ShowWindow(manual_prev,SW_SHOW);ShowWindow(manual_next,SW_SHOW);ShowWindow(manual_lang,SW_SHOW);ShowWindow(manual_close,SW_SHOW);
 if(!manual_books[game].count){
  MessageBoxW(window,L"This game's manual pages were not found alongside the launcher.",L"Dyna Brothers",MB_OK|MB_ICONINFORMATION);
 }
 manuals_set_page(window,0);
 layout(window);
}
static void manuals_close(HWND window) {
 manuals_game=-1;
 if(manual_page_image){image_dispose(manual_page_image);manual_page_image=NULL;}
 ShowWindow(manual_prev,SW_HIDE);ShowWindow(manual_next,SW_HIDE);ShowWindow(manual_lang,SW_HIDE);ShowWindow(manual_close,SW_HIDE);
 for(int i=0;i<2;i++){ShowWindow(cards[i],SW_SHOW);ShowWindow(settings_buttons[i],SW_SHOW);ShowWindow(manual_buttons[i],SW_SHOW);}
 ShowWindow(status,SW_SHOW);
 layout(window);SetFocus(cards[0]);
}
/* Two book-style "paper" leaves filling as much of the window as possible,
 * split by a spine: the original scan on the left leaf, the English
 * translation on the right, instead of a small centered panel. */
static void paint_book_leaf(HDC dc,RECT leaf) {
 HBRUSH paper=CreateSolidBrush(RGB(238,230,206));
 HGDIOBJ old=SelectObject(dc,paper);HPEN pen=CreatePen(PS_SOLID,1,RGB(196,182,146));HGDIOBJ oldp=SelectObject(dc,pen);
 RoundRect(dc,leaf.left,leaf.top,leaf.right,leaf.bottom,scale(10),scale(10));
 SelectObject(dc,old);SelectObject(dc,oldp);DeleteObject(paper);DeleteObject(pen);
}
/* True if an image's own aspect ratio (w/h) says it is a pre-scanned,
 * already-joined two-page spread (wider than tall) rather than one single
 * portrait manual page. Some of this project's own manual scans (the
 * interior DB2 pages) were scanned as one JPEG per open spread, fold and
 * all, while others (DB1, and the DB2 covers) are one JPEG per single
 * page -- the image's own proportions are the only reliable signal. */
static int manuals_image_is_spread(void *image) {
 UINT w=0,h=0;
 if(!image||!image_width||!image_height||image_width(image,&w)||image_height(image,&h)||!w||!h)return 0;
 return w>h;
}
static void paint_manuals(HDC dc,RECT client) {
 FillRect(dc,&client,background_brush);
 if(manuals_game<0)return;
 DynaManualBook *book=&manual_books[manuals_game];
 int pad=scale(10),bar=scale(44),inset=scale(14);
 int left_is_spread=manuals_image_is_spread(manual_page_image);
 int right_page=(!left_is_spread&&manuals_page>0&&manuals_page+1<(int)book->count)?manuals_page+1:-1;
 RECT header={pad,pad,client.right-pad,pad+scale(28)};
 wchar_t title[320];
 if(right_page>=0)swprintf(title,320,L"%ls manual - pages %d-%d / %d",game_names[manuals_game],manuals_page+1,right_page+1,(int)book->count);
 else swprintf(title,320,L"%ls manual - page %d / %d",game_names[manuals_game],book->count?manuals_page+1:0,(int)book->count);
 draw_text(dc,title,header,body_font,muted,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
 RECT avail={pad,header.bottom+scale(4),client.right-pad,client.bottom-bar-pad};
 if(avail.bottom<=avail.top)return;
 /* PDF-viewer pagination: page 0 (cover) alone, then real facing pairs for
  * single-page scans, or the page's own pre-joined spread image shown
  * whole when the scan already contains both facing pages. The JP/EN
  * toggle swaps the right half between that page's own scan and its
  * English translation text (single-page mode only). */
 int show_both=right_page>=0;
 int show_translation_text=show_both&&manuals_show_translation&&book->pages[right_page].translation&&book->pages[right_page].translation[0];
 int spine=scale(6),stack=scale(5);
 /* Size the booklet like the real thing (owner-supplied photo reference: a
  * small, thin saddle-stitched manual), not a page stretched edge to edge.
  * Read the actual loaded image's own aspect ratio instead of guessing, so
  * every page -- single or pre-joined spread -- keeps its true proportions
  * with no letterboxed padding around it. */
 double content_aspect=0.72; /* width/height fallback before any image loads */
 {
  UINT iw=0,ih=0;
  if(manual_page_image&&image_width&&image_height&&!image_width(manual_page_image,&iw)&&!image_height(manual_page_image,&ih)&&iw&&ih)
   content_aspect=(double)iw/ih;
 }
 double book_aspect=left_is_spread?content_aspect:(show_both?content_aspect*2:content_aspect);
 int book_h=avail.bottom-avail.top;if(book_h>scale(560))book_h=scale(560);
 int book_w=(int)(book_h*book_aspect)+stack+scale(4)+(show_both?spine:0);
 if(book_w>avail.right-avail.left){
  double shrink=(double)(avail.right-avail.left)/book_w;book_h=(int)(book_h*shrink);book_w=avail.right-avail.left;
 }
 RECT body={avail.left+(avail.right-avail.left-book_w)/2,avail.top+(avail.bottom-avail.top-book_h)/2,0,0};
 body.right=body.left+book_w;body.bottom=body.top+book_h;
 RECT stack_edge={body.left,body.top+scale(6),body.left+stack,body.bottom-scale(6)};
 HBRUSH stack_brush=CreateSolidBrush(RGB(240,236,222));HGDIOBJ old_stack=SelectObject(dc,stack_brush);
 HPEN stack_pen=CreatePen(PS_SOLID,1,RGB(210,202,180));HGDIOBJ old_stack_pen=SelectObject(dc,stack_pen);
 for(int i=0;i<4;i++){
  RECT layer={stack_edge.left+i*scale(2),stack_edge.top+i*scale(3),stack_edge.left+stack+i*scale(2),stack_edge.bottom-i*scale(3)};
  Rectangle(dc,layer.left,layer.top,layer.right,layer.bottom);
 }
 SelectObject(dc,old_stack);SelectObject(dc,old_stack_pen);DeleteObject(stack_brush);DeleteObject(stack_pen);
 int leaf_x0=body.left+stack+scale(4);
 RECT left_leaf={leaf_x0,body.top,body.right,body.bottom};
 if(show_both)left_leaf.right=leaf_x0+(int)((body.right-leaf_x0-spine)*(left_is_spread?1.0:0.5));
 paint_book_leaf(dc,left_leaf);
 {
  /* Two metal staples through the booklet's actual spine edge. */
  int staple_h=scale(3),staple_w=scale(22),staple_leg=scale(5);
  int staple_x=stack_edge.left+stack/2;
  int staple_ys[2]={body.top+(body.bottom-body.top)*20/100,body.top+(body.bottom-body.top)*76/100};
  HBRUSH metal=CreateSolidBrush(RGB(176,176,182)),shadow=CreateSolidBrush(RGB(90,86,78));
  HPEN none_pen=CreatePen(PS_NULL,0,0);HGDIOBJ oldpen2=SelectObject(dc,none_pen);
  for(int i=0;i<2;i++){
   int sy=staple_ys[i];
   RECT bar={staple_x,sy-staple_h/2,staple_x+staple_w,sy+staple_h/2};
   HGDIOBJ oldb=SelectObject(dc,shadow);
   Rectangle(dc,bar.left+scale(1),bar.top+scale(1),bar.right+scale(1),bar.bottom+scale(1));
   SelectObject(dc,metal);Rectangle(dc,bar.left,bar.top,bar.right,bar.bottom);
   Rectangle(dc,bar.left-scale(1),sy-staple_leg,bar.left+scale(2),sy+staple_leg);
   Rectangle(dc,bar.right-scale(2),sy-staple_leg,bar.right+scale(1),sy+staple_leg);
   SelectObject(dc,oldb);
  }
  SelectObject(dc,oldpen2);DeleteObject(none_pen);DeleteObject(metal);DeleteObject(shadow);
 }
 RECT image_rect={left_leaf.left+inset,left_leaf.top+inset,left_leaf.right-inset,left_leaf.bottom-inset};
 image_paint(dc,manual_page_image,image_rect);
 if(left_is_spread){
  /* The scan itself already shows this page's own physical fold; mark it
   * with the same crease style as a regular facing-pages gutter so both
   * cases read consistently as "two pages meeting at a spine". */
  int mid_x=(image_rect.left+image_rect.right)/2;
  HPEN crease=CreatePen(PS_SOLID,scale(2),RGB(210,202,180));HGDIOBJ oldp=SelectObject(dc,crease);
  MoveToEx(dc,mid_x,image_rect.top,NULL);LineTo(dc,mid_x,image_rect.bottom);
  SelectObject(dc,oldp);DeleteObject(crease);
 }
 if(show_both){
  RECT right_leaf={left_leaf.right+spine,body.top,body.right,body.bottom};
  paint_book_leaf(dc,right_leaf);
  /* Plain crease between the two facing pages -- not another staple seam,
   * this gutter is just where this spread's two scans meet. */
  int spine_x=left_leaf.right+spine/2;
  HPEN crease=CreatePen(PS_SOLID,scale(2),RGB(210,202,180));HGDIOBJ oldp=SelectObject(dc,crease);
  MoveToEx(dc,spine_x,body.top+inset/2,NULL);LineTo(dc,spine_x,body.bottom-inset/2);
  SelectObject(dc,oldp);DeleteObject(crease);
  RECT right_rect={right_leaf.left+inset,right_leaf.top+inset,right_leaf.right-inset,right_leaf.bottom-inset};
  if(show_translation_text){
   wchar_t wtext[4096];ld_utf8_to_wide(book->pages[right_page].translation,wtext,4096);
   /* Long translation notes (ad pages, rule tables) can overflow a fixed-size
    * leaf at the normal body size; shrink the font just enough to make the
    * whole note fit rather than silently clipping text off the bottom. */
   HFONT fit_font=body_font;int created=0;
   for(int size=scale(16);size>=scale(10);size-=scale(1)){
    HFONT candidate=(size==scale(16))?body_font:CreateFontW(-size,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    RECT calc=right_rect;SelectObject(dc,candidate);DrawTextW(dc,wtext,-1,&calc,DT_LEFT|DT_TOP|DT_WORDBREAK|DT_CALCRECT|DT_NOPREFIX);
    int fits=calc.bottom-calc.top<=right_rect.bottom-right_rect.top;
    int last=size-scale(1)<scale(10);
    if(fits||last){
     if(created)DeleteObject(fit_font);
     fit_font=candidate;created=candidate!=body_font;
     break;
    }
    if(candidate!=body_font)DeleteObject(candidate);
   }
   draw_text(dc,wtext,right_rect,fit_font,RGB(42,34,20),DT_LEFT|DT_TOP|DT_WORDBREAK);
   if(created)DeleteObject(fit_font);
  } else {
   image_paint(dc,manual_page_image2,right_rect);
  }
 }
}
/* The logo frame is composed once (GDI+ scaling of the 1778x884 PNG is far too
 * slow per tick); each tick only blends that cached frame and blits once. */
static HBITMAP intro_frame;static int intro_w,intro_h;
static void paint_intro(HDC dc,HWND window){
 RECT r;GetClientRect(window,&r);
 if(!intro_frame||intro_w!=r.right||intro_h!=r.bottom){
  if(intro_frame)DeleteObject(intro_frame);
  intro_frame=CreateCompatibleBitmap(dc,r.right,r.bottom);intro_w=r.right;intro_h=r.bottom;
  HDC m=CreateCompatibleDC(dc);HGDIOBJ o=SelectObject(m,intro_frame);
  FillRect(m,&r,background_brush);
  int w=r.right*6/10,h=w*884/1778;RECT box={(r.right-w)/2,(r.bottom-h)/2,(r.right-w)/2+w,(r.bottom-h)/2+h};
  if(logo)image_paint(m,logo,box);
  SelectObject(m,o);DeleteDC(m);
 }
 ULONGLONG t=GetTickCount64()-intro_start;double a;
 if(t<600)a=t/600.0;else if(t<1200)a=1.0;else a=t<INTRO_MS?(INTRO_MS-t)/600.0:0.0;
 int alpha=(int)(a*255.0+0.5);if(alpha<0)alpha=0;if(alpha>255)alpha=255;
 HDC back=CreateCompatibleDC(dc);HBITMAP bb=CreateCompatibleBitmap(dc,r.right,r.bottom);HGDIOBJ ob=SelectObject(back,bb);
 FillRect(back,&r,background_brush);
 HDC src=CreateCompatibleDC(dc);HGDIOBJ os=SelectObject(src,intro_frame);
 BLENDFUNCTION f={AC_SRC_OVER,0,(BYTE)alpha,0};
 if(alpha)AlphaBlend(back,0,0,r.right,r.bottom,src,0,0,r.right,r.bottom,f);
 BitBlt(dc,0,0,r.right,r.bottom,back,0,0,SRCCOPY);
 SelectObject(src,os);DeleteDC(src);SelectObject(back,ob);DeleteObject(bb);DeleteDC(back);
}
static void paint_home(HDC dc) {
 RECT banner=native_rect(home_layout.banner);
 if(logo)image_paint(dc,logo,banner);else draw_text(dc,L"Dyna Brothers 1 + 2",banner,title_font,text_color,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 RECT version=native_rect(home_layout.version);
 draw_text(dc,L"Alpha v" DYNA_WIDEN(DYNA_VERSION),version,body_font,RGB(90,205,245),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
 RECT note=native_rect(home_layout.note);
 draw_text(dc,L"English is enabled by default. Language and controls are in Settings.\nTranslations are in progress. Use your own Japanese game dumps.",note,body_font,muted,DT_CENTER|DT_WORDBREAK);
}
static void home_capture(HWND window) {
 const char *path=getenv("DYNA_HUB_HOME_CAPTURE");if(!path||!*path)return;
 const char *reveal=getenv("DYNA_HUB_CARD_REVEAL");
 if(reveal){double value=atof(reveal);card_reveal[0]=value;card_reveal[1]=value;}
 RECT r;GetClientRect(window,&r);HDC dc=GetDC(window),memory=CreateCompatibleDC(dc);
 BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
 info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;
 info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void *bits=NULL;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,NULL,0);
 if(!memory||!bitmap||!bits)abort();HGDIOBJ previous=SelectObject(memory,bitmap);
 /* PrintWindow may report success without painting a hidden HWND. Explicit
  * WM_PRINT omits PRF_CHECKVISIBLE and asks these same native controls to draw. */
 FillRect(memory,&r,background_brush);paint_home(memory);
 HWND children[]={cards[0],cards[1],settings_buttons[0],settings_buttons[1],manual_buttons[0],manual_buttons[1],status};
 for(unsigned i=0;i<sizeof children/sizeof children[0];i++){
  HWND child=children[i];if(!(GetWindowLongW(child,GWL_STYLE)&WS_VISIBLE))continue;
  RECT bounds;GetWindowRect(child,&bounds);MapWindowPoints(NULL,window,(POINT *)&bounds,2);
  int saved=SaveDC(memory);SetViewportOrgEx(memory,bounds.left,bounds.top,NULL);
  IntersectClipRect(memory,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
  SendMessageW(child,WM_PRINT,(WPARAM)memory,PRF_CLIENT|PRF_ERASEBKGND);
  RestoreDC(memory,saved);
 }
 wchar_t wide[8192];if(!ld_utf8_to_wide(path,wide,8192))abort();FILE *file=_wfopen(wide,L"wb");if(!file)abort();
 DWORD size=(DWORD)r.right*(DWORD)r.bottom*4;
 BITMAPFILEHEADER header={0};header.bfType=0x4d42;header.bfOffBits=sizeof header+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+size;
 int ok=fwrite(&header,sizeof header,1,file)==1&&fwrite(&info.bmiHeader,sizeof(BITMAPINFOHEADER),1,file)==1&&fwrite(bits,size,1,file)==1;
 if(fclose(file)||!ok)abort();SelectObject(memory,previous);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);
 fprintf(stderr,"[hub-home-test] captured own native GDI client%ldx%ld\n",r.right,r.bottom);
}
/* Native opt-in regression: hit-test actual child windows before activating
 * them. Direct WM_COMMAND tests cannot detect an obscured Settings button. */
static void manuals_capture(HWND window) {
 const char *path=getenv("DYNA_HUB_MANUALS_CAPTURE");if(!path||!*path)return;
 manuals_open(window,getenv("DYNA_HUB_MANUALS_GAME")?atoi(getenv("DYNA_HUB_MANUALS_GAME")):0);
 const char *skip=getenv("DYNA_HUB_MANUALS_NEXT");
 for(int i=skip?atoi(skip):0;i>0;i--)manuals_set_page(window,manuals_next_spread(manuals_page));
 if(getenv("DYNA_HUB_MANUALS_EN"))manuals_show_translation=1;
 RECT r;GetClientRect(window,&r);HDC dc=GetDC(window),memory=CreateCompatibleDC(dc);
 BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
 info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;info.bmiHeader.biPlanes=1;
 info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 void *bits=NULL;HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,NULL,0);
 if(!memory||!bitmap||!bits)abort();HGDIOBJ previous=SelectObject(memory,bitmap);
 FillRect(memory,&r,background_brush);paint_manuals(memory,r);
 HWND children[]={manual_prev,manual_next,manual_lang,manual_close};
 for(unsigned i=0;i<sizeof children/sizeof children[0];i++){
  HWND child=children[i];if(!(GetWindowLongW(child,GWL_STYLE)&WS_VISIBLE))continue;
  RECT bounds;GetWindowRect(child,&bounds);MapWindowPoints(NULL,window,(POINT *)&bounds,2);
  int saved=SaveDC(memory);SetViewportOrgEx(memory,bounds.left,bounds.top,NULL);
  IntersectClipRect(memory,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
  SendMessageW(child,WM_PRINT,(WPARAM)memory,PRF_CLIENT|PRF_ERASEBKGND);
  RestoreDC(memory,saved);
 }
 wchar_t wide[8192];if(!ld_utf8_to_wide(path,wide,8192))abort();FILE *file=_wfopen(wide,L"wb");if(!file)abort();
 DWORD size=(DWORD)r.right*(DWORD)r.bottom*4;
 BITMAPFILEHEADER header={0};header.bfType=0x4d42;header.bfOffBits=sizeof header+sizeof(BITMAPINFOHEADER);header.bfSize=header.bfOffBits+size;
 int ok=fwrite(&header,sizeof header,1,file)==1&&fwrite(&info.bmiHeader,sizeof(BITMAPINFOHEADER),1,file)==1&&fwrite(bits,size,1,file)==1;
 if(fclose(file)||!ok)abort();SelectObject(memory,previous);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);
 fprintf(stderr,"[hub-manuals-test] captured manuals view %ldx%ld, page %d/%d\n",r.right,r.bottom,manuals_page+1,(int)manual_books[0].count);
 manuals_close(window);
}
static void home_selftest(HWND window) {
 HWND original=window;
 manuals_capture(window);
 home_capture(window);
 SDL_setenv("DYNA_HUB_SELFTEST","1",1);
 if(!getenv("LNG_SCRIPT"))SDL_setenv("LNG_SCRIPT","wait:3;quit",1);
 for(int i=0;i<2;i++){
  RECT button;GetWindowRect(settings_buttons[i],&button);
  POINT center={(button.left+button.right)/2,(button.top+button.bottom)/2};ScreenToClient(window,&center);
  HWND hit=ChildWindowFromPointEx(window,center,CWP_SKIPDISABLED|CWP_SKIPINVISIBLE);
  if(hit!=settings_buttons[i]){fprintf(stderr,"[hub-home-test] FAIL Settings%d hit control%d\n",i+1,GetDlgCtrlID(hit));abort();}
  unsigned before=settings_visits[i];
  SendMessageW(hit,BM_CLICK,0,0);
  if(!IsWindow(original)||settings_open||closing_hub||settings_visits[i]!=before+1||GetParent(settings_buttons[i])!=original){
   fprintf(stderr,"[hub-home-test] FAIL Settings%d did not return to same home HWND\n",i+1);abort();
  }
  fprintf(stderr,"[hub-home-test] PASS Settings%d native hit-test/BM_CLICK/inline return HWND\n",i+1);
 }
 RECT client,outer;GetClientRect(window,&client);GetWindowRect(window,&outer);
 fprintf(stderr,"[hub-home-test] PASS dpi%u client%ldx%ld outer%ldx%ld HWND preserved\n",dpi,client.right,client.bottom,outer.right-outer.left,outer.bottom-outer.top);
 DestroyWindow(window);
}
static int draw_item_direct(DRAWITEMSTRUCT *item) {
        
        {const wchar_t *pill_label=NULL;
         switch(item->CtlID){
         case 201:case 202:pill_label=L"Settings";break;
         case 301:case 302:pill_label=L"Manual";break;
         case 401:pill_label=L"< Prev";break;
         case 402:pill_label=L"Next >";break;
         case 403:pill_label=L"JP / EN";break;
         case 404:pill_label=L"Close";break;
         }
         if(pill_label){
            RECT box=item->rcItem;FillRect(item->hDC,&box,background_brush);int selected=(item->itemState&(ODS_FOCUS|ODS_SELECTED))!=0;HBRUSH fill=CreateSolidBrush(RGB(48,100,65));HPEN edge=CreatePen(PS_SOLID,scale(selected?2:1),RGB(238,213,100));
            HGDIOBJ oldb=SelectObject(item->hDC,fill),oldp=SelectObject(item->hDC,edge);RoundRect(item->hDC,box.left,box.top,box.right,box.bottom,scale(18),scale(18));
            SelectObject(item->hDC,oldb);SelectObject(item->hDC,oldp);DeleteObject(fill);DeleteObject(edge);
            draw_text(item->hDC,pill_label,box,body_font,text_color,DT_CENTER|DT_VCENTER|DT_SINGLELINE);return TRUE;
         }}
int index=(int)item->CtlID-101;if(index<0||index>1)return 0;
        RECT r=item->rcItem;int selected=(item->itemState&(ODS_FOCUS|ODS_SELECTED))!=0;
        FillRect(item->hDC,&r,background_brush);
        HBRUSH brush=CreateSolidBrush(selected?RGB(42,91,65):RGB(26,66,50));
        HPEN pen=CreatePen(PS_SOLID,scale(selected?2:1),selected?RGB(255,221,94):RGB(90,141,89));
        HGDIOBJ old_brush=SelectObject(item->hDC,brush),old_pen=SelectObject(item->hDC,pen);
        RoundRect(item->hDC,r.left+2,r.top+2,r.right-2,r.bottom-2,scale(30),scale(30));
        SelectObject(item->hDC,old_brush);SelectObject(item->hDC,old_pen);DeleteObject(brush);DeleteObject(pen);
        int height=r.bottom,width=r.right-r.left;
        int image_top=scale(12),image_bottom=height-scale(84);
        int gap=scale(12),available_width=width-scale(28)-gap;
        int box_h=(image_bottom-image_top)*7/8;
        int width_limited=available_width*10/23;
        if(box_h>width_limited)box_h=width_limited;
        if(box_h>0){
            int box_w=box_h*42/58,art_w=box_h*16/10;
            int box_x=r.left+(width-box_w-gap-art_w)/2;
            int box_y=image_bottom-box_h;
            RECT box={box_x,box_y,box_x+box_w,image_bottom};
            if(box_art[index]&&cart_art[index]&&card_reveal[index]>0){
                int cart_w=box_w*9/10,cart_h=(cart_w*90+519)/520;
                int cart_x=box_x+(box_w-cart_w)/2;
                double reveal=card_reveal[index];
                double eased=reveal*reveal*(3.0-2.0*reveal);
                int cart_y=box_y-(int)(eased*cart_h);
                RECT cart={cart_x,cart_y,cart_x+cart_w,cart_y+cart_h};
                image_paint(item->hDC,cart_art[index],cart);
            }
            image_paint(item->hDC,box_art[index],box);
            RECT art={box.right+gap,box_y,box.right+gap+art_w,image_bottom};
            image_paint(item->hDC,game_art[index],art);
        }
        r.left+=scale(20);r.right-=scale(20);r.top=height-scale(80);r.bottom=height-scale(28);
        draw_text(item->hDC,game_names[index],r,card_font,text_color,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);

        r.top=height-scale(26);r.bottom=height-scale(5);draw_text(item->hDC,L"Open game  >",r,body_font,RGB(90,205,245),DT_CENTER);return TRUE;
    
    return 0;
}
/* Owner-drawn cards and pills are composed off-screen and copied in one
 * blit, so focus moves and the cartridge animation never show half-drawn
 * layers (background, box, cartridge, text) on screen. */
static int draw_item(DRAWITEMSTRUCT *item) {
    RECT r=item->rcItem;int w=r.right-r.left,h=r.bottom-r.top;
    if(w<=0||h<=0)return draw_item_direct(item);
    HDC memory=CreateCompatibleDC(item->hDC);HBITMAP bitmap=CreateCompatibleBitmap(item->hDC,w,h);
    if(!memory||!bitmap){if(bitmap)DeleteObject(bitmap);if(memory)DeleteDC(memory);return draw_item_direct(item);}
    HGDIOBJ old=SelectObject(memory,bitmap);
    SetViewportOrgEx(memory,-r.left,-r.top,NULL);
    DRAWITEMSTRUCT copy=*item;copy.hDC=memory;
    int handled=draw_item_direct(&copy);
    SetViewportOrgEx(memory,0,0,NULL);
    if(handled)BitBlt(item->hDC,r.left,r.top,w,h,memory,0,0,SRCCOPY);
    SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);
    return handled;
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
        for(int i=0;i<2;i++)manual_buttons[i]=CreateWindowW(L"BUTTON",L"Manual",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            0,0,0,0,window,(HMENU)(INT_PTR)(301+i),GetModuleHandleW(NULL),NULL);
        manual_prev=CreateWindowW(L"BUTTON",L"< Prev",WS_CHILD|BS_OWNERDRAW,0,0,0,0,window,(HMENU)(INT_PTR)401,GetModuleHandleW(NULL),NULL);
        manual_next=CreateWindowW(L"BUTTON",L"Next >",WS_CHILD|BS_OWNERDRAW,0,0,0,0,window,(HMENU)(INT_PTR)402,GetModuleHandleW(NULL),NULL);
        manual_lang=CreateWindowW(L"BUTTON",L"Japanese / English",WS_CHILD|BS_OWNERDRAW,0,0,0,0,window,(HMENU)(INT_PTR)403,GetModuleHandleW(NULL),NULL);
        manual_close=CreateWindowW(L"BUTTON",L"Close",WS_CHILD|BS_OWNERDRAW,0,0,0,0,window,(HMENU)(INT_PTR)404,GetModuleHandleW(NULL),NULL);
        status=CreateWindowW(L"STATIC",L"Choose a game",WS_CHILD|WS_VISIBLE,0,0,0,0,window,NULL,GetModuleHandleW(NULL),NULL);
        char settings_utf8[8192];if(ld_wide_to_utf8(config_path,settings_utf8,sizeof settings_utf8))dyna_hub_controls_start(settings_utf8);
        layout(window);find_files(window);SetTimer(window,2,16,NULL);SetFocus(cards[0]);
        if(!direct_launch&&!getenv("DYNA_HUB_NO_INTRO")&&!getenv("DYNA_HUB_HOME_CAPTURE")&&!getenv("LNG_TEST_HIDDEN")){intro_active=1;intro_start=GetTickCount64();intro_controls(0);}
        return 0;
    case WM_APP+9:home_selftest(window);return 0;
    case WM_CLOSE:
        if(settings_open){SDL_Event quit;SDL_zero(quit);quit.type=SDL_QUIT;closing_hub=1;SDL_PushEvent(&quit);return 0;}
        DestroyWindow(window);return 0;
    case WM_SIZE:layout(window);return 0;
    case WM_GETMINMAXINFO:
        {int fw,fh;DynaHubRect bounds;unsigned density=GetDpiForWindow(window);if(!density)density=dpi;
         frame_extents(density,&fw,&fh);dyna_hub_window_bounds(work_area(window),density,fw,fh,1,&bounds);
         ((MINMAXINFO *)l)->ptMinTrackSize.x=bounds.w;((MINMAXINFO *)l)->ptMinTrackSize.y=bounds.h;return 0;}
    case WM_DPICHANGED:
        dpi=HIWORD(w);{RECT r=*(RECT *)l;DynaHubRect area=work_area(window);
         int width=r.right-r.left,height=r.bottom-r.top;if(width>area.w)width=area.w;if(height>area.h)height=area.h;
         if(r.left<area.x)r.left=area.x;if(r.top<area.y)r.top=area.y;
         if(r.left+width>area.x+area.w)r.left=area.x+area.w-width;
         if(r.top+height>area.y+area.h)r.top=area.y+area.h-height;
         SetWindowPos(window,NULL,r.left,r.top,width,height,SWP_NOZORDER|SWP_NOACTIVATE);}layout(window);return 0;
    case WM_COMMAND:
        if(LOWORD(w)==201||LOWORD(w)==202){int game=LOWORD(w)-201;if(configure_game(window,game)==0){options.setup=0;launch(window,game);}return 0;}
        if(LOWORD(w)==101||LOWORD(w)==102){launch(window,LOWORD(w)-101);return 0;}
        if(LOWORD(w)==301||LOWORD(w)==302){manuals_open(window,LOWORD(w)-301);return 0;}
        if(LOWORD(w)==401){manuals_set_page(window,manuals_prev_spread(manuals_page));return 0;}
        if(LOWORD(w)==402){manuals_set_page(window,manuals_next_spread(manuals_page));return 0;}
        if(LOWORD(w)==403){manuals_show_translation=!manuals_show_translation;InvalidateRect(window,NULL,TRUE);return 0;}
        if(LOWORD(w)==404){manuals_close(window);return 0;}
        break;
    case WM_TIMER:
        if(w==2&&intro_active){
            if(GetTickCount64()-intro_start>=INTRO_MS||(dyna_hub_controls_poll()&~0u))intro_finish(window);
            else InvalidateRect(window,NULL,FALSE);
            return 0;
        }
        if(w==2){
            /* The Manual viewer hides cards/settings and uses its own
             * buttons; this gamepad focus cycle must stay out of its way
             * while it's open (those controls would be invisible targets). */
            if(!settings_open&&manuals_game<0&&!game_process&&GetForegroundWindow()==window){
                HWND focused=GetFocus();int id=GetDlgCtrlID(focused);
                if(id==101||id==102)hub_navigation.focus=(unsigned)(id-101);
                else if(id==201||id==202)hub_navigation.focus=(unsigned)(id-199);
                else if(id==301||id==302)hub_navigation.focus=(unsigned)(id-301)+4;
                unsigned old_focus=hub_navigation.focus;
                int command=dyna_hub_navigation_tick(&hub_navigation,dyna_hub_controls_poll(),SDL_GetTicks64());
                if(hub_navigation.focus!=old_focus){
                    HWND *row[3]={cards,settings_buttons,manual_buttons};
                    SetFocus(row[hub_navigation.focus/2][hub_navigation.focus%2]);
                }
                if(command)SendMessageW(window,WM_COMMAND,(WPARAM)command,0);
            }
            if(!settings_open&&manuals_game>=0&&GetForegroundWindow()==window){
                int action=dyna_menu_controls_tick(&manuals_controls,dyna_hub_controls_poll(),1,SDL_GetTicks64());
                if(action==DYNA_MENU_LEFT)manuals_set_page(window,manuals_prev_spread(manuals_page));
                else if(action==DYNA_MENU_RIGHT)manuals_set_page(window,manuals_next_spread(manuals_page));
                else if(action==DYNA_MENU_ACCEPT){manuals_show_translation=!manuals_show_translation;InvalidateRect(window,NULL,TRUE);}
                else if(action==DYNA_MENU_BACK)manuals_close(window);
            }
            POINT pointer;
            HWND hovered=GetCursorPos(&pointer)?WindowFromPoint(pointer):NULL;
            for(int i=0;i<2;i++){
                int active=cards[i]&&(GetFocus()==cards[i]||hovered==cards[i]);
                double target=active?1.0:0.0;double before=card_reveal[i];
                double step=0.18;
                if(card_reveal[i]<target){card_reveal[i]+=step;if(card_reveal[i]>target)card_reveal[i]=target;}
                else if(card_reveal[i]>target){card_reveal[i]-=step;if(card_reveal[i]<target)card_reveal[i]=target;}
                if(card_reveal[i]!=before&&cards[i])InvalidateRect(cards[i],NULL,FALSE);
            }return 0;
        }
        if(game_process&&WaitForSingleObject(game_process,0)==WAIT_OBJECT_0) {
            DWORD exit_code=0;GetExitCodeProcess(game_process,&exit_code);
            CloseHandle(game_process);game_process=NULL;KillTimer(window,1);
            /* The game's own "Quit to Desktop" menu action exits with this
             * code (also used when it relaunched itself for Restart Level,
             * since the fresh instance is no longer this launcher's child). */
            char exit_message[96];
            snprintf(exit_message,sizeof exit_message,"Game process exited with code %lu",(unsigned long)exit_code);
            log_launch(exit_message);
            if(exit_code>=LD_ROM_MISSING&&exit_code<=LD_ROM_WRONG_GAME)
                report_launch_error(window,(int)exit_code,ld_rom_error((ld_rom_result)exit_code,options.game?options.game:running_game));
            else if(exit_code!=0 && exit_code!=42){
                snprintf(exit_message,sizeof exit_message,"The game closed with exit code %lu. Check the profile's logs folder.",(unsigned long)exit_code);
                report_launch_error(window,(int)exit_code,exit_message);
            }
            if(direct_launch || exit_code==42){direct_exit_code=exit_code==42?0:exit_code;DestroyWindow(window);return 0;}
            ShowWindow(window,SW_SHOW);
            EnableWindow(cards[0],TRUE);EnableWindow(cards[1],TRUE);SetWindowTextW(status,L"Choose a game");SetFocus(cards[0]);
        }return 0;
    case WM_DRAWITEM: if(draw_item((DRAWITEMSTRUCT *)l))return TRUE;break;
    case WM_CTLCOLORSTATIC:SetTextColor((HDC)w,muted);SetBkColor((HDC)w,background);return (LRESULT)background_brush;
    case WM_ERASEBKGND:if(settings_open||intro_active)return 1;{RECT r;GetClientRect(window,&r);FillRect((HDC)w,&r,background_brush);return 1;}
    case WM_PRINTCLIENT: {
        if(!settings_open){RECT r;GetClientRect(window,&r);FillRect((HDC)w,&r,background_brush);
         if(manuals_game>=0)paint_manuals((HDC)w,r);else paint_home((HDC)w);}return 0;
    }
    case WM_PAINT: {
        if(settings_open){ValidateRect(window,NULL);return 0;}
        PAINTSTRUCT p;HDC dc=BeginPaint(window,&p);
        if(intro_active)paint_intro(dc,window);
        else if(manuals_game>=0){RECT r;GetClientRect(window,&r);paint_manuals(dc,r);}else paint_home(dc);
        EndPaint(window,&p);return 0;
    }
    case WM_DESTROY:if(logo)image_dispose(logo);for(int i=0;i<2;i++)if(game_art[i])image_dispose(game_art[i]);for(int i=0;i<2;i++){if(box_art[i])image_dispose(box_art[i]);if(cart_art[i])image_dispose(cart_art[i]);}if(manual_page_image)image_dispose(manual_page_image);if(manual_page_image2)image_dispose(manual_page_image2);for(int i=0;i<2;i++)if(manual_books_loaded[i])dyna_manual_free(&manual_books[i]);if(image_token)image_shutdown(image_token);if(image_library)FreeLibrary(image_library);if(game_process)CloseHandle(game_process);DeleteObject(title_font);DeleteObject(body_font);DeleteObject(card_font);DeleteObject(background_brush);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR command,int show) {
    (void)previous;(void)command;
    int argc=0;wchar_t **argv=CommandLineToArgvW(GetCommandLineW(),&argc);wchar_t error[256];
    if(!argv || !dyna_launcher_options_parse(argc,(const wchar_t *const *)argv,&options,error,256)) {
        char message[1024]="Unable to read launch options.";
        if(argv){ld_wide_to_utf8(error,message,sizeof message);LocalFree(argv);}
        direct_launch=argc>1;
        return report_launch_error(NULL,2,message);
    }
    if(options.integration_info){
        const char *json=dyna_launcher_integration_info();DWORD written;
        HANDLE output=GetStdHandle(STD_OUTPUT_HANDLE);
        int ok=output && output!=INVALID_HANDLE_VALUE && WriteFile(output,json,(DWORD)strlen(json),&written,NULL);
        LocalFree(argv);return ok?0:1;
    }
    if(options.help){MessageBoxW(NULL,dyna_launcher_options_help(),L"Dyna Brothers",MB_OK);LocalFree(argv);return 0;}
    direct_launch=options.game!=0;
    DWORD dir_len=GetModuleFileNameW(NULL,exe_dir,LD_PATH_CAP);if(!dir_len||dir_len>=LD_PATH_CAP){LocalFree(argv);return 1;}
    wchar_t *last=wcsrchr(exe_dir,L'\\');if(!last){LocalFree(argv);return 1;}*last=0;
    /* Package layout: DynaBrothers.exe + data\ + roms\. When this launcher
     * runs from data\, ROMs (and mods) are discovered from the package root. */
    wcscpy(root_dir,exe_dir);
    {wchar_t *leaf=wcsrchr(root_dir,L'\\');if(leaf&&!_wcsicmp(leaf+1,L"data"))*leaf=0;else root_dir[0]=0;}
    char portable[8192],profile[DYNA_PROFILE_PATH_CAP];
    if(!ld_wide_to_utf8(exe_dir,portable,sizeof portable))portable[0]=0;
    DynaProfileChoice choice=dyna_profile_resolve(portable,profile,sizeof profile);
    if(choice==DYNA_PROFILE_UNAVAILABLE || !ld_utf8_to_wide(profile,profile_dir,LD_PATH_CAP) ||
       !SetEnvironmentVariableW(L"GENESIS_USER_DIR",profile_dir)) {
        LocalFree(argv);
        return report_launch_error(NULL,1,"Could not open a writable profile. Check GENESIS_USER_DIR or your local application data folder.");
    }
    launcher_log=dyna_log_open(profile,"launcher",NULL,0);
    log_launch(choice==DYNA_PROFILE_FALLBACK?"Launcher started with a per-user profile":"Launcher started");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0){
        int result=report_launch_error(NULL,1,"Could not initialize display and controllers.");
        if(launcher_log)fclose(launcher_log);LocalFree(argv);return result;
    }
    swprintf(config_path,LD_PATH_CAP,L"%ls\\settings.ini",profile_dir);
    for(int i=0;i<2;i++){
        swprintf(cache_path[i],LD_PATH_CAP,L"%ls\\rom-DynaBrothers%dAOT.cfg",profile_dir,i+1);
        FILE *file=_wfopen(cache_path[i],L"rb");if(file){char value[8192];if(fgets(value,sizeof value,file)){value[strcspn(value,"\r\n")]=0;ld_utf8_to_wide(value,cached_rom[i],LD_PATH_CAP);}fclose(file);}
    }
    WNDCLASSW type={0};type.lpfnWndProc=procedure;type.hInstance=instance;
    type.hCursor=LoadCursorW(NULL,IDC_ARROW);type.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));type.lpszClassName=L"DynaBrothersCheckpoint";
    if(!RegisterClassW(&type))return 1;
    dpi=GetDpiForSystem();
    int fw,fh;DynaHubRect bounds;frame_extents(dpi,&fw,&fh);
    dyna_hub_window_bounds(work_area(NULL),dpi,fw,fh,0,&bounds);
    HWND window=CreateWindowW(type.lpszClassName,L"Dyna Brothers - v" DYNA_WIDEN(DYNA_VERSION),WS_OVERLAPPEDWINDOW,
        bounds.x,bounds.y,bounds.w,bounds.h,NULL,NULL,instance,NULL);if(!window)return 1;
    if(direct_launch){
        direct_exit_code=(DWORD)launch(window,options.game-1);
        if(!game_process)DestroyWindow(window);
    }
    else {
        /* Borderless by default: the selector covers the monitor it opens on. */
        if(!getenv("DYNA_HUB_WINDOWED")){
            MONITORINFO mi={sizeof mi};GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&mi);
            SetWindowLongPtrW(window,GWL_STYLE,WS_POPUP|WS_CLIPCHILDREN);
            SetWindowPos(window,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,mi.rcMonitor.right-mi.rcMonitor.left,mi.rcMonitor.bottom-mi.rcMonitor.top,SWP_FRAMECHANGED);
            layout(window);show=SW_SHOW;
        }
        if(!getenv("LNG_TEST_HIDDEN"))ShowWindow(window,show);UpdateWindow(window);}
    if(getenv("DYNA_HUB_HOME_SELFTEST")&&!direct_launch)PostMessageW(window,WM_APP+9,0,0);
    MSG message;while(GetMessageW(&message,NULL,0,0)>0){
        if(intro_active&&(message.message==WM_KEYDOWN||message.message==WM_LBUTTONDOWN)){intro_finish(window);continue;}
        if(!settings_open&&message.message==WM_KEYDOWN&&message.wParam==VK_RETURN){
            int id=GetDlgCtrlID(GetFocus());if(id==101||id==102||id==201||id==202){SendMessageW(window,WM_COMMAND,id,0);continue;}
        }
        if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}
    }
    SDL_Quit();LocalFree(argv);if(launcher_log)fclose(launcher_log);return (int)direct_exit_code;
}
