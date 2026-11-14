#pragma once

namespace ew {

void LoadGlobalSettings();
void SaveGlobalSettings();
float ConstructionSpeedMultiplier() noexcept;
void SetConstructionSpeedMultiplier(float value) noexcept;
bool HighContrastColors() noexcept;
void SetHighContrastColors(bool enabled) noexcept;
bool DistanceBasedFacilityContribution() noexcept;
void SetDistanceBasedFacilityContribution(bool enabled) noexcept;
int CloseFacilityDistanceMeters() noexcept;
int MediumFacilityDistanceMeters() noexcept;
int FarFacilityDistanceMeters() noexcept;
void SetFacilityDistanceThresholds(int closeMeters,int mediumMeters,int farMeters) noexcept;
double FacilityDistanceContribution(double distanceMeters) noexcept;

}
