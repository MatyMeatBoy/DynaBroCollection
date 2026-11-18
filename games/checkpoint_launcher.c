#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <wchar.h>

static HWND cards[2],status;
static HFONT title_font,body_font,card_font;
static HBRUSH background_brush;
#define DYNA_WIDEN_INNER(x) L##x
#define DYNA_WIDEN(x) DYNA_WIDEN_INNER(x)
static HANDLE game_process;
static UINT dpi=96;
static int scale(int x){return MulDiv(x,(int)dpi,96);}
static COLORREF background=RGB(9,17,27),text_color=RGB(231,241,251),muted=RGB(153,177,200);
static const wchar_t *game_names[]={L"Dyna Brothers",L"Dyna Brothers 2 Special"};
static void fonts(void) {
    if(title_font)DeleteObject(title_font);if(body_font)DeleteObject(body_font);if(card_font)DeleteObject(card_font);
    title_font=CreateFontW(-scale(34),0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    card_font=CreateFontW(-scale(25),0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
    body_font=CreateFontW(-scale(16),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
}
static void layout(HWND window) {
    RECT r;GetClientRect(window,&r);int margin=scale(28),gap=scale(20),width=(r.right-margin*2-gap)/2;
    MoveWindow(cards[0],margin,scale(140),width,scale(220),TRUE);
    MoveWindow(cards[1],margin+width+gap,scale(140),width,scale(220),TRUE);
    MoveWindow(status,margin,r.bottom-scale(45),r.right-margin*2,scale(25),TRUE);
    SendMessageW(status,WM_SETFONT,(WPARAM)body_font,TRUE);InvalidateRect(window,NULL,TRUE);
}
static void draw_text(HDC dc,const wchar_t *text,RECT r,HFONT font,COLORREF color,UINT flags) {
    SelectObject(dc,font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,text,-1,&r,flags|DT_NOPREFIX);
}
static void launch(HWND window,int index) {
    if(game_process)return;
    wchar_t path[2048],directory[2048];DWORD length=GetModuleFileNameW(NULL,directory,2048);
    if(!length||length>=2048)return;
    wchar_t *slash=wcsrchr(directory,L'\\');if(!slash)return;slash[1]=0;
    if(swprintf(path,2048,L"%ls%ls",directory,index?L"DynaBrothers2AOT.exe":L"DynaBrothers1AOT.exe")<0)return;
    STARTUPINFOW startup={0};startup.cb=sizeof startup;PROCESS_INFORMATION process={0};
    if(!CreateProcessW(path,NULL,NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,directory,&startup,&process)) {
        MessageBoxW(window,L"The game could not be started. Keep the checkpoint files and runtime libraries together.",L"Dyna Brothers",MB_OK|MB_ICONERROR);return;
    }
    CloseHandle(process.hThread);game_process=process.hProcess;
    EnableWindow(cards[0],FALSE);EnableWindow(cards[1],FALSE);
    SetWindowTextW(status,L"Game running. Close its window to return here.");SetTimer(window,1,250,NULL);
}
static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_CREATE:
        background_brush=CreateSolidBrush(background);
        dpi=GetDpiForWindow(window);fonts();
        for(int i=0;i<2;++i)cards[i]=CreateWindowW(L"BUTTON",game_names[i],WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            0,0,0,0,window,(HMENU)(INT_PTR)(101+i),GetModuleHandleW(NULL),NULL);
        status=CreateWindowW(L"STATIC",L"Choose a game",WS_CHILD|WS_VISIBLE,0,0,0,0,window,NULL,GetModuleHandleW(NULL),NULL);
        layout(window);SetFocus(cards[0]);return 0;
    case WM_SIZE:layout(window);return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)l)->ptMinTrackSize.x=scale(800);((MINMAXINFO *)l)->ptMinTrackSize.y=scale(540);return 0;
    case WM_DPICHANGED:
        dpi=HIWORD(w);fonts();{RECT *r=(RECT *)l;SetWindowPos(window,NULL,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);}layout(window);return 0;
    case WM_COMMAND:
        if(LOWORD(w)==101||LOWORD(w)==102){launch(window,LOWORD(w)-101);return 0;}break;
    case WM_TIMER:
        if(game_process&&WaitForSingleObject(game_process,0)==WAIT_OBJECT_0) {
            CloseHandle(game_process);game_process=NULL;KillTimer(window,1);
            EnableWindow(cards[0],TRUE);EnableWindow(cards[1],TRUE);SetWindowTextW(status,L"Choose a game");SetFocus(cards[0]);
        }return 0;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *item=(DRAWITEMSTRUCT *)l;int index=(int)item->CtlID-101;if(index<0||index>1)break;
        RECT r=item->rcItem;int selected=(item->itemState&(ODS_FOCUS|ODS_SELECTED))!=0;
        FillRect(item->hDC,&r,background_brush);
        HBRUSH brush=CreateSolidBrush(selected?RGB(23,51,74):RGB(18,33,48));
        HPEN pen=CreatePen(PS_SOLID,scale(selected?2:1),selected?RGB(68,190,235):RGB(48,75,98));
        HGDIOBJ old_brush=SelectObject(item->hDC,brush),old_pen=SelectObject(item->hDC,pen);
        RoundRect(item->hDC,r.left+2,r.top+2,r.right-2,r.bottom-2,scale(30),scale(30));
        SelectObject(item->hDC,old_brush);SelectObject(item->hDC,old_pen);DeleteObject(brush);DeleteObject(pen);
        r.left+=scale(24);r.right-=scale(20);r.top+=scale(24);r.bottom=r.top+scale(76);
        draw_text(item->hDC,index?L"Dyna Brothers 2\nSpecial":L"Dyna Brothers",r,card_font,text_color,DT_LEFT|DT_WORDBREAK);
        r.top=scale(120);r.bottom=scale(150);draw_text(item->hDC,index?L"Special edition":L"Original game",r,body_font,muted,DT_LEFT);
        r.top=scale(171);r.bottom=scale(200);draw_text(item->hDC,L"Open game  >",r,body_font,RGB(90,205,245),DT_LEFT);return TRUE;
    }
    case WM_CTLCOLORSTATIC:SetTextColor((HDC)w,muted);SetBkColor((HDC)w,background);return (LRESULT)background_brush;
    case WM_ERASEBKGND:{RECT r;GetClientRect(window,&r);FillRect((HDC)w,&r,background_brush);return 1;}
    case WM_PAINT: {
        PAINTSTRUCT p;HDC dc=BeginPaint(window,&p);RECT r;GetClientRect(window,&r);r.left=scale(28);r.top=scale(24);r.bottom=scale(75);
        draw_text(dc,L"Dyna Brothers",r,title_font,text_color,DT_LEFT);
        r.top=scale(83);r.bottom=scale(112);draw_text(dc,L"Checkpoint v" DYNA_WIDEN(DYNA_VERSION),r,body_font,RGB(90,205,245),DT_LEFT);
        r.top=scale(386);r.bottom=scale(440);r.right-=scale(28);
        draw_text(dc,L"English is enabled by default. Language and controls are in each game's settings.\nTranslations are still in progress. Use your own Japanese game dumps.",r,body_font,muted,DT_LEFT|DT_WORDBREAK);
        EndPaint(window,&p);return 0;
    }
    case WM_DESTROY:if(game_process)CloseHandle(game_process);DeleteObject(title_font);DeleteObject(body_font);DeleteObject(card_font);DeleteObject(background_brush);PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR command,int show) {
    (void)previous;(void)command;WNDCLASSW type={0};type.lpfnWndProc=procedure;type.hInstance=instance;
    type.hCursor=LoadCursorW(NULL,IDC_ARROW);type.lpszClassName=L"DynaBrothersCheckpoint";
    if(!RegisterClassW(&type))return 1;
    dpi=GetDpiForSystem();
    HWND window=CreateWindowW(type.lpszClassName,L"Dyna Brothers - v" DYNA_WIDEN(DYNA_VERSION),WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,scale(900),scale(540),NULL,NULL,instance,NULL);if(!window)return 1;
    ShowWindow(window,show);UpdateWindow(window);
    MSG message;while(GetMessageW(&message,NULL,0,0)>0){if(!IsDialogMessageW(window,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    return 0;
}
