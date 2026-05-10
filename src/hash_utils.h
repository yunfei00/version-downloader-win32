#pragma once

#include <string>

bool ComputeFileSHA256(const std::wstring& file_path, std::wstring& out_hash, std::wstring& error);
