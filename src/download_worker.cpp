#include "pch.h"
#include "download_worker.h"
#include "hash_utils.h"
#include <filesystem>
#include <vector>

#pragma comment(lib, "winhttp.lib")

DownloadWorker::~DownloadWorker(){ Cancel(); Join(); }
void DownloadWorker::Join(){ if(worker_.joinable()) worker_.join(); }
void DownloadWorker::Cancel(){ cancel_=true; }

bool DownloadWorker::Start(HWND hwnd, int row, int fileIndex, int fileCount, const std::wstring& filename, const std::wstring& url, const std::wstring& savePath, const std::wstring& expectedSha256, const DownloadOptions& options, UINT msgProgress, UINT msgLog, UINT msgDone) {
    if (running_) return false; Join(); cancel_=false; running_=true;
    worker_=std::thread([&, this]{
        auto done = new DownloadDonePayload{row,false,L"失败",0,0,false};
        std::wstring partPath = savePath + L".part";
        URL_COMPONENTS uc{}; uc.dwStructSize=sizeof(uc); wchar_t host[256]={}, path[2048]={}; uc.lpszHostName=host; uc.dwHostNameLength=256; uc.lpszUrlPath=path; uc.dwUrlPathLength=2048;
        if(!WinHttpCrackUrl(url.c_str(),0,0,&uc)){done->message=L"URL解析失败"; goto finish;}
        std::filesystem::create_directories(std::filesystem::path(savePath).parent_path());
        for(int attempt=1; attempt<=options.retryCount && !done->success && !cancel_; ++attempt){
            ULONGLONG existSize=0; bool resumed=false;
            if(options.enableResume && std::filesystem::exists(partPath)) { existSize=std::filesystem::file_size(partPath); resumed=existSize>0; if(resumed) PostMessageW(hwnd,msgLog,0,(LPARAM)new std::wstring(L"检测到未完成文件，继续下载")); }
            HANDLE hf=CreateFileW(partPath.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,resumed?OPEN_EXISTING:CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
            if(hf==INVALID_HANDLE_VALUE){done->message=L"文件无权限或无法写入"; break;} if(resumed) SetFilePointer(hf,0,nullptr,FILE_END);
            HINTERNET s=WinHttpOpen(L"VersionDownloader/2.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0);
            if(!s){CloseHandle(hf); done->message=L"WinHTTP会话失败"; continue;}
            WinHttpSetTimeouts(s, options.dnsTimeoutMs, options.connectTimeoutMs, options.sendTimeoutMs, options.receiveTimeoutMs);
            HINTERNET c=WinHttpConnect(s,uc.lpszHostName,uc.nPort,0); DWORD flags=(uc.nScheme==INTERNET_SCHEME_HTTPS)?WINHTTP_FLAG_SECURE:0; HINTERNET r=c?WinHttpOpenRequest(c,L"GET",uc.lpszUrlPath,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,flags):nullptr;
            std::wstring rangeHeader; if(resumed){rangeHeader=L"Range: bytes="+std::to_wstring(existSize)+L"-\r\n";}
            bool reqOk = r && WinHttpSendRequest(r,resumed?rangeHeader.c_str():WINHTTP_NO_ADDITIONAL_HEADERS,resumed?(DWORD)-1L:0,WINHTTP_NO_REQUEST_DATA,0,0,0) && WinHttpReceiveResponse(r,nullptr);
            if(!reqOk){done->message=L"网络连接失败";} else {
                DWORD st=0,stLen=sizeof(st); WinHttpQueryHeaders(r,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&st,&stLen,WINHTTP_NO_HEADER_INDEX);
                if((st==403||st==404)){done->message=L"HTTP "+std::to_wstring(st); CloseHandle(hf); WinHttpCloseHandle(r); WinHttpCloseHandle(c); WinHttpCloseHandle(s); break;}
                if(resumed && st!=206){ PostMessageW(hwnd,msgLog,0,(LPARAM)new std::wstring(L"服务器不支持断点续传，重新下载")); CloseHandle(hf); DeleteFileW(partPath.c_str()); resumed=false; existSize=0; if(r)WinHttpCloseHandle(r); if(c)WinHttpCloseHandle(c); if(s)WinHttpCloseHandle(s); if(attempt<=options.retryCount){attempt--; continue;} }
                unsigned long long contentLen=0; DWORD len=sizeof(contentLen); bool hasLen=WinHttpQueryHeaders(r,WINHTTP_QUERY_CONTENT_LENGTH|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&contentLen,&len,WINHTTP_NO_HEADER_INDEX);
                unsigned long long total=hasLen?(resumed?existSize+contentLen:contentLen):0; done->expected=total;
                unsigned long long downloaded=existSize; auto start=GetTickCount64(); std::vector<unsigned char> buf(64*1024);
                while(!cancel_){ DWORD avail=0; if(!WinHttpQueryDataAvailable(r,&avail)){done->message=L"读取数据失败"; break;} if(avail==0){done->success=true; done->message=L"下载完成"; break;} DWORD rd=0; DWORD toRead=std::min<DWORD>(avail,(DWORD)buf.size()); if(!WinHttpReadData(r,buf.data(),toRead,&rd)||rd==0){done->message=L"下载中断"; break;} DWORD wr=0; if(!WriteFile(hf,buf.data(),rd,&wr,nullptr)||wr!=rd){done->message=L"写入失败"; break;} downloaded+=rd; done->downloaded=downloaded; int p= total? (int)(downloaded*100/total):0; double sec=(GetTickCount64()-start)/1000.0; double kb=sec>0?((downloaded-existSize)/1024.0/sec):0; PostMessageW(hwnd,msgProgress,reinterpret_cast<WPARAM>(new DownloadProgressPayload{row,p,kb,downloaded,total,fileIndex,fileCount,filename,attempt,resumed}),0);} 
                if(cancel_){done->message=L"已取消"; done->cancelledAll=true;}
            }
            CloseHandle(hf); if(r)WinHttpCloseHandle(r); if(c)WinHttpCloseHandle(c); if(s)WinHttpCloseHandle(s);
            if(done->success){
                if(done->expected>0 && done->downloaded!=done->expected){done->success=false; done->message=L"下载大小校验失败";}
                if(done->success && options.verifySHA256 && !expectedSha256.empty()){
                    std::wstring actual,err; PostMessageW(hwnd,msgLog,0,(LPARAM)new std::wstring(L"期望 SHA256: "+expectedSha256));
                    if(!ComputeFileSHA256(partPath,actual,err)){done->success=false; done->message=L"SHA256 计算失败: "+err;}
                    PostMessageW(hwnd,msgLog,0,(LPARAM)new std::wstring(L"实际 SHA256: "+actual));
                    if(done->success && _wcsicmp(actual.c_str(), expectedSha256.c_str())!=0){done->success=false; done->message=L"SHA256 校验失败";}
                    PostMessageW(hwnd,msgLog,0,(LPARAM)new std::wstring(done->success?L"SHA256 校验成功":L"SHA256 校验失败"));
                }
                if(done->success){DeleteFileW(savePath.c_str()); MoveFileExW(partPath.c_str(),savePath.c_str(),MOVEFILE_REPLACE_EXISTING);} 
            }
            if(done->success||cancel_) break;
            if(attempt<options.retryCount){ PostMessageW(hwnd,msgLog,0,(LPARAM)new std::wstring(L"下载失败，准备第 "+std::to_wstring(attempt+1)+L"/"+std::to_wstring(options.retryCount)+L" 次重试")); Sleep((attempt==1?1000:(attempt==2?2000:4000))); }
        }
finish: PostMessageW(hwnd,msgDone,reinterpret_cast<WPARAM>(done),0); running_=false;
    });
    return true;
}
