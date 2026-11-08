#pragma once
#include "planning.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ew {

constexpr uint32_t StateSchemaVersion = 9;
constexpr size_t MaxSavedProjects = 32;
constexpr size_t MaxSavedEarthworks = 128;
constexpr size_t MaxVisibleProjects = MaxSavedEarthworks;
constexpr size_t MaxProjectFacilities = 16;
constexpr size_t MaxSavedNameBytes = 48;

enum class ProjectStatus : uint8_t { Draft = 0, Queued = 1, Running = 2, Paused = 3, Completed = 4, Cancelled = 5, Waiting = 6 };

struct SavedProject {
    uint64_t id = 0;
    std::string name;
    std::vector<uint32_t> facilityIds;
};

struct SavedEarthwork {
    uint64_t id = 0;
    uint64_t projectId = 0;
    std::string name;
    ProjectStatus status = ProjectStatus::Draft;
    uint32_t executionCursor = 0;
    float progress = 0;
    // Baseline terrain work measured on the native terrain grid at Site start.
    // Kept separate from the coarse planning cut/fill estimate for UI progress.
    float totalWorkM3 = 0;
    bool needsReview = false;
    float workRemainderM3 = 0;
    unsigned allocatedWorkers = 0, allocatedMachines = 0;
    Plan plan{};
};

struct DurableState {
    uint32_t schema = StateSchemaVersion;
    std::array<uint8_t, 32> terrainFingerprint{};
    Plan draft{};
    std::string draftName = "Site-1";
    std::vector<SavedProject> projects;
    std::vector<SavedEarthwork> earthworks;
    uint64_t selectedProjectId = 0;
    uint32_t nextSiteNumber = 1;
};

// Versioned, bounded binary representation for a companion file. It contains
// values only: no game pointers, native handles, or vehicle reservations.
bool EncodeState(const DurableState& state, std::vector<uint8_t>* bytes);
bool DecodeState(const uint8_t* bytes, size_t size, DurableState* state);
bool FingerprintMatches(const DurableState& state, const std::array<uint8_t, 32>& current) noexcept;

} // namespace ew
