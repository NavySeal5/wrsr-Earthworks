#include "productivity.h"
#include <algorithm>
#include <cmath>

namespace ew {

double MachineRate(double constructionSkill) noexcept
{
    if (!std::isfinite(constructionSkill) || constructionSkill <= 0.0) return 0.0;
    return std::floor(constructionSkill / 3.0);
}

} // namespace ew
