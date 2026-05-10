#pragma once
#include <commctrl.h>
#include <windows.h>

struct UiHandles { HWND urlEdit{}, loadBtn{}, loadLocalBtn{}, list{}, downloadBtn{}, cancelBtn{}, openDirBtn{}, clearLogBtn{}, progress{}, speedText{}, sizeText{}, logEdit{}; HFONT font{};};

void CreateMainUi(HWND hwnd, HINSTANCE h, UiHandles& ui);
void ResizeMainUi(HWND hwnd, UiHandles& ui);
void InitListViewColumns(HWND list);
void AppendLogText(HWND logEdit, const std::wstring& line);
