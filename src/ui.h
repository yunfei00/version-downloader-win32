#pragma once
#include <commctrl.h>
#include <windows.h>

struct UiHandles {
    HWND urlEdit{}, loadBtn{}, loadLocalBtn{}, downloadDirEdit{}, chooseDirBtn{}, list{}, downloadBtn{}, cancelBtn{}, openDirBtn{}, clearLogBtn{}, progress{}, speedText{}, sizeText{}, fileText{}, etaText{}, logEdit{};
    HMENU listMenu{}, logMenu{};
    HFONT font{};
};

void CreateMainUi(HWND hwnd, HINSTANCE h, UiHandles& ui);
void ResizeMainUi(HWND hwnd, UiHandles& ui);
void InitListViewColumns(HWND list);
void AppendLogText(const std::wstring& line);
