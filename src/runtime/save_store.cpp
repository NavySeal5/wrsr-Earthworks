#include "save_store.h"
#include <windows.h>
#include <bcrypt.h>
#include <strsafe.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <utility>
#include <vector>

namespace ew {
namespace {
constexpr size_t SaveNameOffset = 0x9F8;
constexpr size_t SaveTriggerOffset = 0x14D35;
constexpr size_t SaveNameBytes = 120;
constexpr size_t MaxStateBytes = 400 * 1024;

bool SaveName(void* world, char* out, size_t capacity)
{
    if (!world || !out || capacity < 2) return false;
    char raw[SaveNameBytes]{};
    __try { std::memcpy(raw, static_cast<unsigned char*>(world) + SaveNameOffset, sizeof(raw)); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    raw[sizeof(raw)-1]=0;
    if (std::strncmp(raw,"save/",5)!=0 && std::strncmp(raw,"save\\",5)!=0) return false;
    const char* name=raw+5;
    const size_t length=std::strlen(name);
    if (!length || length+1>capacity || std::strcmp(name,".")==0 || std::strcmp(name,"..")==0) return false;
    for (size_t i=0;i<length;++i) {
        const unsigned char c=static_cast<unsigned char>(name[i]);
        const bool alpha=(c>='A'&&c<='Z')||(c>='a'&&c<='z');
        const bool digit=c>='0'&&c<='9';
        if (!(alpha || digit || c==' ' || c=='-' || c=='_' || c=='(' || c==')' || c=='.')) return false;
    }
    std::memcpy(out,name,length+1); return true;
}

bool Paths(const TsmHost* host, const char* name, char* dir, size_t dirSize, char* state, size_t stateSize, char* height, size_t heightSize)
{
    if (!host || !host->exeModule || !name) return false;
    char exe[MAX_PATH]{};
    const DWORD length=GetModuleFileNameA(static_cast<HMODULE>(host->exeModule),exe,static_cast<DWORD>(sizeof(exe)));
    if (!length || length>=sizeof(exe)) return false;
    char* slash=std::strrchr(exe,'\\');
    if (!slash) slash=std::strrchr(exe,'/');
    if (!slash) return false;
    *slash=0;
    if (FAILED(StringCchPrintfA(dir,dirSize,"%s\\media_soviet\\save\\%s",exe,name))) return false;
    if (FAILED(StringCchPrintfA(state,stateSize,"%s\\earthworks.dat",dir))) return false;
    if (FAILED(StringCchPrintfA(height,heightSize,"%s\\heightmap.dds",dir))) return false;
    return true;
}

bool HashFile(const char* path, std::array<uint8_t,32>* digest)
{
    if (!path || !digest) return false;
    HANDLE file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file,&size) || size.QuadPart<0 || size.QuadPart>256ll*1024*1024) { CloseHandle(file); return false; }
    BCRYPT_ALG_HANDLE alg=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    DWORD objectBytes=0, got=0, hashBytes=0;
    bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0
        && BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectBytes),sizeof(objectBytes),&got,0)>=0
        && BCryptGetProperty(alg,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&hashBytes),sizeof(hashBytes),&got,0)>=0
        && hashBytes==digest->size();
    std::vector<UCHAR> object(objectBytes);
    if (ok) ok=BCryptCreateHash(alg,&hash,object.data(),objectBytes,nullptr,0,0)>=0;
    std::array<UCHAR,64*1024> buffer{};
    while (ok) {
        DWORD read=0;
        if (!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&read,nullptr)) { ok=false; break; }
        if (!read) break;
        ok=BCryptHashData(hash,buffer.data(),read,0)>=0;
    }
    if (ok) ok=BCryptFinishHash(hash,digest->data(),static_cast<ULONG>(digest->size()),0)>=0;
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg,0);
    CloseHandle(file); return ok;
}

bool ReadStateFile(const char* path, std::vector<uint8_t>* bytes)
{
    HANDLE file=CreateFileA(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file,&size) || size.QuadPart<0 || size.QuadPart>static_cast<LONGLONG>(MaxStateBytes)) { CloseHandle(file); return false; }
    bytes->resize(static_cast<size_t>(size.QuadPart));
    size_t offset=0; bool ok=true;
    while (offset<bytes->size()) {
        DWORD read=0; const DWORD ask=static_cast<DWORD>(std::min<size_t>(bytes->size()-offset,64*1024));
        if (!ReadFile(file,bytes->data()+offset,ask,&read,nullptr) || !read) { ok=false; break; }
        offset+=read;
    }
    CloseHandle(file); return ok;
}

bool WriteStateFile(const char* path, const std::vector<uint8_t>& bytes)
{
    char temp[MAX_PATH]{};
    if (FAILED(StringCchPrintfA(temp,sizeof(temp),"%s.tmp",path))) return false;
    HANDLE file=CreateFileA(temp,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if (file==INVALID_HANDLE_VALUE) return false;
    size_t offset=0; bool ok=true;
    while (offset<bytes.size()) {
        DWORD wrote=0; const DWORD ask=static_cast<DWORD>(std::min<size_t>(bytes.size()-offset,64*1024));
        if (!WriteFile(file,bytes.data()+offset,ask,&wrote,nullptr) || !wrote) { ok=false; break; }
        offset+=wrote;
    }
    if (ok) ok=FlushFileBuffers(file)!=0;
    CloseHandle(file);
    if (!ok) { DeleteFileA(temp); return false; }
    if (!MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)) { DeleteFileA(temp); return false; }
    return true;
}
}

bool SaveIsActive(void* world) noexcept
{
    if (!world) return false;
    __try { return *(static_cast<volatile unsigned char*>(world)+SaveTriggerOffset)!=0; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

bool SaveDurableState(const TsmHost* host, void* world, const DurableState& source)
{
    char name[SaveNameBytes]{},dir[MAX_PATH]{},statePath[MAX_PATH]{},heightPath[MAX_PATH]{};
    if (!SaveName(world,name,sizeof(name)) || !Paths(host,name,dir,sizeof(dir),statePath,sizeof(statePath),heightPath,sizeof(heightPath))) return false;
    DurableState snapshot=source;
    if (!HashFile(heightPath,&snapshot.terrainFingerprint)) return false;
    std::vector<uint8_t> bytes;
    if (!EncodeState(snapshot,&bytes) || bytes.size()>MaxStateBytes) return false;
    return WriteStateFile(statePath,bytes);
}

StateLoadResult LoadDurableState(const TsmHost* host, void* world, DurableState* state)
{
    char name[SaveNameBytes]{},dir[MAX_PATH]{},statePath[MAX_PATH]{},heightPath[MAX_PATH]{};
    if (!SaveName(world,name,sizeof(name)) || !Paths(host,name,dir,sizeof(dir),statePath,sizeof(statePath),heightPath,sizeof(heightPath))) return StateLoadResult::Unavailable;
    if (GetFileAttributesA(statePath)==INVALID_FILE_ATTRIBUTES) return GetLastError()==ERROR_FILE_NOT_FOUND ? StateLoadResult::Missing : StateLoadResult::Unavailable;
    std::vector<uint8_t> bytes; DurableState parsed; std::array<uint8_t,32> fingerprint{};
    if (!ReadStateFile(statePath,&bytes) || !DecodeState(bytes.data(),bytes.size(),&parsed)) return StateLoadResult::Corrupt;
    if (!HashFile(heightPath,&fingerprint)) return StateLoadResult::Unavailable;
    if (!FingerprintMatches(parsed,fingerprint)) return StateLoadResult::TerrainMismatch;
    *state=std::move(parsed); return StateLoadResult::Loaded;
}

} // namespace ew
