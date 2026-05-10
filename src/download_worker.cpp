#include "download_worker.h"
#include "logger.h"
#include <windows.h>
#include <winhttp.h>
#include <filesystem>
#include <vector>
#pragma comment(lib, "winhttp.lib")

DownloadWorker::~DownloadWorker(){ Cancel(); Join(); }
void DownloadWorker::Join(){ if(worker_.joinable()) worker_.join(); }
void DownloadWorker::Cancel(){ cancel_=true; }

bool DownloadWorker::Start(HWND hwnd, int row, int fileIndex, int fileCount, const std::wstring& filename, const std::wstring& url, const std::wstring& savePath, UINT msgProgress, UINT msgLog, UINT msgDone) {
    if (running_) return false; Join(); cancel_=false; running_=true;
    worker_=std::thread([=, this]{
        auto done = new DownloadDonePayload{row,false,L"失败",0,0,false};
        std::wstring partPath = savePath + L".part";
        URL_COMPONENTS uc{}; uc.dwStructSize=sizeof(uc); wchar_t host[256]={}, path[2048]={};
        uc.lpszHostName=host; uc.dwHostNameLength=256; uc.lpszUrlPath=path; uc.dwUrlPathLength=2048;
        if(url.empty()){ done->message=L"URL为空"; goto finish; }
        if(!WinHttpCrackUrl(url.c_str(),0,0,&uc)){done->message=L"URL解析失败"; goto finish;}
        std::filesystem::create_directories(std::filesystem::path(savePath).parent_path());
        {
            HANDLE hf=CreateFileW(partPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(hf==INVALID_HANDLE_VALUE){done->message=L"文件无法写入"; goto finish;}
            HINTERNET s=WinHttpOpen(L"VersionDownloader/2.1",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
            HINTERNET c=s?WinHttpConnect(s,uc.lpszHostName,uc.nPort,0):nullptr;
            DWORD flags=(uc.nScheme==INTERNET_SCHEME_HTTPS)?WINHTTP_FLAG_SECURE:0;
            HINTERNET r=c?WinHttpOpenRequest(c,L"GET",uc.lpszUrlPath,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,flags):nullptr;
            if(!r||!WinHttpSendRequest(r,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)||!WinHttpReceiveResponse(r,nullptr)){done->message=L"网络连接失败"; if(r)WinHttpCloseHandle(r); if(c)WinHttpCloseHandle(c); if(s)WinHttpCloseHandle(s); CloseHandle(hf); DeleteFileW(partPath.c_str()); goto finish;}
            DWORD st=0,stLen=sizeof(st); WinHttpQueryHeaders(r,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&st,&stLen,WINHTTP_NO_HEADER_INDEX);
            if(st!=200){done->message=L"HTTP状态码不是200"; CloseHandle(hf); WinHttpCloseHandle(r); WinHttpCloseHandle(c); WinHttpCloseHandle(s); DeleteFileW(partPath.c_str()); goto finish;}
            unsigned long long total=0; DWORD len=sizeof(total); WinHttpQueryHeaders(r,WINHTTP_QUERY_CONTENT_LENGTH|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&total,&len,WINHTTP_NO_HEADER_INDEX); done->expected=total;
            unsigned long long downloaded=0; auto start=GetTickCount64(); std::vector<unsigned char> buf(64*1024);
            while(!cancel_){ DWORD avail=0; if(!WinHttpQueryDataAvailable(r,&avail)){done->message=L"服务器断开"; break;} if(avail==0){done->success=true; done->message=L"下载完成"; break;}
                DWORD rd=0; DWORD toRead=min<DWORD>(avail,(DWORD)buf.size()); if(!WinHttpReadData(r,buf.data(),toRead,&rd)||rd==0){done->message=L"下载中断"; break;}
                DWORD wr=0; if(!WriteFile(hf,buf.data(),rd,&wr,nullptr)||wr!=rd){done->message=L"写入失败"; break;}
                downloaded+=rd; done->downloaded=downloaded; int p= total? (int)(downloaded*100/total):0; double sec=(GetTickCount64()-start)/1000.0; double kb=sec>0?(downloaded/1024.0/sec):0;
                PostMessageW(hwnd,msgProgress,reinterpret_cast<WPARAM>(new DownloadProgressPayload{row,p,kb,downloaded,total,fileIndex,fileCount,filename}),0);
            }
            if(cancel_){done->message=L"已取消"; done->cancelledAll=true;}
            CloseHandle(hf); WinHttpCloseHandle(r); WinHttpCloseHandle(c); WinHttpCloseHandle(s);
            if(done->success){
                if(total>0 && downloaded!=total){done->success=false; done->message=L"下载大小校验失败"; DeleteFileW(partPath.c_str());}
                else { DeleteFileW(savePath.c_str()); MoveFileExW(partPath.c_str(),savePath.c_str(),MOVEFILE_REPLACE_EXISTING); }
            }else if(done->cancelledAll){ }
            else { DeleteFileW(partPath.c_str()); }
        }
finish: PostMessageW(hwnd,msgDone,reinterpret_cast<WPARAM>(done),0); running_=false;
    });
    return true;
}
