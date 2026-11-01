#pragma once
#include "../core/state_store.h"
#include "tesmio_api.h"

namespace ew {
enum class StateLoadResult { Loaded, Missing, Corrupt, TerrainMismatch, Unavailable };
bool SaveDurableState(const TsmHost* host, void* world, const DurableState& state);
StateLoadResult LoadDurableState(const TsmHost* host, void* world, DurableState* state);
bool SaveIsActive(void* world) noexcept;
} // namespace ew
