#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

class Downloader {
public:
    using ProgressCallback = std::function<void(int)>;
    using FinishCallback = std::function<void(bool, const std::wstring&)>;

    Downloader();
    ~Downloader();

    void Start(const std::wstring& url, const std::wstring& savePath,
               ProgressCallback onProgress, FinishCallback onFinish);
    void Cancel();
    bool IsRunning() const;

private:
    void JoinWorker();

    std::thread worker_;
    std::atomic<bool> cancelRequested_{false};
    std::atomic<bool> running_{false};
};
