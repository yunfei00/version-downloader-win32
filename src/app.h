#pragma once
#include <windows.h>
#include <vector>
#include "version_item.h"
#include "ui.h"
#include "download_worker.h"

class App{public: explicit App(HINSTANCE h):h_(h){} int Run(int nCmdShow);
private: static LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM); LRESULT Handle(UINT,WPARAM,LPARAM);
 void LoadFromUrl(); void LoadFromLocal(); void Populate(); void SetStatus(int row,const std::wstring& s); void StartDownload(bool singleDoubleClick=false,int row=-1); void Cancel(); void OpenDownloads(); void SetUiBusy(bool b);
 void LoadConfig(); void SaveConfig(); bool ChooseDownloadDir(); std::vector<int> SelectedRows() const; bool SaveLogs(); void ClearStatuses(); void CopySelectedUrls(); bool ConfirmOverwrite(const std::wstring& path, bool& cancelAll, bool& skip);
 HINSTANCE h_; HWND hwnd_{}; UiHandles ui_{}; std::vector<VersionItem> items_; DownloadWorker worker_; std::wstring downloadDir_; std::vector<int> queue_; int queuePos_=0; unsigned long long sumDone_=0; unsigned long long sumTotal_=0; bool cancelling_=false;};
constexpr UINT WM_APP_PROGRESS = WM_APP + 1; constexpr UINT WM_APP_LOG = WM_APP + 2; constexpr UINT WM_APP_DONE = WM_APP + 3;
