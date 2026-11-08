#pragma once

namespace ew {

// Provisional machine-only scale for Earthworks. A 22-rated mechanism is the
// reference machine; worker availability does not gate Earthworks output.
constexpr double ReferenceGroundworksRating = 22.0;
// Current provisional execution scale: one relative manual-rate unit processes
// this much terrain per simulated hour. UI forecasts use the same value.
constexpr double VolumePerRateUnitPerSimulationHour = 10.0;
constexpr double SimulationHoursPerInGameDay = 24.0;

// Rate is relative to one unit of player-performed manual terraforming. The
// machine rating is provisional; this does not estimate volume/time.
double MachineRate(double groundworksRating) noexcept;

} // namespace ew
