#pragma once

namespace ew {

// Construction skill is converted to machine rate units by flooring skill / 3.
// Worker availability does not gate Earthworks output.
// Each rate unit processes this much terrain per simulated hour; UI forecasts
// and execution use the same value.
constexpr double VolumePerRateUnitPerSimulationHour = 10.0;
// The game's calendar clock advances through 60 simulation hours per date.
constexpr double SimulationHoursPerInGameDay = 60.0;

// Each vehicle contributes floor(constructionSkill / 3) rate units.
double MachineRate(double constructionSkill) noexcept;

} // namespace ew
