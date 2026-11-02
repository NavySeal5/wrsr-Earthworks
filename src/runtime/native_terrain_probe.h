#pragma once
#include <cstdint>

namespace ew {

struct NativeTerrainCallView {
    bool bound = false;
    bool callerInGame = false;
    uint64_t calls = 0;
    uint64_t caller = 0; // Game RVA when callerInGame; absolute address otherwise.
    float x = 0, z = 0;
    float brush[4]{};
    bool flag = false;
    float angleLimit = 0;
};

bool BindNativeTerrainCallProbe();
NativeTerrainCallView ReadNativeTerrainCallProbe() noexcept;

} // namespace ew
