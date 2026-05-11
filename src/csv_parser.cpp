#include "pch.h"
#include "csv_parser.h"
#include "string_utils.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace {
std::wstring Trim(const std::wstring& s) { const wchar_t* ws=L" \t\r\n"; size_t b=s.find_first_not_of(ws); if(b==std::wstring::npos) return L""; size_t e=s.find_last_not_of(ws); return s.substr(b,e-b+1);} 
std::vector<std::wstring> Split(const std::wstring& line){ std::wstringstream ls(line); std::wstring c; std::vector<std::wstring> v; while(std::getline(ls,c,L',')) v.push_back(Trim(c)); return v; }
std::wstring Lower(std::wstring s){ std::transform(s.begin(),s.end(),s.begin(),::towlower); return s; }
}

bool LoadUtf8TextFile(const std::wstring& path, std::wstring& outText) { HANDLE h=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr); if(h==INVALID_HANDLE_VALUE) return false; DWORD size=GetFileSize(h,nullptr); std::string buf(size,'\0'); DWORD read=0; bool ok=ReadFile(h,buf.data(),size,&read,nullptr); CloseHandle(h); if(!ok) return false; buf.resize(read); if(buf.size()>=3 && (unsigned char)buf[0]==0xEF && (unsigned char)buf[1]==0xBB && (unsigned char)buf[2]==0xBF){ buf.erase(0,3);} outText = Utf8ToWide(buf); if(!outText.empty()&&outText[0]==0xFEFF) outText.erase(outText.begin()); return !outText.empty() || buf.empty();} 

bool LoadUtf8TextFromHttp(const std::wstring& url, std::wstring& outText, std::wstring& err) {
URL_COMPONENTS uc{}; uc.dwStructSize=sizeof(uc); wchar_t host[256]={}, path[2048]={}; uc.lpszHostName=host; uc.dwHostNameLength=256; uc.lpszUrlPath=path; uc.dwUrlPathLength=2048;
if(!WinHttpCrackUrl(url.c_str(),0,0,&uc)){err=L"URL解析失败"; return false;} HINTERNET s=WinHttpOpen(L"VersionDownloader/2.0",WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0); if(!s){err=L"WinHTTP会话失败";return false;} HINTERNET c=WinHttpConnect(s,uc.lpszHostName,uc.nPort,0); if(!c){WinHttpCloseHandle(s); err=L"连接失败"; return false;} DWORD flags=uc.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0; HINTERNET r=WinHttpOpenRequest(c,L"GET",uc.lpszUrlPath,nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,flags); if(!r){WinHttpCloseHandle(c);WinHttpCloseHandle(s);err=L"请求创建失败";return false;}
bool ok=WinHttpSendRequest(r,WINHTTP_NO_ADDITIONAL_HEADERS,0,WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(r,nullptr); std::string bytes; while(ok){DWORD avail=0; if(!WinHttpQueryDataAvailable(r,&avail)){ok=false;break;} if(avail==0)break; size_t old=bytes.size(); bytes.resize(old+avail); DWORD got=0; if(!WinHttpReadData(r,bytes.data()+old,avail,&got)){ok=false;break;} bytes.resize(old+got);} WinHttpCloseHandle(r); WinHttpCloseHandle(c); WinHttpCloseHandle(s); if(!ok){err=L"HTTP读取失败"; return false;} if(bytes.size()>=3 && (unsigned char)bytes[0]==0xEF && (unsigned char)bytes[1]==0xBB && (unsigned char)bytes[2]==0xBF){ bytes.erase(0,3);} outText = Utf8ToWide(bytes); if(!outText.empty()&&outText[0]==0xFEFF) outText.erase(outText.begin()); return !outText.empty() || bytes.empty();
}

bool ParseCsvText(const std::wstring& csvText, std::vector<VersionItem>& items) {
    std::wstringstream ss(csvText); std::wstring line; items.clear();
    if(!std::getline(ss,line)) return false;
    auto headers=Split(line); std::map<std::wstring,int> idx;
    for(int i=0;i<(int)headers.size();++i) idx[Lower(headers[i])]=i;
    if(!idx.count(L"name") || !idx.count(L"filename") || !idx.count(L"size") || !idx.count(L"url")) return false;
    int row=1;
    while(std::getline(ss,line)){
        line=Trim(line); if(line.empty()) continue;
        auto cols=Split(line);
        auto get=[&](const std::wstring& k)->std::wstring{ auto it=idx.find(k); if(it==idx.end()||it->second>=(int)cols.size()) return L""; return cols[it->second];};
        VersionItem it; it.index=row++; it.name=get(L"name"); it.filename=get(L"filename"); it.size=get(L"size"); it.url=get(L"url");
        if(it.name.empty()||it.filename.empty()||it.url.empty()) { OutputDebugStringW((L"CSV非法行: "+line+L"\n").c_str()); continue; }
        items.push_back(it);
    }
    return !items.empty();
}
