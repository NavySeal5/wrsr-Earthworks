#pragma once
#include "../core/planning.h"
#include "tesmio_api.h"
#include "terrain_probe.h"
#include "native_terrain_probe.h"
#include "../core/state_store.h"
#include "workforce_probe.h"
#include <atomic>
#include <cstdint>

namespace ew {
enum class Action { Pick, Select, Sample, Target, Clear, Undo, Redo, ConfirmProject, AssignProject, ClearProjectAssignment, StartProject, PauseProject, ResumeProject, ReviewResumeProject, CancelProject, CaptureTest, RaiseTest, LowerTest, RecheckTest, InspectBuilding, ChooseBuilding, ClearInspector, LogInspector };
enum class PickMode { Corners, Sample, None, InspectBuilding };
struct Command { Action action; float value = 0; uint64_t generation = 0; };
struct Snapshot {
    bool ready = false, cursorValid = false, cameraValid = false, canUndo = false, canRedo = false;
    bool projectQueued = false;
    bool assignedFacilityFound = false;
    ProjectStatus projectStatus = ProjectStatus::Draft;
    bool projectNeedsReview = false;
    float projectProgress = 0;
    uint64_t generation = 0, revision = 0;
    uint64_t projectId = 0;
    unsigned projectFacilityId = 0;
    float projectFacilityDistance = 0;
    int day = 0, year = 0;
    float time = 0;
    Point cursor{};
    float view[16]{}, projection[16]{};
    Plan plan{};
    Plan queuedPlan{};
    Volumes volumes{};
    ProbeView probe{};
    NativeTerrainCallView nativeTerrainCall{};
    WorkforceView workforce{};
    PickMode mode = PickMode::Corners;
    char message[160] = "Load a republic to begin.";
};
extern const TsmHost* host;
extern std::atomic<bool> panelOpen, mouseCapture, keyboardCapture, leftRelease;
bool LeftTaken();
bool InstallRuntime();
void ClearInputLatches();
Snapshot ReadSnapshot();
void Enqueue(Command command);
void NotifyNativeTerrainEdit(float x, float z, float brushRadius) noexcept;
void StartOverlay();
}
