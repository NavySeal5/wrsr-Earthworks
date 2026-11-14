#pragma once

namespace ew {

// Each point of the vehicle panel's displayed Speed/Level value is one machine
// rate unit.
// Worker availability does not gate Earthworks output.
// Each rate unit processes this much terrain per simulated hour; UI forecasts
// and execution use the same value.
// At the default 1x setting, 60 simulation hours per in-game day make this
// 250 m3 per displayed Speed/Level point per in-game day.
constexpr double VolumePerRateUnitPerSimulationHour = 250.0 / 60.0;
// The game's calendar clock advances through 60 simulation hours per date.
constexpr double SimulationHoursPerInGameDay = 60.0;

// Each vehicle contributes its displayed Speed/Level value as rate units.
double MachineRate(double displayedLevel) noexcept;

} // namespace ew
