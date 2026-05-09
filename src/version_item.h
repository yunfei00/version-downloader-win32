#pragma once

#include <string>

struct VersionItem {
    int index = 0;
    std::wstring name;
    std::wstring filename;
    std::wstring size;
    std::wstring url;
    std::wstring status = L"等待下载";
};
