#include "user_settings.h"

#include <windows.h>

#include <algorithm>
#include <atomic>
#include <climits>
#include <cmath>
#include <string>

namespace ew {
namespace {
std::atomic<int> speedPercent{100};
std::atomic<bool> highContrastColors{false};
std::atomic<bool> distanceBasedContribution{false};
std::atomic<int> closeDistanceMeters{3500};
std::atomic<int> mediumDistanceMeters{5000};
std::atomic<int> farDistanceMeters{10000};

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
    const int loadedDistance=static_cast<int>(GetPrivateProfileIntW(L"earthworks",L"distance_based_facility_contribution",0,path.c_str()));
    distanceBasedContribution.store(loadedDistance!=0,std::memory_order_relaxed);
    int closeMetersValue=static_cast<int>(GetPrivateProfileIntW(L"earthworks",L"facility_close_distance_m",3500,path.c_str()));
    int mediumMetersValue=static_cast<int>(GetPrivateProfileIntW(L"earthworks",L"facility_medium_distance_m",5000,path.c_str()));
    int farMetersValue=static_cast<int>(GetPrivateProfileIntW(L"earthworks",L"facility_far_distance_m",10000,path.c_str()));
    closeMetersValue=std::clamp(closeMetersValue,1,INT_MAX-2);
    mediumMetersValue=std::clamp(mediumMetersValue,closeMetersValue+1,INT_MAX-1);
    farMetersValue=std::clamp(farMetersValue,mediumMetersValue+1,INT_MAX);
    closeDistanceMeters.store(closeMetersValue,std::memory_order_relaxed);
    mediumDistanceMeters.store(mediumMetersValue,std::memory_order_relaxed);
    farDistanceMeters.store(farMetersValue,std::memory_order_relaxed);
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
    WritePrivateProfileStringW(L"earthworks",L"distance_based_facility_contribution",distanceBasedContribution.load(std::memory_order_relaxed)?enabled:disabled,path.c_str());
    WritePrivateProfileStringW(L"earthworks",L"facility_close_distance_m",std::to_wstring(CloseFacilityDistanceMeters()).c_str(),path.c_str());
    WritePrivateProfileStringW(L"earthworks",L"facility_medium_distance_m",std::to_wstring(MediumFacilityDistanceMeters()).c_str(),path.c_str());
    WritePrivateProfileStringW(L"earthworks",L"facility_far_distance_m",std::to_wstring(FarFacilityDistanceMeters()).c_str(),path.c_str());
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

bool DistanceBasedFacilityContribution() noexcept
{
    return distanceBasedContribution.load(std::memory_order_relaxed);
}

void SetDistanceBasedFacilityContribution(bool enabled) noexcept
{
    distanceBasedContribution.store(enabled,std::memory_order_relaxed);
}

int CloseFacilityDistanceMeters() noexcept { return closeDistanceMeters.load(std::memory_order_relaxed); }
int MediumFacilityDistanceMeters() noexcept { return mediumDistanceMeters.load(std::memory_order_relaxed); }
int FarFacilityDistanceMeters() noexcept { return farDistanceMeters.load(std::memory_order_relaxed); }

void SetFacilityDistanceThresholds(int closeMeters,int mediumMeters,int farMeters) noexcept
{
    closeMeters=std::clamp(closeMeters,1,INT_MAX-2);
    mediumMeters=std::clamp(mediumMeters,closeMeters+1,INT_MAX-1);
    farMeters=std::clamp(farMeters,mediumMeters+1,INT_MAX);
    closeDistanceMeters.store(closeMeters,std::memory_order_relaxed);
    mediumDistanceMeters.store(mediumMeters,std::memory_order_relaxed);
    farDistanceMeters.store(farMeters,std::memory_order_relaxed);
}

double FacilityDistanceContribution(double distanceMeters) noexcept
{
    if(!DistanceBasedFacilityContribution()||!std::isfinite(distanceMeters))return 1.0;
    const double closeThreshold=static_cast<double>(CloseFacilityDistanceMeters());
    const double mediumThreshold=static_cast<double>(MediumFacilityDistanceMeters());
    const double farThreshold=static_cast<double>(FarFacilityDistanceMeters());
    if(distanceMeters<=closeThreshold)return 1.0;
    if(distanceMeters<=mediumThreshold)return 1.0-0.5*(distanceMeters-closeThreshold)/(mediumThreshold-closeThreshold);
    if(distanceMeters<=farThreshold)return 0.5-0.25*(distanceMeters-mediumThreshold)/(farThreshold-mediumThreshold);
    return 0.25;
}
}
