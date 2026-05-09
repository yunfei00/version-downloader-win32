#pragma once

#include <commctrl.h>
#include <windows.h>

#include <string>
#include <vector>

#include "downloader.h"
#include "version_item.h"

class App {
public:
    App(HINSTANCE hInstance);
    int Run(int nCmdShow);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void CreateControls(HWND hwnd);
    void InitListView();
    void ResizeControls();
    void OnLoadList();
    void OnDownloadSelected();
    void OnCancelDownload();

    void AppendLog(const std::wstring& text);
    void SetRowStatus(int row, const std::wstring& status);
    void PopulateList();

    bool LoadTextFromPathOrUrl(const std::wstring& src, std::wstring& outText);
    bool ParseCsv(const std::wstring& csv);

    HINSTANCE hInstance_;
    HWND hwnd_ = nullptr;
    HWND urlEdit_ = nullptr;
    HWND loadButton_ = nullptr;
    HWND listView_ = nullptr;
    HWND downloadButton_ = nullptr;
    HWND cancelButton_ = nullptr;
    HWND progress_ = nullptr;
    HWND logEdit_ = nullptr;

    std::vector<VersionItem> items_;
    Downloader downloader_;
    int activeRow_ = -1;
};

constexpr UINT WM_APP_PROGRESS = WM_APP + 1;
constexpr UINT WM_APP_FINISH = WM_APP + 2;
