#pragma once

namespace ew {

void LoadGlobalSettings();
void SaveGlobalSettings();
float ConstructionSpeedMultiplier() noexcept;
void SetConstructionSpeedMultiplier(float value) noexcept;
bool HighContrastColors() noexcept;
void SetHighContrastColors(bool enabled) noexcept;

}
