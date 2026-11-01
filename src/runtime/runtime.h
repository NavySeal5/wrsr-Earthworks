#pragma once
#include "../core/planning.h"
#include "tesmio_api.h"
#include "terrain_probe.h"
#include "workforce_probe.h"
#include <atomic>
#include <cstdint>

namespace ew {
enum class Action { Pick, Select, Sample, Target, Clear, Undo, Redo, ConfirmProject, CaptureTest, RaiseTest, LowerTest, RecheckTest, InspectBuilding, ChooseBuilding, RegisterBuilding, ClearRegistration, ClearInspector, LogInspector };
enum class PickMode { Corners, Sample, None, InspectBuilding };
struct Command { Action action; float value = 0; uint64_t generation = 0; };
struct Snapshot {
    bool ready = false, cursorValid = false, cameraValid = false, canUndo = false, canRedo = false;
    bool projectQueued = false;
    uint64_t generation = 0, revision = 0;
    uint64_t projectId = 0;
    int day = 0, year = 0;
    float time = 0;
    Point cursor{};
    float view[16]{}, projection[16]{};
    Plan plan{};
    Volumes volumes{};
    ProbeView probe{};
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
void StartOverlay();
}
