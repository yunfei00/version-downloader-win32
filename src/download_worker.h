#pragma once
#include <atomic>
#include <string>
#include <thread>
#include <windows.h>

struct DownloadProgressPayload { int row; int percent; double speedKB; unsigned long long downloaded; unsigned long long total; int fileIndex; int fileCount; std::wstring filename; };
struct DownloadDonePayload { int row; bool success; std::wstring message; unsigned long long downloaded; unsigned long long expected; bool cancelledAll; };

class DownloadWorker {
public:
    ~DownloadWorker();
    bool Start(HWND hwnd, int row, int fileIndex, int fileCount, const std::wstring& filename, const std::wstring& url, const std::wstring& savePath, UINT msgProgress, UINT msgLog, UINT msgDone);
    void Cancel(); bool IsRunning() const { return running_; }
private: void Join(); std::thread worker_; std::atomic<bool> cancel_{false}; std::atomic<bool> running_{false};
};
