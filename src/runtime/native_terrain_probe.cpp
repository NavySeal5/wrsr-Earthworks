// SPDX-License-Identifier: GPL-3.0-only
// Read-only observation of the game's native angle-limited terrain brush call.
#include <windows.h>
#include <intrin.h>
#include <atomic>
#include <cstring>
#include "native_terrain_probe.h"
#include "runtime.h"

#pragma intrinsic(_ReturnAddress)

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
std::atomic<uint64_t> calls{0};
std::atomic<uintptr_t> caller{0};
std::atomic<bool> callerInGame{false};
std::atomic<float> x{0}, z{0};
std::atomic<float> brush[4]{};
std::atomic<float> angleLimit{0};
std::atomic<bool> lastFlag{false};
std::atomic<bool> bound{false};

void HookAngleLimit(void* terrain, const float* position, float a, float b, float c, float d, bool flag, float limit)
{
    const auto returnAddress = reinterpret_cast<uintptr_t>(_ReturnAddress());
    if (host && returnAddress >= reinterpret_cast<uintptr_t>(host->exeBase)
        && returnAddress < reinterpret_cast<uintptr_t>(host->exeBase) + host->exeSize) {
        caller.store(returnAddress - reinterpret_cast<uintptr_t>(host->exeBase), std::memory_order_relaxed);
        callerInGame.store(true, std::memory_order_relaxed);
    } else {
        caller.store(returnAddress, std::memory_order_relaxed);
        callerInGame.store(false, std::memory_order_relaxed);
    }
    __try {
        if (position) { x.store(position[0], std::memory_order_relaxed); z.store(position[2], std::memory_order_relaxed); }
        brush[0].store(a, std::memory_order_relaxed);
        brush[1].store(b, std::memory_order_relaxed);
        brush[2].store(c, std::memory_order_relaxed);
        brush[3].store(d, std::memory_order_relaxed);
        lastFlag.store(flag, std::memory_order_relaxed);
        angleLimit.store(limit, std::memory_order_relaxed);
    } __except (EXCEPTION_EXECUTE_HANDLER) { }
    calls.fetch_add(1, std::memory_order_relaxed);
    original(terrain, position, a, b, c, d, flag, limit);
}
}

bool BindNativeTerrainCallProbe()
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
        host->log("earthworks  native angle-limited terrain-call probe unavailable; export/IAT/prologue mismatch");
        return false;
    }
    if (!host->patchIat(host->exeModule, "C3DDLL64.dll", Symbol, reinterpret_cast<void*>(HookAngleLimit),
        reinterpret_cast<void**>(&original), "Earthworks native terrain-call probe")) {
        host->log("earthworks  native angle-limited terrain-call IAT hook was not installed");
        return false;
    }
    bound.store(true, std::memory_order_release);
    host->log("earthworks  read-only angle-limited terrain-call probe bound at C3DDLL64+0x%llX via EXE IAT+0x%llX",
        static_cast<unsigned long long>(AngleLimitRva), static_cast<unsigned long long>(AngleLimitIatRva));
    return true;
}

NativeTerrainCallView ReadNativeTerrainCallProbe() noexcept
{
    NativeTerrainCallView out;
    out.bound = bound.load(std::memory_order_acquire);
    out.calls = calls.load(std::memory_order_relaxed);
    out.caller = caller.load(std::memory_order_relaxed);
    out.callerInGame = callerInGame.load(std::memory_order_relaxed);
    out.x = x.load(std::memory_order_relaxed);
    out.z = z.load(std::memory_order_relaxed);
    for (int i = 0; i < 4; ++i) out.brush[i] = brush[i].load(std::memory_order_relaxed);
    out.flag = lastFlag.load(std::memory_order_relaxed);
    out.angleLimit = angleLimit.load(std::memory_order_relaxed);
    return out;
}
} // namespace ew
