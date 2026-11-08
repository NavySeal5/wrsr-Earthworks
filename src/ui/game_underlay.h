#pragma once

#include "../runtime/runtime.h"

namespace ew {
// Draws site wireframes through the game's 2D panel immediately before its
// native interface, so native windows cover the terrain preview.
bool InstallGameUnderlay(const TsmHost* host);
// True when the screen point lies inside a native game UI sprite/panel drawn
// in the most recent interface pass. Used to keep map tools from claiming UI clicks.
struct NativeUiOcclusionInfo { unsigned rectCount=0; int hitIndex=-1; float left=0,top=0,right=0,bottom=0; };
bool NativeUiOccludes(float x,float y,NativeUiOcclusionInfo* info=nullptr);
// Supplies a frame-to-frame Site geometry preview while the user drags a point.
// A null Plan clears the temporary override; it never changes runtime state.
void SetInteractiveSitePreview(uint64_t siteId, const Plan* plan);
}
