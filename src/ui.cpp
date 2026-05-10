#include "ui.h"
#include <string>

namespace { constexpr int M=10, H=28; }
void InitListViewColumns(HWND list){
    const wchar_t* cols[]={L"序号",L"版本名称",L"文件名",L"大小",L"下载地址",L"状态"}; int w[]={60,140,190,100,360,120};
    for(int i=0;i<6;++i){ LVCOLUMNW c{}; c.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM; c.pszText=(LPWSTR)cols[i]; c.cx=w[i]; c.iSubItem=i; ListView_InsertColumn(list,i,&c);} }
void CreateMainUi(HWND hwnd,HINSTANCE h,UiHandles& ui){
    ui.font=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    ui.urlEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,0,0,0,0,hwnd,(HMENU)101,h,nullptr);
    ui.loadBtn=CreateWindowW(L"BUTTON",L"加载列表",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)102,h,nullptr);
    ui.loadLocalBtn=CreateWindowW(L"BUTTON",L"加载本地CSV",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)108,h,nullptr);
    ui.list=CreateWindowExW(WS_EX_CLIENTEDGE,WC_LISTVIEWW,L"",WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SINGLESEL,0,0,0,0,hwnd,(HMENU)103,h,nullptr);
    ListView_SetExtendedListViewStyle(ui.list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_INFOTIP);
    ui.downloadBtn=CreateWindowW(L"BUTTON",L"下载选中项",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)104,h,nullptr);
    ui.cancelBtn=CreateWindowW(L"BUTTON",L"取消下载",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)105,h,nullptr);
    ui.openDirBtn=CreateWindowW(L"BUTTON",L"打开下载目录",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)109,h,nullptr);
    ui.clearLogBtn=CreateWindowW(L"BUTTON",L"清空日志",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)110,h,nullptr);
    ui.progress=CreateWindowExW(0,PROGRESS_CLASSW,nullptr,WS_CHILD|WS_VISIBLE|PBS_SMOOTH,0,0,0,0,hwnd,(HMENU)106,h,nullptr);
    ui.speedText=CreateWindowW(L"STATIC",L"速度: 0 KB/s",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,h,nullptr);
    ui.sizeText=CreateWindowW(L"STATIC",L"大小: 0 / 0",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,h,nullptr);
    ui.logEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY|WS_VSCROLL,0,0,0,0,hwnd,(HMENU)107,h,nullptr);
    for(HWND c: {ui.urlEdit,ui.loadBtn,ui.loadLocalBtn,ui.list,ui.downloadBtn,ui.cancelBtn,ui.openDirBtn,ui.clearLogBtn,ui.progress,ui.speedText,ui.sizeText,ui.logEdit}) SendMessageW(c,WM_SETFONT,(WPARAM)ui.font,TRUE);
    InitListViewColumns(ui.list);
}
void ResizeMainUi(HWND hwnd,UiHandles& u){ RECT r{}; GetClientRect(hwnd,&r); int w=r.right,h=r.bottom;
    MoveWindow(u.urlEdit,M,M,w-360,H,TRUE); MoveWindow(u.loadBtn,w-340,M,100,H,TRUE); MoveWindow(u.loadLocalBtn,w-230,M,120,H,TRUE);
    MoveWindow(u.list,M,46,w-20,h-250,TRUE); int y=h-195; MoveWindow(u.downloadBtn,M,y,110,H,TRUE); MoveWindow(u.cancelBtn,125,y,95,H,TRUE); MoveWindow(u.openDirBtn,230,y,120,H,TRUE); MoveWindow(u.clearLogBtn,360,y,95,H,TRUE);
    MoveWindow(u.progress,465,y,w-475,H,TRUE); MoveWindow(u.speedText,M,h-158,220,22,TRUE); MoveWindow(u.sizeText,240,h-158,280,22,TRUE); MoveWindow(u.logEdit,M,h-132,w-20,122,TRUE); }
void AppendLogText(HWND logEdit,const std::wstring& line){ int len=GetWindowTextLengthW(logEdit); SendMessageW(logEdit,EM_SETSEL,len,len); SendMessageW(logEdit,EM_REPLACESEL,FALSE,(LPARAM)line.c_str()); SendMessageW(logEdit,EM_SCROLLCARET,0,0);} 
