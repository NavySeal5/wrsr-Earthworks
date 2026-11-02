#pragma once
#include "planning.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ew {

constexpr uint32_t StateSchemaVersion = 4;
constexpr size_t MaxSavedProjects = 128;
constexpr size_t MaxVisibleProjects = MaxSavedProjects;

enum class ProjectStatus : uint8_t { Draft = 0, Queued = 1, Running = 2, Paused = 3, Completed = 4, Cancelled = 5, Waiting = 6 };

struct SavedProject {
    uint64_t id = 0;
    ProjectStatus status = ProjectStatus::Draft;
    uint32_t facilityId = 0;
    uint32_t executionCursor = 0;
    float progress = 0;
    bool needsReview = false;
    float workRemainderM3 = 0;
    unsigned allocatedWorkers = 0, allocatedMachines = 0;
    Plan plan{};
};

struct DurableState {
    uint32_t schema = StateSchemaVersion;
    std::array<uint8_t, 32> terrainFingerprint{};
    Plan draft{};
    std::vector<SavedProject> projects;
};

// Versioned, bounded binary representation for a companion file. It contains
// values only: no game pointers, native handles, or vehicle reservations.
bool EncodeState(const DurableState& state, std::vector<uint8_t>* bytes);
bool DecodeState(const uint8_t* bytes, size_t size, DurableState* state);
bool FingerprintMatches(const DurableState& state, const std::array<uint8_t, 32>& current) noexcept;

} // namespace ew
