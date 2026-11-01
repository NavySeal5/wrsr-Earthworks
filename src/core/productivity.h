#pragma once

namespace ew {

// Provisional scale for the first Earthworks-owned work model. A 22-rated
// mechanism is the reference machine; the community-reported vehicle rating
// interpretation treats that as roughly 22 fully productive workers.
constexpr double ReferenceGroundworksRating = 22.0;
constexpr double WorkersPerFullyStaffedMachine = 10.0;

// Rates are relative to one unit of player-performed manual terraforming.
// They are deliberately not volume/time estimates: native terrain throughput
// has not yet been calibrated, and the user's sampled machine run included an
// unknown workforce.
double WorkerOnlyRate(int workers) noexcept;
double MachineRate(int assignedWorkers, double groundworksRating) noexcept;

} // namespace ew
