#pragma once

#include <string>
#include <vector>

#include "version_item.h"

bool ParseCsvText(const std::wstring& csvText, std::vector<VersionItem>& items);
bool LoadUtf8TextFile(const std::wstring& path, std::wstring& outText);
bool LoadUtf8TextFromHttp(const std::wstring& url, std::wstring& outText, std::wstring& err);
