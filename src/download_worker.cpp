#include "download_worker.h"
#include "logger.h"
#include <windows.h>
#include <winhttp.h>
#include <vector>
#include <filesystem>

#pragma comment(lib, "winhttp.lib")

DownloadWorker::~DownloadWorker(){ Cancel(); Join(); }
void DownloadWorker::Join(){ if(worker_.joinable()) worker_.join(); }
void DownloadWorker::Cancel(){ cancel_=true; }

bool DownloadWorker::Start(HWND hwnd, int row, const std::wstring& url, const std::wstring& savePath, UINT msgProgress, UINT msgLog, UINT msgDone) {
    if (running_) return false; Join(); cancel_=false; running_=true;
    worker_=std::thread([=, this]{
        auto postLog=[&](const std::wstring& m){ PostMessageW(hwnd,msgLog,0,reinterpret_cast<LPARAM>(new std::wstring(BuildLogLine(m)))); };
        postLog(L"开始下载: "+url);
        std::filesystem::create_directories(std::filesystem::path(savePath).parent_path());
        URL_COMPONENTS uc{}; uc.dwStructSize=sizeof(uc); wchar_t host[256]={}, path[2048]={};
        uc.lpszHostName=host; uc.dwHostNameLength=256; uc.lpszUrlPath=path; uc.dwUrlPathLength=2048;
        DownloadDonePayload* done = new DownloadDonePayload{row,false,L"失败"};
        if(!WinHttpCrackUrl(url.c_str(),0,0,&uc)){done->message=L"URL解析失败"; goto finish;}
        {
            HANDLE hf=CreateFileW(savePath.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(hf==INVALID_HANDLE_VALUE){done->message=L"无法创建文件"; goto finish;}
            HINTERNET s=WinHttpOpen(L"VersionDownloader/2.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
            HINTERNET c=s?WinHttpConnect(s,uc.lpszHostName,uc.nPort,0):nullptr;
            DWORD flags=(uc.nScheme==INTERNET_SCHEME_HTTPS)?WINHTTP_FLAG_SECURE:0;
            HINTERNET r=c?WinHttpOpenRequest(c,L"GET",uc.lpszUrlPath,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,flags):nullptr;
            if(!r||!WinHttpSendRequest(r,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(r,nullptr)){done->message=L"HTTP请求失败"; if(r)WinHttpCloseHandle(r); if(c)WinHttpCloseHandle(c); if(s)WinHttpCloseHandle(s); CloseHandle(hf); goto finish;}
            unsigned long long total=0; DWORD len=sizeof(total); WinHttpQueryHeaders(r,WINHTTP_QUERY_CONTENT_LENGTH|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&total,&len,WINHTTP_NO_HEADER_INDEX);
            unsigned long long downloaded=0; auto start=GetTickCount64(); std::vector<unsigned char> buf(64*1024);
            while(!cancel_){ DWORD avail=0; if(!WinHttpQueryDataAvailable(r,&avail)){done->message=L"读取失败"; break;} if(avail==0){done->success=true; done->message=L"下载完成"; break;}
                DWORD rd=0; DWORD toRead=min<DWORD>(avail,(DWORD)buf.size()); if(!WinHttpReadData(r,buf.data(),toRead,&rd)||rd==0){done->message=L"下载中断"; break;}
                DWORD wr=0; if(!WriteFile(hf,buf.data(),rd,&wr,nullptr)||wr!=rd){done->message=L"写入失败"; break;}
                downloaded+=rd; int p= total? (int)(downloaded*100/total):0; double sec=(GetTickCount64()-start)/1000.0; double kb=sec>0?(downloaded/1024.0/sec):0;
                PostMessageW(hwnd,msgProgress,reinterpret_cast<WPARAM>(new DownloadProgressPayload{row,p,kb,downloaded,total}),0);
            }
            if(cancel_){done->message=L"已取消";}
            CloseHandle(hf); WinHttpCloseHandle(r); WinHttpCloseHandle(c); WinHttpCloseHandle(s);
        }
finish:
        PostMessageW(hwnd,msgDone,reinterpret_cast<WPARAM>(done),0);
        running_=false;
    });
    return true;
}
