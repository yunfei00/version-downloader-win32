#include "pch.h"
#include "ui.h"
#include <string>

namespace { constexpr int M=10, H=28; HWND g_logEdit = nullptr; }
void InitListViewColumns(HWND list){
    const wchar_t* cols[]={L"序号",L"版本名称",L"文件名",L"大小",L"下载地址",L"状态"}; int w[]={60,180,220,110,600,120};
    for(int i=0;i<6;++i){ LVCOLUMNW c{}; c.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM; c.pszText=(LPWSTR)cols[i]; c.cx=w[i]; c.iSubItem=i; ListView_InsertColumn(list,i,&c);} }
void CreateMainUi(HWND hwnd,HINSTANCE h,UiHandles& ui){
    ui.font=CreateFontW(-18,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    ui.urlEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,0,0,0,0,hwnd,(HMENU)101,h,nullptr);
    ui.loadBtn=CreateWindowW(L"BUTTON",L"加载列表",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)102,h,nullptr);
    ui.loadLocalBtn=CreateWindowW(L"BUTTON",L"加载本地CSV",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)108,h,nullptr);
    ui.downloadDirEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL,0,0,0,0,hwnd,(HMENU)111,h,nullptr);
    ui.chooseDirBtn=CreateWindowW(L"BUTTON",L"选择目录",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)112,h,nullptr);
    ui.list=CreateWindowExW(WS_EX_CLIENTEDGE,WC_LISTVIEWW,L"",WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,0,0,0,0,hwnd,(HMENU)103,h,nullptr);
    ListView_SetExtendedListViewStyle(ui.list,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_INFOTIP);
    ui.downloadBtn=CreateWindowW(L"BUTTON",L"下载选中项",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)104,h,nullptr);
    ui.cancelBtn=CreateWindowW(L"BUTTON",L"取消下载",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)105,h,nullptr);
    ui.openDirBtn=CreateWindowW(L"BUTTON",L"打开下载目录",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)109,h,nullptr);
    ui.clearLogBtn=CreateWindowW(L"BUTTON",L"清空日志",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,(HMENU)110,h,nullptr);
    ui.progress=CreateWindowExW(0,PROGRESS_CLASSW,nullptr,WS_CHILD|WS_VISIBLE|PBS_SMOOTH,0,0,0,0,hwnd,(HMENU)106,h,nullptr);
    ui.fileText=CreateWindowW(L"STATIC",L"当前文件: -",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,h,nullptr);
    ui.speedText=CreateWindowW(L"STATIC",L"速度: 0 KB/s",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,h,nullptr);
    ui.sizeText=CreateWindowW(L"STATIC",L"大小: 0 / 未知",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,h,nullptr);
    ui.etaText=CreateWindowW(L"STATIC",L"预计剩余: --",WS_CHILD|WS_VISIBLE,0,0,0,0,hwnd,nullptr,h,nullptr);
    ui.logEdit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_AUTOVSCROLL|ES_READONLY|WS_VSCROLL,0,0,0,0,hwnd,(HMENU)107,h,nullptr);
    g_logEdit = ui.logEdit;
    for(HWND c: {ui.urlEdit,ui.loadBtn,ui.loadLocalBtn,ui.downloadDirEdit,ui.chooseDirBtn,ui.list,ui.downloadBtn,ui.cancelBtn,ui.openDirBtn,ui.clearLogBtn,ui.progress,ui.fileText,ui.speedText,ui.sizeText,ui.etaText,ui.logEdit}) SendMessageW(c,WM_SETFONT,(WPARAM)ui.font,TRUE);
    InitListViewColumns(ui.list);
    ui.listMenu=CreatePopupMenu(); AppendMenuW(ui.listMenu,MF_STRING,1201,L"下载选中项"); AppendMenuW(ui.listMenu,MF_STRING,1202,L"复制下载地址"); AppendMenuW(ui.listMenu,MF_STRING,1203,L"打开下载目录"); AppendMenuW(ui.listMenu,MF_STRING,1204,L"清空状态");
    ui.logMenu=CreatePopupMenu(); AppendMenuW(ui.logMenu,MF_STRING,1301,L"复制日志"); AppendMenuW(ui.logMenu,MF_STRING,1302,L"清空日志"); AppendMenuW(ui.logMenu,MF_STRING,1303,L"保存日志到文件");
}
void ResizeMainUi(HWND hwnd,UiHandles& u){ RECT r{}; GetClientRect(hwnd,&r); int w=r.right,h=r.bottom;
    MoveWindow(u.urlEdit,M,M,w-520,H,TRUE); MoveWindow(u.loadBtn,w-500,M,100,H,TRUE); MoveWindow(u.loadLocalBtn,w-390,M,120,H,TRUE);
    MoveWindow(u.downloadDirEdit,M,46,w-280,H,TRUE); MoveWindow(u.chooseDirBtn,w-260,46,110,H,TRUE); MoveWindow(u.openDirBtn,w-140,46,130,H,TRUE);
    MoveWindow(u.list,M,82,w-20,h-300,TRUE); int y=h-210; MoveWindow(u.downloadBtn,M,y,120,H,TRUE); MoveWindow(u.cancelBtn,138,y,95,H,TRUE); MoveWindow(u.clearLogBtn,241,y,95,H,TRUE);
    MoveWindow(u.progress,345,y,w-355,H,TRUE); MoveWindow(u.fileText,M,h-176,w-20,20,TRUE); MoveWindow(u.speedText,M,h-154,220,20,TRUE); MoveWindow(u.sizeText,240,h-154,320,20,TRUE); MoveWindow(u.etaText,570,h-154,220,20,TRUE); MoveWindow(u.logEdit,M,h-132,w-20,122,TRUE); }
void AppendLogText(const std::wstring& line){ if(!g_logEdit) return; int len=GetWindowTextLengthW(g_logEdit); SendMessageW(g_logEdit,EM_SETSEL,len,len); SendMessageW(g_logEdit,EM_REPLACESEL,FALSE,(LPARAM)line.c_str()); SendMessageW(g_logEdit,EM_SCROLLCARET,0,0);} 
