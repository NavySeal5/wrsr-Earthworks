#pragma once

#include "../runtime/runtime.h"

namespace ew {
// Draws site wireframes through the game's 2D panel immediately before its
// native interface, so native windows cover the terrain preview.
bool InstallGameUnderlay(const TsmHost* host);
// Supplies a frame-to-frame Site geometry preview while the user drags a point.
// A null Plan clears the temporary override; it never changes runtime state.
void SetInteractiveSitePreview(uint64_t siteId, const Plan* plan);
}
