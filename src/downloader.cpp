#include "downloader.h"

#include <windows.h>
#include <winhttp.h>

#include <vector>

#pragma comment(lib, "winhttp.lib")

Downloader::Downloader() = default;

Downloader::~Downloader() {
    Cancel();
    JoinWorker();
}

void Downloader::Start(const std::wstring& url, const std::wstring& savePath,
                       ProgressCallback onProgress, FinishCallback onFinish) {
    if (running_) {
        return;
    }

    cancelRequested_ = false;
    running_ = true;
    JoinWorker();

    worker_ = std::thread([=]() {
        bool success = false;
        std::wstring errorMsg;

        URL_COMPONENTS uc{};
        uc.dwStructSize = sizeof(uc);
        wchar_t host[256] = {};
        wchar_t path[2048] = {};
        uc.lpszHostName = host;
        uc.dwHostNameLength = static_cast<DWORD>(std::size(host));
        uc.lpszUrlPath = path;
        uc.dwUrlPathLength = static_cast<DWORD>(std::size(path));

        if (!WinHttpCrackUrl(url.c_str(), 0, 0, &uc)) {
            errorMsg = L"URL 解析失败";
            goto done;
        }

        {
            HANDLE hFile = CreateFileW(savePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile == INVALID_HANDLE_VALUE) {
                errorMsg = L"无法创建输出文件";
                goto done;
            }

            HINTERNET hSession = WinHttpOpen(L"VersionDownloader/1.0",
                                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                             WINHTTP_NO_PROXY_NAME,
                                             WINHTTP_NO_PROXY_BYPASS, 0);
            if (!hSession) {
                CloseHandle(hFile);
                errorMsg = L"WinHTTP 会话创建失败";
                goto done;
            }

            HINTERNET hConnect = WinHttpConnect(hSession, uc.lpszHostName, uc.nPort, 0);
            if (!hConnect) {
                CloseHandle(hFile);
                WinHttpCloseHandle(hSession);
                errorMsg = L"连接服务器失败";
                goto done;
            }

            DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
            HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", uc.lpszUrlPath,
                                                    nullptr, WINHTTP_NO_REFERER,
                                                    WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
            if (!hRequest) {
                CloseHandle(hFile);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                errorMsg = L"创建请求失败";
                goto done;
            }

            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(hRequest, nullptr)) {
                CloseHandle(hFile);
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                errorMsg = L"发送请求失败";
                goto done;
            }

            ULONGLONG totalSize = 0;
            DWORD sizeLen = sizeof(totalSize);
            WinHttpQueryHeaders(hRequest,
                                WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX, &totalSize, &sizeLen,
                                WINHTTP_NO_HEADER_INDEX);

            ULONGLONG downloaded = 0;
            std::vector<std::byte> buffer(64 * 1024);

            while (!cancelRequested_) {
                DWORD available = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &available)) {
                    errorMsg = L"读取数据失败";
                    break;
                }
                if (available == 0) {
                    success = true;
                    break;
                }

                DWORD toRead = (available > buffer.size()) ? static_cast<DWORD>(buffer.size()) : available;
                DWORD read = 0;
                if (!WinHttpReadData(hRequest, buffer.data(), toRead, &read) || read == 0) {
                    errorMsg = L"下载中断";
                    break;
                }

                DWORD written = 0;
                if (!WriteFile(hFile, buffer.data(), read, &written, nullptr) || written != read) {
                    errorMsg = L"写入文件失败";
                    break;
                }

                downloaded += read;
                if (totalSize > 0) {
                    int progress = static_cast<int>((downloaded * 100) / totalSize);
                    onProgress(progress);
                }
            }

            if (cancelRequested_) {
                errorMsg = L"已取消";
            }

            CloseHandle(hFile);
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
        }

    done:
        running_ = false;
        onFinish(success, errorMsg);
    });
}

void Downloader::Cancel() {
    cancelRequested_ = true;
}

bool Downloader::IsRunning() const {
    return running_;
}

void Downloader::JoinWorker() {
    if (worker_.joinable()) {
        worker_.join();
    }
}
