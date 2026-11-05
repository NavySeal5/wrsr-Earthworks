#include "productivity.h"
#include <algorithm>
#include <cmath>

namespace ew {

double MachineRate(double groundworksRating) noexcept
{
    if (!std::isfinite(groundworksRating) || groundworksRating <= 0.0) groundworksRating = ReferenceGroundworksRating;
    return groundworksRating / ReferenceGroundworksRating;
}

} // namespace ew
