#pragma once

#include "../runtime/runtime.h"

namespace ew {
// Draws site wireframes through the game's 2D panel immediately before its
// native interface, so native windows cover the terrain preview.
bool InstallGameUnderlay(const TsmHost* host);
}
