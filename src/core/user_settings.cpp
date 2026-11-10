#include "user_settings.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <string>

namespace ew {
namespace {
std::atomic<int> speedPercent{100};
std::atomic<bool> highContrastColors{false};

std::wstring SettingsPath()
{
    wchar_t localAppData[MAX_PATH]{};
    const DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",localAppData,MAX_PATH);
    if(!length||length>=MAX_PATH)return {};
    std::wstring folder(localAppData,length);
    folder+=L"\\EarthworksPlanning";
    if(!CreateDirectoryW(folder.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)return {};
    return folder+L"\\settings.ini";
}
}

void LoadGlobalSettings()
{
    const auto path=SettingsPath();
    if(path.empty())return;
    const int loadedSpeed=static_cast<int>(GetPrivateProfileIntW(L"earthworks",L"construction_speed_percent",100,path.c_str()));
    speedPercent.store(std::clamp(loadedSpeed,10,1000),std::memory_order_relaxed);
    const int loadedContrast=static_cast<int>(GetPrivateProfileIntW(L"earthworks",L"high_contrast_colors",0,path.c_str()));
    highContrastColors.store(loadedContrast!=0,std::memory_order_relaxed);
}

void SaveGlobalSettings()
{
    const auto path=SettingsPath();
    if(path.empty())return;
    const auto speed=std::to_wstring(speedPercent.load(std::memory_order_relaxed));
    const wchar_t contrast[]=L"high_contrast_colors";
    const wchar_t enabled[]=L"1";
    const wchar_t disabled[]=L"0";
    WritePrivateProfileStringW(L"earthworks",L"construction_speed_percent",speed.c_str(),path.c_str());
    WritePrivateProfileStringW(L"earthworks",contrast,highContrastColors.load(std::memory_order_relaxed)?enabled:disabled,path.c_str());
    WritePrivateProfileStringW(nullptr,nullptr,nullptr,path.c_str());
}

float ConstructionSpeedMultiplier() noexcept
{
    return static_cast<float>(speedPercent.load(std::memory_order_relaxed))*0.01f;
}

void SetConstructionSpeedMultiplier(float value) noexcept
{
    const int percent=static_cast<int>(value*100.0f+0.5f);
    speedPercent.store(std::clamp(percent,10,1000),std::memory_order_relaxed);
}

bool HighContrastColors() noexcept
{
    return highContrastColors.load(std::memory_order_relaxed);
}

void SetHighContrastColors(bool enabled) noexcept
{
    highContrastColors.store(enabled,std::memory_order_relaxed);
}
}
