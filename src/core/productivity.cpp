#include "productivity.h"
#include <algorithm>
#include <cmath>

namespace ew {

double WorkerOnlyRate(int workers) noexcept
{
    return static_cast<double>(std::max(workers, 0)) / ReferenceGroundworksRating;
}

double MachineRate(int assignedWorkers, double groundworksRating) noexcept
{
    if (assignedWorkers <= 0) return 0.0;
    if (!std::isfinite(groundworksRating) || groundworksRating <= 0.0) groundworksRating = ReferenceGroundworksRating;
    const double staffing = std::min(static_cast<double>(assignedWorkers) / WorkersPerFullyStaffedMachine, 1.0);
    return staffing * groundworksRating / ReferenceGroundworksRating;
}

} // namespace ew
