// SPDX-License-Identifier: GPL-3.0-only
// Minimal bridge for pausing active Sites when vanilla terrain edits overlap.
#include <windows.h>
#include <cmath>
#include <cstring>
#include "native_terrain_edit_hook.h"
#include "runtime.h"

namespace ew {
namespace {
using AngleLimitFn = void (*)(void*, const float*, float, float, float, float, bool, float);
AngleLimitFn original = nullptr;
constexpr uintptr_t AngleLimitRva = 0xF9380;
constexpr uintptr_t AngleLimitIatRva = 0x86D240;
constexpr unsigned char angleLimitBytes[] = {
    0x48,0x8B,0xC4,0xF3,0x0F,0x11,0x58,0x20,0x55,0x48,0x81,0xEC,
    0x20,0x01,0x00,0x00,0x4C,0x8B,0x89,0x10,0x02,0x00,0x00
};

void HookAngleLimit(void* terrain, const float* position, float a, float b, float c, float d, bool flag, float limit)
{
    __try {
        if (position) {
            const float x = position[0];
            const float z = position[2];
            if (std::isfinite(x) && std::isfinite(z) && std::isfinite(b)) NotifyNativeTerrainEdit(x, z, b);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    original(terrain, position, a, b, c, d, flag, limit);
}
}

bool BindNativeTerrainEditHook()
{
    if (!host || !host->engineModule || !host->installInlineHook) return false;
    auto module = static_cast<HMODULE>(host->engineModule);
    constexpr char Symbol[] = "?EditRaiseLowerAngleLimit@C3D_TERRAIN@@QEAAXVC3DVECTOR3@@MMMM_NM@Z";
    auto* target = reinterpret_cast<unsigned char*>(GetProcAddress(module, Symbol));
    auto** slot = host->findIatSlot && host->exeModule
        ? reinterpret_cast<void**>(host->findIatSlot(host->exeModule, "C3DDLL64.dll", Symbol))
        : nullptr;
    const auto exeBase = reinterpret_cast<uintptr_t>(host->exeBase);
    if (!target || !slot || !host->patchIat
        || reinterpret_cast<uintptr_t>(target) - reinterpret_cast<uintptr_t>(module) != AngleLimitRva
        || reinterpret_cast<uintptr_t>(slot) < exeBase
        || reinterpret_cast<uintptr_t>(slot) - exeBase != AngleLimitIatRva
        || *slot != target || std::memcmp(target, angleLimitBytes, sizeof(angleLimitBytes)) != 0) {
        host->log("earthworks  native terrain-overlap hook unavailable for this game build");
        return false;
    }
    if (!host->patchIat(host->exeModule, "C3DDLL64.dll", Symbol, reinterpret_cast<void*>(HookAngleLimit),
        reinterpret_cast<void**>(&original), "Earthworks native terrain overlap")) {
        host->log("earthworks  native terrain-overlap hook was not installed");
        return false;
    }
    host->log("earthworks  native terrain-overlap guard installed");
    return true;
}
} // namespace ew
