#include "app.h"

#include <shlwapi.h>
#include <urlmon.h>

#include <sstream>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shlwapi.lib")

namespace {
constexpr int IDC_URL = 101;
constexpr int IDC_LOAD = 102;
constexpr int IDC_LIST = 103;
constexpr int IDC_DOWNLOAD = 104;
constexpr int IDC_CANCEL = 105;
constexpr int IDC_PROGRESS = 106;
constexpr int IDC_LOG = 107;
}

App::App(HINSTANCE hInstance) : hInstance_(hInstance) {}

int App::Run(int nCmdShow) {
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = App::WndProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = L"VersionDownloaderWnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(0, wc.lpszClassName, L"Version Downloader", WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, 900, 600,
                            nullptr, nullptr, hInstance_, this);
    if (!hwnd_) return 1;

    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    App* app = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (msg == WM_NCCREATE) {
        auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }
    return app ? app->HandleMessage(hwnd, msg, wParam, lParam) : DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT App::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: CreateControls(hwnd); return 0;
        case WM_SIZE: ResizeControls(); return 0;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_LOAD: OnLoadList(); break;
                case IDC_DOWNLOAD: OnDownloadSelected(); break;
                case IDC_CANCEL: OnCancelDownload(); break;
                default: break;
            }
            return 0;
        case WM_APP_PROGRESS: {
            int progress = static_cast<int>(wParam);
            SendMessageW(progress_, PBM_SETPOS, progress, 0);
            if (activeRow_ >= 0) {
                SetRowStatus(activeRow_, L"下载中 " + std::to_wstring(progress) + L"%");
            }
            return 0;
        }
        case WM_APP_FINISH: {
            auto* ok = reinterpret_cast<bool*>(wParam);
            auto* msgText = reinterpret_cast<std::wstring*>(lParam);
            if (*ok) {
                AppendLog(L"下载完成");
                if (activeRow_ >= 0) SetRowStatus(activeRow_, L"已完成");
                SendMessageW(progress_, PBM_SETPOS, 100, 0);
            } else {
                if (*msgText == L"已取消") {
                    AppendLog(L"下载已取消");
                    if (activeRow_ >= 0) SetRowStatus(activeRow_, L"已取消");
                } else {
                    AppendLog(L"下载失败: " + *msgText);
                    if (activeRow_ >= 0) SetRowStatus(activeRow_, L"失败");
                }
            }
            delete ok;
            delete msgText;
            activeRow_ = -1;
            return 0;
        }
        case WM_DESTROY: PostQuitMessage(0); return 0;
        default: return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

void App::CreateControls(HWND hwnd) {
    urlEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                               0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_URL), hInstance_, nullptr);
    loadButton_ = CreateWindowW(L"BUTTON", L"加载版本列表", WS_CHILD | WS_VISIBLE,
                                0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_LOAD), hInstance_, nullptr);
    listView_ = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_LIST), hInstance_, nullptr);
    ListView_SetExtendedListViewStyle(listView_, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    downloadButton_ = CreateWindowW(L"BUTTON", L"下载选中项", WS_CHILD | WS_VISIBLE,
                                    0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_DOWNLOAD), hInstance_, nullptr);
    cancelButton_ = CreateWindowW(L"BUTTON", L"取消下载", WS_CHILD | WS_VISIBLE,
                                  0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_CANCEL), hInstance_, nullptr);
    progress_ = CreateWindowExW(0, PROGRESS_CLASSW, nullptr, WS_CHILD | WS_VISIBLE,
                                0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_PROGRESS), hInstance_, nullptr);
    logEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                               WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                               0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(IDC_LOG), hInstance_, nullptr);
    InitListView();
}

void App::InitListView() {
    const wchar_t* cols[] = {L"序号", L"版本名称", L"文件名", L"大小", L"下载地址", L"状态"};
    int widths[] = {60, 120, 180, 100, 320, 100};
    for (int i = 0; i < 6; ++i) {
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.cx = widths[i];
        col.pszText = const_cast<wchar_t*>(cols[i]);
        col.iSubItem = i;
        ListView_InsertColumn(listView_, i, &col);
    }
}

void App::ResizeControls() {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    MoveWindow(urlEdit_, 10, 10, w - 140, 28, TRUE);
    MoveWindow(loadButton_, w - 120, 10, 110, 28, TRUE);
    MoveWindow(listView_, 10, 50, w - 20, h - 250, TRUE);
    MoveWindow(downloadButton_, 10, h - 190, 120, 30, TRUE);
    MoveWindow(cancelButton_, 140, h - 190, 100, 30, TRUE);
    MoveWindow(progress_, 250, h - 190, w - 260, 30, TRUE);
    MoveWindow(logEdit_, 10, h - 150, w - 20, 140, TRUE);
}

void App::AppendLog(const std::wstring& text) {
    int len = GetWindowTextLengthW(logEdit_);
    SendMessageW(logEdit_, EM_SETSEL, len, len);
    std::wstring line = text + L"\r\n";
    SendMessageW(logEdit_, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));
}

bool App::LoadTextFromPathOrUrl(const std::wstring& src, std::wstring& outText) {
    if (PathIsURLW(src.c_str())) {
        wchar_t tempPath[MAX_PATH] = {};
        wchar_t tempFile[MAX_PATH] = {};
        GetTempPathW(MAX_PATH, tempPath);
        GetTempFileNameW(tempPath, L"vdl", 0, tempFile);

        HRESULT hr = URLDownloadToFileW(nullptr, src.c_str(), tempFile, 0, nullptr);
        if (FAILED(hr)) return false;

        HANDLE hFile = CreateFileW(tempFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;
        DWORD size = GetFileSize(hFile, nullptr);
        std::string buf(size, '\0');
        DWORD read = 0;
        ReadFile(hFile, buf.data(), size, &read, nullptr);
        CloseHandle(hFile);
        DeleteFileW(tempFile);

        int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), read, nullptr, 0);
        if (wlen <= 0) return false;
        outText.resize(wlen);
        MultiByteToWideChar(CP_UTF8, 0, buf.data(), read, outText.data(), wlen);
        return true;
    }

    HANDLE hFile = CreateFileW(src.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;
    DWORD size = GetFileSize(hFile, nullptr);
    std::string buf(size, '\0');
    DWORD read = 0;
    ReadFile(hFile, buf.data(), size, &read, nullptr);
    CloseHandle(hFile);

    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), read, nullptr, 0);
    if (wlen <= 0) return false;
    outText.resize(wlen);
    MultiByteToWideChar(CP_UTF8, 0, buf.data(), read, outText.data(), wlen);
    return true;
}

bool App::ParseCsv(const std::wstring& csv) {
    std::wstringstream ss(csv);
    std::wstring line;
    items_.clear();

    if (!std::getline(ss, line)) return false;
    int idx = 1;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == L'\r') line.pop_back();
        if (line.empty()) continue;
        std::wstringstream ls(line);
        std::wstring cell;
        std::vector<std::wstring> cols;
        while (std::getline(ls, cell, L',')) cols.push_back(cell);
        if (cols.size() < 4) continue;

        VersionItem item;
        item.index = idx++;
        item.name = cols[0];
        item.filename = cols[1];
        item.size = cols[2];
        item.url = cols[3];
        item.status = L"等待下载";
        items_.push_back(item);
    }
    return !items_.empty();
}

void App::PopulateList() {
    ListView_DeleteAllItems(listView_);
    for (size_t i = 0; i < items_.size(); ++i) {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);
        std::wstring idx = std::to_wstring(items_[i].index);
        item.pszText = idx.data();
        ListView_InsertItem(listView_, &item);
        ListView_SetItemText(listView_, i, 1, items_[i].name.data());
        ListView_SetItemText(listView_, i, 2, items_[i].filename.data());
        ListView_SetItemText(listView_, i, 3, items_[i].size.data());
        ListView_SetItemText(listView_, i, 4, items_[i].url.data());
        ListView_SetItemText(listView_, i, 5, items_[i].status.data());
    }
}

void App::SetRowStatus(int row, const std::wstring& status) {
    if (row < 0 || row >= static_cast<int>(items_.size())) return;
    items_[row].status = status;
    ListView_SetItemText(listView_, row, 5, const_cast<wchar_t*>(status.c_str()));
}

void App::OnLoadList() {
    AppendLog(L"开始加载版本列表");
    wchar_t src[2048] = {};
    GetWindowTextW(urlEdit_, src, 2048);
    std::wstring text;
    if (!LoadTextFromPathOrUrl(src, text) || !ParseCsv(text)) {
        AppendLog(L"加载失败");
        return;
    }
    PopulateList();
    AppendLog(L"加载成功");
}

void App::OnDownloadSelected() {
    if (downloader_.IsRunning()) return;
    int row = ListView_GetNextItem(listView_, -1, LVNI_SELECTED);
    if (row < 0 || row >= static_cast<int>(items_.size())) return;

    wchar_t path[MAX_PATH] = {};
    wcscpy_s(path, items_[row].filename.c_str());
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd_;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files\0*.*\0";
    ofn.Flags = OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn)) return;

    AppendLog(L"开始下载");
    SendMessageW(progress_, PBM_SETPOS, 0, 0);
    activeRow_ = row;
    SetRowStatus(row, L"下载中 0%");

    std::wstring url = items_[row].url;
    std::wstring save = path;
    downloader_.Start(url, save,
        [this](int p) {
            PostMessageW(hwnd_, WM_APP_PROGRESS, static_cast<WPARAM>(p), 0);
            AppendLog(L"下载进度: " + std::to_wstring(p) + L"%");
        },
        [this](bool success, const std::wstring& msg) {
            auto* ok = new bool(success);
            auto* m = new std::wstring(msg);
            PostMessageW(hwnd_, WM_APP_FINISH, reinterpret_cast<WPARAM>(ok), reinterpret_cast<LPARAM>(m));
        });
}

void App::OnCancelDownload() {
    if (!downloader_.IsRunning()) return;
    downloader_.Cancel();
    AppendLog(L"请求取消下载");
}
