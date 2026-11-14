#include "productivity.h"
#include <algorithm>
#include <cmath>

namespace ew {

double MachineRate(double displayedLevel) noexcept
{
    if (!std::isfinite(displayedLevel) || displayedLevel <= 0.0) return 0.0;
    return displayedLevel;
}

} // namespace ew
