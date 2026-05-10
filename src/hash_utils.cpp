#include "pch.h"
#include "hash_utils.h"

#include <bcrypt.h>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

bool ComputeFileSHA256(const std::wstring& file_path, std::wstring& out_hash, std::wstring& error) {
    out_hash.clear();
    error.clear();

    HANDLE file = CreateFileW(file_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = L"打开文件失败";
        return false;
    }

    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::vector<BYTE> obj;
    std::vector<BYTE> digest;

    NTSTATUS st = BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(st)) { CloseHandle(file); error = L"BCryptOpenAlgorithmProvider 失败"; return false; }

    DWORD objLen = 0, cb = 0, hashLen = 0;
    st = BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objLen), sizeof(objLen), &cb, 0);
    if (!BCRYPT_SUCCESS(st)) { CloseHandle(file); BCryptCloseAlgorithmProvider(alg,0); error = L"读取哈希对象长度失败"; return false; }
    st = BCryptGetProperty(alg, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLen), sizeof(hashLen), &cb, 0);
    if (!BCRYPT_SUCCESS(st)) { CloseHandle(file); BCryptCloseAlgorithmProvider(alg,0); error = L"读取哈希长度失败"; return false; }

    obj.resize(objLen);
    digest.resize(hashLen);
    st = BCryptCreateHash(alg, &hash, obj.data(), objLen, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(st)) { CloseHandle(file); BCryptCloseAlgorithmProvider(alg,0); error = L"创建哈希失败"; return false; }

    std::vector<BYTE> buf(64 * 1024);
    for (;;) {
        DWORD rd = 0;
        if (!ReadFile(file, buf.data(), static_cast<DWORD>(buf.size()), &rd, nullptr)) {
            error = L"读取文件失败";
            BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(alg,0); CloseHandle(file); return false;
        }
        if (rd == 0) break;
        st = BCryptHashData(hash, buf.data(), rd, 0);
        if (!BCRYPT_SUCCESS(st)) { error = L"更新哈希失败"; BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(alg,0); CloseHandle(file); return false; }
    }

    st = BCryptFinishHash(hash, digest.data(), hashLen, 0);
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    CloseHandle(file);
    if (!BCRYPT_SUCCESS(st)) { error = L"完成哈希失败"; return false; }

    static const wchar_t* kHex = L"0123456789abcdef";
    out_hash.reserve(hashLen * 2);
    for (BYTE b : digest) {
        out_hash.push_back(kHex[(b >> 4) & 0x0F]);
        out_hash.push_back(kHex[b & 0x0F]);
    }
    return true;
}
