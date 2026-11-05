#pragma once

namespace ew {

// Provisional machine-only scale for Earthworks. A 22-rated mechanism is the
// reference machine; worker availability does not gate Earthworks output.
constexpr double ReferenceGroundworksRating = 22.0;

// Rate is relative to one unit of player-performed manual terraforming. The
// machine rating is provisional; this does not estimate volume/time.
double MachineRate(double groundworksRating) noexcept;

} // namespace ew
