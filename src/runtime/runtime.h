#pragma once
#include "../core/planning.h"
#include "tesmio_api.h"
#include "native_terrain_edit_hook.h"
#include "../core/state_store.h"
#include "workforce_probe.h"
#include <atomic>
#include <cstdint>

namespace ew {
enum class Action { Pick, Select, NewSite, SetShapeMode, SetGridSnap, SetSiteSafeAngles, FinishSite, CloseSiteEditor, CancelSiteDraft, SetSiteTarget, SetSitePointTarget, AdjustSitePointTarget, SampleSiteTarget, SampleSitePointTarget, SelectSitePoint, AddSitePoint, RemoveSitePoint, MoveSitePoint, MoveSiteShape, Sample, CancelPick, Target, Clear, Undo, Redo, ConfirmProject, SetDraftSiteName, SelectProject, SelectNamedProject, CreateNamedProject, RenameNamedProject, DeleteNamedProject, RenameSite, AddProjectFacility, RemoveProjectFacility, MoveProjectUp, MoveProjectDown, AssignProject, ClearProjectAssignment, StartProject, RestartProject, PauseProject, ResumeProject, ReviewResumeProject, CancelProject, InspectBuilding, ChooseBuilding, ClearInspector, LogInspector, RemoveProject, RefreshFacilities, DismissFacilityWarning, RemoveUnresolvedFacilityAssignments };
enum class PickMode { DrawSite, Sample, None, InspectBuilding, SampleSitePointTarget };
struct Command { Action action; float value = 0; uint64_t generation = 0; uint64_t projectId = 0; unsigned pointIndex = 0; Point point{}; bool allSitePoints=false; char text[MaxSavedNameBytes+1]{}; };
struct NamedProjectBrief { uint64_t id=0; char name[MaxSavedNameBytes+1]{}; unsigned facilityCount=0; };
struct ProjectFacilityBrief {
    unsigned id=0,eligibleMachines=0;
    unsigned eligibleMachineRateUnits=0;
    float distance=0;
    uint64_t assignedProjectId=0;
    bool machineCountValid=false,machineRateValid=false;
    char assetName[64]{};
    char displayName[257]{};
    char assignedProjectName[MaxSavedNameBytes+1]{};
};
struct ProjectBrief {
    uint64_t id = 0;
    char name[MaxSavedNameBytes+1]{};
    ProjectStatus status = ProjectStatus::Draft;
    unsigned facilityId = 0;
    unsigned allocatedWorkers = 0, allocatedMachines = 0, allocatedRateUnits = 0;
    float progress = 0;
    bool needsReview = false;
    SiteOutline outline{};
    Point a{},b{};
    float target = 0;
};
struct Snapshot {
    bool ready = false, cursorValid = false, cameraValid = false, canUndo = false, canRedo = false;
    bool facilityAssignmentWarning = false;
    bool projectQueued = false;
    bool assignedFacilityFound = false;
    ProjectStatus projectStatus = ProjectStatus::Draft;
    bool projectNeedsReview = false;
    float projectProgress = 0;
    float terrainCellSize = 0;
    float terrainGridOriginX=0,terrainGridOriginZ=0,terrainGridStepX=0,terrainGridStepZ=0;
    bool terrainGridValid=false,lockToGrid=false;
    uint64_t generation = 0, revision = 0;
    uint64_t projectId = 0;
    size_t projectCount = 0, projectTotal = 0;
    size_t namedProjectCount = 0;
    std::array<NamedProjectBrief,MaxSavedProjects> namedProjects{};
    uint64_t selectedNamedProjectId = 0;
    char projectContainerName[MaxSavedNameBytes+1]{};
    char selectedSiteName[MaxSavedNameBytes+1]{};
    char siteDraftName[MaxSavedNameBytes+1] = "Site-1";
    bool siteEditorOpen = false, creatingSite = false;
    SiteShapeMode shapeMode = SiteShapeMode::FreeDraw;
    uint64_t editingSiteId = 0;
    unsigned selectedSitePoint = 0;
    uint32_t selectedSitePointsMask = 1;
    Plan editorPlan{};
    Volumes editorVolumes{};
    bool editorVolumesValid=false;
    unsigned assignedEligibleMachines=0, assignedMachineRateUnits=0;
    unsigned editorAllocatedMachines=0,editorAllocatedRateUnits=0;
    bool assignedMachineReadingsValid=false;
    char siteDraftError[192]{};
    char siteActionError[192]{};
    unsigned assignedFacilityCount = 0, facilityCandidateCount = 0;
    bool facilityListTruncated=false;
    std::array<unsigned,MaxProjectFacilities> assignedFacilityIds{};
    std::array<ProjectFacilityBrief,MaxProjectFacilities> assignedFacilities{};
    std::array<ProjectFacilityBrief,WorldEarthworksFacilities> facilityCandidates{};
    unsigned projectFacilityId = 0;
    float projectFacilityDistance = 0;
    int day = 0, year = 0;
    float time = 0;
    Point cursor{};
    float view[16]{}, projection[16]{};
    Plan plan{};
    Plan queuedPlan{};
    std::array<ProjectBrief,MaxVisibleProjects> projects{};
    Volumes volumes{};
    WorkforceView workforce{};
    PickMode mode = PickMode::None;
    char message[160] = "Load a republic to begin.";
};
extern const TsmHost* host;
extern std::atomic<bool> panelOpen, mouseCapture, keyboardCapture, leftRelease, mapPickCapture, escapeCapture;
bool LeftTaken();
bool InstallRuntime();
void ClearInputLatches();
const Snapshot& ReadSnapshot();
void Enqueue(Command command);
void NotifyNativeTerrainEdit(float x, float z, float brushRadius) noexcept;
void StartOverlay();
}
