// SPDX-License-Identifier: GPL-3.0-only
// Native binding addresses, input interception and camera layout adapted from
// RepublicMP (E3418), commit 630775724abd5e3346eb59f9c66e2873128d74a4.
#include <windows.h>
#include "runtime.h"
#include "save_store.h"
#include "vehicle_filter.h"
#include "native_terrain_probe.h"
#include "../core/terrain_grid.h"
#include "../core/productivity.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <mutex>
#include <utility>

namespace ew {
const TsmHost* host = nullptr;
std::atomic<bool> panelOpen{false}, mouseCapture{false}, keyboardCapture{false}, leftRelease{false}, mapPickCapture{false};
namespace {
std::mutex dataMutex;
Snapshot published, current;
std::deque<Command> commands;
History history;
DurableState durableState;
uint64_t selectedProjectId=0;
bool savePending = false;
bool loadMessagePending = false;
ULONGLONG lastFacilityRefresh = 0;
using LateFn = void (*)(__int64);
using LoadFn = void (*)();
using HeightFn = float (*)(void*, const float*);
using RaiseFn = void (*)(void*,const float*,float,float,float,float,bool);
using TextureFn = void (*)(void*);
LateFn originalLate = nullptr;
LoadFn originalLoad = nullptr;
HeightFn heightQuery = nullptr;
RaiseFn raiseLower = nullptr;
TextureFn openHeightmap = nullptr, closeHeightmap = nullptr;
double lastSimulationHour = -1;
ULONGLONG lastCompletionCheck = 0;
using ConstructionAssignFn = void (*)(void*,void*,void*,float,bool,bool);
ConstructionAssignFn originalConstructionAssign = nullptr;
using VehicleSlotFn = unsigned char (*)(void*,void*,void*,uintptr_t,int*);
VehicleSlotFn originalVehicleSlot = nullptr;
constexpr size_t LateRva = 0x30D1A0, LoadRva = 0x430FC0, ConstructionAssignRva=0x161C90, VehicleSlotRva=0x3E3F40;
const unsigned char lateBytes[] = {0x40,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8D,0xAC,0x24,0xE0,0xDB,0xFF,0xFF};
const unsigned char loadBytes[] = {0x40,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8D,0xAC,0x24,0x00,0x38,0xFF,0xFF};
const unsigned char constructionAssignBytes[] = {0x48,0x8B,0xC4,0xF3,0x0F,0x11,0x58,0x20,0x4C,0x89,0x40,0x18,0x48,0x89,0x50,0x10,0x53,0x56};
const unsigned char vehicleSlotBytes[] = {0x44,0x89,0x4C,0x24,0x20,0x53,0x56,0x41,0x54,0x41,0x55,0x41,0x56,0x48,0x83,0xEC,0x50};
bool ReadNative(Snapshot* out)
{
    __try {
        auto* b = host->exeBase;
        out->ready = *reinterpret_cast<void**>(b + 0x9E46B0) != nullptr && b[0x993C65] != 0;
        if (!out->ready) return true;
        const auto* world = b + 0x9D4F10;
        std::memcpy(&out->cursor, world + 0xF78, sizeof(Point));
        out->cursorValid = std::isfinite(out->cursor.x) && std::isfinite(out->cursor.y) && std::isfinite(out->cursor.z)
            && (out->cursor.x != 0 || out->cursor.z != 0);
        out->day = *reinterpret_cast<const int*>(world+0x590);
        out->year = *reinterpret_cast<const int*>(world+0x594);
        out->time = *reinterpret_cast<const float*>(world+0x59C);
        const auto* camera = *reinterpret_cast<unsigned char**>(b+0x9EAD10);
        out->cameraValid = camera != nullptr;
        if (camera) {
            std::memcpy(out->view, camera, sizeof(out->view));
            std::memcpy(out->projection, camera+0xC8, sizeof(out->projection));
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool ReadHeight(float x, float z, float* out)
{
    __try {
        const auto* scene = *reinterpret_cast<unsigned char**>(host->exeBase+0x9941F0);
        if (!scene) return false;
        auto* terrain = *reinterpret_cast<void* const*>(scene+0xED8);
        if (!terrain) return false;
        const float p[] = {x, 0, z};
        *out = heightQuery(terrain, p);
        return std::isfinite(*out) && std::abs(*out) < 100000;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void* TerrainObject()
{
    __try {
        auto* holder=*reinterpret_cast<unsigned char**>(host->exeBase+0x9941F0);
        return holder ? *reinterpret_cast<void* const*>(holder+0xED8) : nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
bool ReadTerrainGrid(void* terrain,TerrainGrid* out)
{
    __try {
        auto* p=static_cast<unsigned char*>(terrain);
        out->originX=*reinterpret_cast<float*>(p+0x890);out->originZ=*reinterpret_cast<float*>(p+0x898);
        out->heightScale=*reinterpret_cast<float*>(p+0x89C);out->width=*reinterpret_cast<int*>(p+0x220);
        out->depth=*reinterpret_cast<int*>(p+0x224);
        out->stepX=*reinterpret_cast<float*>(p+0x87C)/out->width;out->stepZ=*reinterpret_cast<float*>(p+0x880)/out->depth;
        return out->width>=64 && out->width<=8192 && out->depth>=64 && out->depth<=8192
            && out->stepX>0.1f && out->stepX<50 && out->stepZ>0.1f && out->stepZ<50
            && std::isfinite(out->originX) && std::isfinite(out->originZ)
            && std::isfinite(out->stepX) && std::isfinite(out->stepZ)
            && std::isfinite(out->heightScale) && out->heightScale>10 && out->heightScale<100000;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool ApplyProjectVertex(void* terrain,const TerrainGrid& grid,int x,int z,float delta)
{
    __try {
        const Point point=grid.At(x,z);const float pos[]={point.x,0,point.z};
        raiseLower(terrain,pos,0.20f,0.45f,delta/grid.heightScale,-1.0f,false);return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
enum class ProjectAreaError { None, TerrainMissing, GridInvalid, OutsideHeightmap, TooSmall, TooLarge };
ProjectAreaError CheckProjectArea(const Plan& plan,TerrainGrid* gridOut,GridRect* rectOut)
{
    void* terrain=TerrainObject();
    if(!terrain)return ProjectAreaError::TerrainMissing;
    TerrainGrid grid{};
    const bool gridValid=ReadTerrainGrid(terrain,&grid);
    if(gridOut)*gridOut=grid;
    if(!gridValid)return ProjectAreaError::GridInvalid;
    if(!plan.complete || !SnapInside(plan,grid,rectOut)) {
        const float x0=(std::min(plan.a.x,plan.b.x)-grid.originX)/grid.stepX;
        const float x1=(std::max(plan.a.x,plan.b.x)-grid.originX)/grid.stepX;
        const float z0=(std::min(plan.a.z,plan.b.z)-grid.originZ)/grid.stepZ;
        const float z1=(std::max(plan.a.z,plan.b.z)-grid.originZ)/grid.stepZ;
        if(!std::isfinite(x0)||!std::isfinite(x1)||!std::isfinite(z0)||!std::isfinite(z1)
            || x0<3 || z0<3 || x1>static_cast<float>(grid.width-4) || z1>static_cast<float>(grid.depth-4))
            return ProjectAreaError::OutsideHeightmap;
        return ProjectAreaError::TooSmall;
    }
    const int count=rectOut->Count();
    if(count<=0)return ProjectAreaError::TooSmall;
    if(count>40000)return ProjectAreaError::TooLarge;
    return ProjectAreaError::None;
}
const char* ProjectAreaErrorText(ProjectAreaError error)
{
    switch(error) {
    case ProjectAreaError::None:return "valid";
    case ProjectAreaError::TerrainMissing:return "terrain object unavailable";
    case ProjectAreaError::GridInvalid:return "heightmap dimensions or spacing unreadable";
    case ProjectAreaError::OutsideHeightmap:return "footprint outside the supported heightmap interior";
    case ProjectAreaError::TooSmall:return "footprint contains no complete heightmap cell";
    case ProjectAreaError::TooLarge:return "footprint exceeds the 40,000-vertex limit";
    }
    return "unknown geometry error";
}
const char* ProjectAreaMessage(ProjectAreaError error)
{
    switch(error) {
    case ProjectAreaError::None:return "Project area is valid.";
    case ProjectAreaError::TerrainMissing:return "Terrain is not ready. The project remains queued; try again when the world is fully loaded.";
    case ProjectAreaError::GridInvalid:return "The terrain heightmap could not be read. The project remains queued; capture the runtime log if this repeats.";
    case ProjectAreaError::OutsideHeightmap:return "The footprint reaches outside the supported heightmap area. Replan inside the playable terrain.";
    case ProjectAreaError::TooSmall:return "The footprint is smaller than one terrain cell. Enlarge the selected area.";
    case ProjectAreaError::TooLarge:return "The footprint exceeds the current 40,000-vertex limit. Reduce the area.";
    }
    return "Terrain area validation failed.";
}
bool AvailabilitySignal()
{
    // These two fixed-address scalars outlive worlds. Late stops running in
    // menus, so the renderer also gates snapshots on this lifecycle signal.
    // No terrain/camera object is followed or queried from the renderer.
    __try { return host->exeBase[0x993C65] != 0 && *reinterpret_cast<void**>(host->exeBase+0x9E46B0) != nullptr; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void Message(const char* text) { strcpy_s(current.message, text); }
bool ProjectReservesFootprint(ProjectStatus status)
{
    return status==ProjectStatus::Queued||status==ProjectStatus::Running||status==ProjectStatus::Waiting||status==ProjectStatus::Paused;
}
bool FootprintsOverlap(const Plan& a,const Plan& b)
{
    const float ax0=std::min(a.a.x,a.b.x),ax1=std::max(a.a.x,a.b.x);
    const float az0=std::min(a.a.z,a.b.z),az1=std::max(a.a.z,a.b.z);
    const float bx0=std::min(b.a.x,b.b.x),bx1=std::max(b.a.x,b.b.x);
    const float bz0=std::min(b.a.z,b.b.z),bz1=std::max(b.a.z,b.b.z);
    return std::max(ax0,bx0)<=std::min(ax1,bx1)&&std::max(az0,bz0)<=std::min(az1,bz1);
}
SavedProject* FindProject(uint64_t id)
{
    for(auto& project:durableState.projects)if(project.id==id)return &project;
    return nullptr;
}
void Changed()
{
    durableState.draft = current.plan;
    current.volumes = Estimate(current.plan);
    ++current.revision;
    host->log("earthworks  draft-only revision=%llu world=%llu complete=%d target=%.3f area=%.2f cut=%.2f fill=%.2f",
        current.revision, current.generation, current.plan.complete ? 1 : 0, current.plan.target,
        static_cast<double>(current.plan.Width())*current.plan.Depth(), current.volumes.cut, current.volumes.fill);
}
void ResetWorld()
{
    const auto generation = current.generation+1;
    current = Snapshot{}; current.generation = generation;
    durableState = DurableState{}; selectedProjectId=0; savePending = false; loadMessagePending = false;lastSimulationHour=-1;
    history.Reset(); panelOpen = false; mouseCapture = false; keyboardCapture = false; mapPickCapture = false;
    ProbeWorldBoundary();
    ResetWorkforce();
    std::lock_guard<std::mutex> lock(dataMutex);
    commands.clear(); published = current;
    host->log("earthworks  world boundary; discarded session preview, generation=%llu", generation);
}
void RefreshProjectStatus()
{
    current.projectTotal=durableState.projects.size();
    current.projectCount=std::min(durableState.projects.size(),MaxVisibleProjects);
    for(size_t i=0;i<current.projectCount;++i) {
        const auto& source=durableState.projects[i];auto& target=current.projects[i];
        target.id=source.id;target.status=source.status;target.facilityId=source.facilityId;
        target.allocatedWorkers=source.status==ProjectStatus::Running?source.allocatedWorkers:0;
        target.allocatedMachines=source.status==ProjectStatus::Running?source.allocatedMachines:0;
        target.progress=source.progress;target.needsReview=source.needsReview;
        target.a=source.plan.a;target.b=source.plan.b;target.target=source.plan.target;
    }
    current.projectQueued=false;current.projectId=0;
    current.projectFacilityId=0;
    current.assignedFacilityFound=false;
    current.projectStatus=ProjectStatus::Draft;
    current.projectNeedsReview=false;
    current.projectProgress=0;
    current.projectFacilityDistance=0;
    current.queuedPlan=Plan{};
    auto selected=durableState.projects.end();
    if(selectedProjectId)selected=std::find_if(durableState.projects.begin(),durableState.projects.end(),[](const SavedProject& p){return p.id==selectedProjectId;});
    if(selected==durableState.projects.end()) {
        selected=std::find_if(durableState.projects.begin(),durableState.projects.end(),[](const SavedProject& p){return p.status==ProjectStatus::Queued||p.status==ProjectStatus::Running||p.status==ProjectStatus::Waiting||p.status==ProjectStatus::Paused;});
        if(selected==durableState.projects.end()&&!durableState.projects.empty())selected=durableState.projects.begin();
    }
    if(selected!=durableState.projects.end()) {
        selectedProjectId=selected->id;current.projectQueued=true;current.projectId=selected->id;current.queuedPlan=selected->plan;
        current.projectStatus=selected->status;current.projectNeedsReview=selected->needsReview;current.projectProgress=selected->progress;
        current.projectFacilityId=selected->facilityId;
        if(selected->facilityId) {
            BuildingReading facility{};
            if(FindEarthworksFacility(selected->facilityId,&facility)) {
                current.assignedFacilityFound=true;
                const Point center{(selected->plan.a.x+selected->plan.b.x)*0.5f,0,(selected->plan.a.z+selected->plan.b.z)*0.5f};
                current.projectFacilityDistance=std::hypot(center.x-facility.position.x,center.z-facility.position.z);
            }
        }
    }
}
bool SamplePlan(Plan* plan)
{
    for (int z=0; z<GridSide; ++z) for (int x=0; x<GridSide; ++x) {
        const auto p = plan->Vertex(x,z,true);
        if (!ReadHeight(p.x,p.z,&plan->ground[static_cast<size_t>(z*GridSide+x)])) return false;
    }
    return true;
}
void ProcessProbe(ProbeAction action)
{
    const unsigned edits=ReadProbeView().edits;
    RunProbe(action,current.plan);
    const auto result=ReadProbeView();
    if(result.edits!=edits) {
        current.plan=Plan{};history.Reset();current.mode=PickMode::None;Changed();
        Message("Diagnostic edit changed real ground. Old preview cleared; inspect the test measurements below.");
    } else Message("See the Stage 3 test status and measurements below.");
}
void Process(const Command& command)
{
    if (command.generation != current.generation || !current.ready) return;
    switch(command.action) {
    case Action::Select:
        history.Commit(current.plan); current.plan = Plan{}; Changed();
        current.mode = PickMode::Corners;
        Message("Click the first corner on the ground.");
        break;
    case Action::Sample:
        current.mode = PickMode::Sample;
        Message("Click the ground to sample its height.");
        break;
    case Action::Pick: {
        if (!current.cursorValid) { Message("No ground under the cursor. Try another point."); break; }
        if(current.mode==PickMode::InspectBuilding) {
            FindWorkforceBuildings(current.cursor);current.mode=PickMode::None;
            Message("Select your Earthworks facility in the Projects tab, then assign it to the selected project.");break;
        }
        Point p = current.cursor;
        if (!ReadHeight(p.x,p.z,&p.y)) { Message("Cannot sample terrain here. Try another point."); break; }
        if (current.mode == PickMode::Sample) {
            history.Commit(current.plan); current.plan.target = p.y; current.mode = PickMode::None;
            Message("Height sampled. Adjust the target or select a new area."); Changed();
        } else if (current.mode == PickMode::Corners) {
            Plan next = current.plan;
            if (!next.first || next.complete) {
                history.Commit(current.plan); next = Plan{}; next.first = true; next.a = p; next.b = p; next.target = p.y;
                current.plan = next; Message("Click the opposite corner. Right-drag and zoom still work."); Changed();
            } else {
                next.b = p;
                if (next.Width()<0.5f || next.Depth()<0.5f) { Message("Corners are too close: choose a rectangle at least 0.5 m on each side."); break; }
                if (!SamplePlan(&next)) { Message("Could not read the whole area. Choose a smaller or different rectangle."); break; }
                next.complete = true; history.Commit(current.plan); current.plan = next; current.mode = PickMode::None;
                Message("Preview ready. Actual terrain is unchanged."); Changed();
            }
        }
        break;
    }
    case Action::Target:
        if (std::isfinite(command.value) && std::abs(command.value)<100000) {
            history.Commit(current.plan); current.plan.target = command.value; Changed(); Message("Target updated. Actual terrain is unchanged.");
        } else Message("Enter a finite height within the terrain preview's numeric range.");
        break;
    case Action::Clear:
        history.Commit(current.plan); current.plan = Plan{}; current.mode = PickMode::None;
        Changed(); Message("Preview cleared. Select area to begin again."); break;
    case Action::Undo:
        current.plan = history.Undo(current.plan);
        current.mode = current.plan.complete ? PickMode::None : PickMode::Corners;
        Changed(); Message("Undid the last draft edit."); break;
    case Action::Redo:
        current.plan = history.Redo(current.plan);
        current.mode = current.plan.complete ? PickMode::None : PickMode::Corners;
        Changed(); Message("Redid the draft edit."); break;
    case Action::ConfirmProject: {
        if(!current.plan.complete) { Message("Select a complete area before confirming a project."); break; }
        if(durableState.projects.size()>=MaxSavedProjects) {Message("Project history reached 128 records. Use a new save for another project list.");break;}
        for(const auto& saved:durableState.projects)if(ProjectReservesFootprint(saved.status)&&FootprintsOverlap(current.plan,saved.plan)) {
            Message("Project overlaps a queued or active footprint. Choose another area or cancel the other project first.");return;
        }
        SavedProject project;
        project.id=1;
        for(const auto& saved:durableState.projects)project.id=std::max(project.id,saved.id+1);
        project.status=ProjectStatus::Queued;
        project.plan=current.plan;
        durableState.projects.push_back(project);
        selectedProjectId=project.id;
        RefreshProjectStatus();
        current.plan=Plan{};
        current.mode=PickMode::None;
        history.Reset();
        Changed();
        host->log("earthworks  project confirmed and queued id=%llu priority=%zu",static_cast<unsigned long long>(project.id),durableState.projects.size());
        Message("Project queued. Assign an Earthworks facility, then start it when ready.");
        break;
    }
    case Action::SelectProject:
        if(FindProject(command.projectId)){selectedProjectId=command.projectId;RefreshProjectStatus();}
        break;
    case Action::MoveProjectUp:
    case Action::MoveProjectDown: {
        auto it=std::find_if(durableState.projects.begin(),durableState.projects.end(),[&](const SavedProject& p){return p.id==command.projectId;});
        if(it==durableState.projects.end()||!ProjectReservesFootprint(it->status))break;
        if(command.action==Action::MoveProjectUp) {
            auto prior=it;bool found=false;
            while(prior!=durableState.projects.begin()){--prior;if(ProjectReservesFootprint(prior->status)){found=true;break;}}
            if(found)std::iter_swap(it,prior);
        } else {
            auto next=it;bool found=false;
            while(++next!=durableState.projects.end())if(ProjectReservesFootprint(next->status)){found=true;break;}
            if(found)std::iter_swap(it,next);
        }
        RefreshProjectStatus();Message("Project priority order updated.");break;
    }
    case Action::AssignProject: {
        if(!current.projectQueued) { Message("Select a project before assigning a facility."); break; }
        if(current.projectStatus!=ProjectStatus::Queued&&current.projectStatus!=ProjectStatus::Paused) {Message("Only queued or paused projects can be assigned or reassigned.");break;}
        const auto workforce=ReadWorkforce();
        if(!workforce.selected || !workforce.buildingValid || !workforce.building.earthworksFacility) {
            Message("Select a recognized Earthworks facility in the inspector first.");break;
        }
        for(auto& project:durableState.projects)if(project.id==current.projectId && (project.status==ProjectStatus::Queued||project.status==ProjectStatus::Paused)) {
            project.facilityId=workforce.building.id;RefreshProjectStatus();
            SelectEarthworksFacility(project.facilityId);
            Message(project.status==ProjectStatus::Paused
                ? (project.needsReview?"Project reassigned. Progress is kept; review the footprint before resuming.":"Project reassigned while paused. Progress is kept.")
                : "Project assigned. It will not start until you press Start.");
            break;
        }
        break;
    }
    case Action::ClearProjectAssignment:
        for(auto& project:durableState.projects)if(project.id==current.projectId && project.status==ProjectStatus::Queued)project.facilityId=0;
        RefreshProjectStatus();Message("Facility assignment cleared. The queued project remains saved.");break;
    case Action::StartProject:
        for(auto& project:durableState.projects)if(project.id==current.projectId && project.status==ProjectStatus::Queued) {
            BuildingReading facility{};WorkforceView workforce{};
            if(!raiseLower || !heightQuery || !openHeightmap || !closeHeightmap) {
                Message("Native terrain writer is unavailable for this game build. Project remains queued.");break;
            }
            if(!project.facilityId || !FindEarthworksFacility(project.facilityId,&facility)) {
                Message("Assigned Earthworks facility is missing or unreadable.");break;
            }
            if(!ReadEarthworksFacilityWorkforce(project.facilityId,&workforce)||!workforce.buildingValid
                ||!workforce.occupantsValid||!workforce.vehiclesValid||!workforce.parkedValid) {
                Message("Assigned facility readings are incomplete. Try again when the building can be read.");break;
            }
            TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project.plan,&grid,&rect);
            if(areaError!=ProjectAreaError::None) {
                host->log("earthworks  start rejected project=%llu facility=%u area=%s grid=%dx%d origin=(%.3f,%.3f) step=(%.3f,%.3f) plan=(%.3f,%.3f)..(%.3f,%.3f)",
                    static_cast<unsigned long long>(project.id),project.facilityId,ProjectAreaErrorText(areaError),grid.width,grid.depth,
                    grid.originX,grid.originZ,grid.stepX,grid.stepZ,project.plan.a.x,project.plan.a.z,project.plan.b.x,project.plan.b.z);
                Message(ProjectAreaMessage(areaError));break;
            }
            Plan refreshed=project.plan;
            if(!SamplePlan(&refreshed)) {Message("Cannot recheck every planned terrain sample. Replan this project before starting.");break;}
            float maxDifference=0;
            for(size_t i=0;i<refreshed.ground.size();++i)maxDifference=std::max(maxDifference,std::abs(refreshed.ground[i]-project.plan.ground[i]));
            if(maxDifference>0.001f) {
                host->log("earthworks  start rejected project=%llu terrain changed after planning maxDelta=%.6f area=(%.3f,%.3f)..(%.3f,%.3f)",
                    static_cast<unsigned long long>(project.id),maxDifference,project.plan.a.x,project.plan.a.z,project.plan.b.x,project.plan.b.z);
                Message("Terrain changed since planning. Replan the footprint before starting.");break;
            }
            project.status=ProjectStatus::Running;project.needsReview=false;project.workRemainderM3=0;
            lastSimulationHour=static_cast<double>(current.year*366+current.day+current.time)*24.0;
            RefreshProjectStatus();Message("Project started. It works when its priority receives facility capacity.");break;
        }
        break;
    case Action::PauseProject:
        for(auto& project:durableState.projects)if(project.id==current.projectId && (project.status==ProjectStatus::Running||project.status==ProjectStatus::Waiting)) {
            project.status=ProjectStatus::Paused;RefreshProjectStatus();Message("Project paused. Current terrain and progress are kept.");break;
        }
        break;
    case Action::ResumeProject:
        for(auto& project:durableState.projects)if(project.id==current.projectId && project.status==ProjectStatus::Paused && !project.needsReview) {
            BuildingReading facility{};
            if(!project.facilityId||!FindEarthworksFacility(project.facilityId,&facility)){Message("Assign this paused project to an available Earthworks facility first.");break;}
            TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project.plan,&grid,&rect);
            if(areaError!=ProjectAreaError::None) {Message(ProjectAreaMessage(areaError));break;}
            project.status=ProjectStatus::Running;lastSimulationHour=static_cast<double>(current.year*366+current.day+current.time)*24.0;
            RefreshProjectStatus();Message("Project resumed.");break;
        }
        break;
    case Action::ReviewResumeProject:
        for(auto& project:durableState.projects)if(project.id==current.projectId && project.status==ProjectStatus::Paused && project.needsReview) {
            BuildingReading facility{};
            if(!project.facilityId||!FindEarthworksFacility(project.facilityId,&facility)){Message("Assign this paused project to an available Earthworks facility before reviewing it.");break;}
            TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project.plan,&grid,&rect);
            if(areaError!=ProjectAreaError::None) {Message(ProjectAreaMessage(areaError));break;}
            if(!SamplePlan(&project.plan)) {Message("Could not read the project footprint. It remains paused for review.");break;}
            project.needsReview=false;project.status=ProjectStatus::Running;
            lastSimulationHour=static_cast<double>(current.year*366+current.day+current.time)*24.0;
            RefreshProjectStatus();Message("Footprint reviewed. Project resumed toward its saved target.");break;
        }
        break;
    case Action::CancelProject:
        for(auto& project:durableState.projects)if(project.id==current.projectId &&
            (project.status==ProjectStatus::Queued || project.status==ProjectStatus::Running || project.status==ProjectStatus::Waiting || project.status==ProjectStatus::Paused)) {
            project.status=ProjectStatus::Cancelled;project.needsReview=false;RefreshProjectStatus();
            Message("Project cancelled. Terrain already changed is kept.");break;
        }
        break;
    case Action::CaptureTest: ProcessProbe(ProbeAction::Capture); break;
    case Action::RaiseTest: ProcessProbe(ProbeAction::Raise); break;
    case Action::LowerTest: ProcessProbe(ProbeAction::Lower); break;
    case Action::RecheckTest: ProcessProbe(ProbeAction::Recheck); break;
    case Action::InspectBuilding:
        current.mode=PickMode::InspectBuilding;Message("Click near your Earthworks facility on the map. Select it in the Projects tab.");break;
    case Action::ChooseBuilding:
        if(std::isfinite(command.value) && command.value>=0 && command.value<WorkforceCandidates)
            SelectWorkforceBuilding(static_cast<unsigned>(command.value));
        break;
    case Action::ClearInspector: ResetWorkforce();current.mode=PickMode::None;break;
    case Action::LogInspector: LogWorkforce();break;
    }
}
void NotifyNativeTerrainEditImpl(float x,float z,float brushRadius) noexcept
{
    if(!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(brushRadius))return;
    for(auto& project:durableState.projects)if(project.status==ProjectStatus::Running||project.status==ProjectStatus::Waiting) {
        const float minX=std::min(project.plan.a.x,project.plan.b.x),maxX=std::max(project.plan.a.x,project.plan.b.x);
        const float minZ=std::min(project.plan.a.z,project.plan.b.z),maxZ=std::max(project.plan.a.z,project.plan.b.z);
        const float dx=std::max({minX-x,0.0f,x-maxX});
        const float dz=std::max({minZ-z,0.0f,z-maxZ});
        const float radius=std::clamp(std::abs(brushRadius),0.0f,1000.0f);
        if(dx*dx+dz*dz<=radius*radius) {
            project.status=ProjectStatus::Paused;project.needsReview=true;
            RefreshProjectStatus();
            Message("Native terrain edit overlapped this project. It is paused; review the footprint before resuming.");
            host->log("earthworks  project=%llu paused for overlapping native terrain edit at (%.2f,%.2f), radius=%.2f",
                static_cast<unsigned long long>(project.id),x,z,radius);
            break;
        }
    }
}
void ExecuteProjectSlice(SavedProject* project,double relativeRate,double elapsed)
{
    if(!project||relativeRate<=0||elapsed<=0)return;
    TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project->plan,&grid,&rect);
    if(areaError==ProjectAreaError::TerrainMissing)return;
    if(areaError!=ProjectAreaError::None) {
        project->status=ProjectStatus::Paused;project->needsReview=false;RefreshProjectStatus();
        host->log("earthworks  project=%llu paused by area check: %s grid=%dx%d origin=(%.3f,%.3f) step=(%.3f,%.3f) plan=(%.3f,%.3f)..(%.3f,%.3f)",
            static_cast<unsigned long long>(project->id),ProjectAreaErrorText(areaError),grid.width,grid.depth,
            grid.originX,grid.originZ,grid.stepX,grid.stepZ,project->plan.a.x,project->plan.a.z,project->plan.b.x,project->plan.b.z);
        Message(ProjectAreaMessage(areaError));return;
    }
    void* terrain=TerrainObject();if(!terrain)return;
    // Provisional Stage 8 conversion: one relative manual-rate unit completes
    // 10 m3 per simulation hour. Balancing remains open under D10/D14.
    project->workRemainderM3=static_cast<float>(std::min(1.0e9,
        static_cast<double>(project->workRemainderM3)+relativeRate*10.0*std::min(elapsed,1.0)));
    const int count=rect.Count();
    const double cellArea=static_cast<double>(grid.stepX)*grid.stepZ;
    const double precision=static_cast<double>(grid.heightScale)/60000.0;
    const int batch=std::min(count,64);
    const double threshold=cellArea*precision*batch;
    if(project->workRemainderM3<threshold)return;
    const double maxStep=0.05;
    const double requestedStep=project->workRemainderM3/(cellArea*batch);
    const float step=static_cast<float>(std::min(requestedStep,maxStep));
    if(!std::isfinite(step) || step<=0)return;
    if(!openHeightmap || !closeHeightmap || !raiseLower || !heightQuery) {
        project->status=ProjectStatus::Paused;project->needsReview=true;RefreshProjectStatus();
        Message("Native terrain writer became unavailable. Project paused for review.");return;
    }
    __try { openHeightmap(terrain); } __except(EXCEPTION_EXECUTE_HANDLER) {
        project->status=ProjectStatus::Paused;project->needsReview=true;RefreshProjectStatus();Message("Terrain writer failed to open; project paused for review.");return;
    }
    double usedVolume=0;int edited=0;bool failed=false;
    for(int n=0;n<batch;++n) {
        const uint32_t linear=(project->executionCursor++)%static_cast<uint32_t>(count);
        const int x=rect.x0+static_cast<int>(linear%static_cast<uint32_t>(rect.x1-rect.x0+1));
        const int z=rect.z0+static_cast<int>(linear/static_cast<uint32_t>(rect.x1-rect.x0+1));
        const auto point=grid.At(x,z);float currentHeight=0;
        if(!ReadHeight(point.x,point.z,&currentHeight)) {failed=true;break;}
        const float difference=project->plan.target-currentHeight;
        if(std::abs(difference)<=precision)continue;
        const float delta=std::copysign(std::min(std::abs(difference),step),difference);
        if(!ApplyProjectVertex(terrain,grid,x,z,delta)) {failed=true;break;}
        float afterHeight=0;
        if(!ReadHeight(point.x,point.z,&afterHeight)) {failed=true;break;}
        const double applied=std::abs(static_cast<double>(afterHeight-currentHeight));
        usedVolume+=applied*cellArea;if(applied>0)++edited;
    }
    __try { closeHeightmap(terrain); } __except(EXCEPTION_EXECUTE_HANDLER) { failed=true; }
    if(failed) {
        project->status=ProjectStatus::Paused;project->needsReview=true;RefreshProjectStatus();
        Message("Terrain edit/readback failed. Project paused for review; inspect the area before resuming.");return;
    }
    project->workRemainderM3=static_cast<float>(std::max(0.0,static_cast<double>(project->workRemainderM3)-usedVolume));
    const auto estimate=Estimate(project->plan);const double total=estimate.cut+estimate.fill;
    if(total>0)project->progress=std::clamp(project->progress+static_cast<float>(usedVolume/total),0.0f,1.0f);
    const auto now=GetTickCount64();
    if(now-lastCompletionCheck>=500) {
        lastCompletionCheck=now;bool complete=true;
        for(int z=rect.z0;z<=rect.z1 && complete;++z)for(int x=rect.x0;x<=rect.x1;++x) {
            const auto point=grid.At(x,z);float value=0;
            if(!ReadHeight(point.x,point.z,&value) || std::abs(project->plan.target-value)>precision*1.1) {complete=false;break;}
        }
        if(complete) {project->progress=1;project->status=ProjectStatus::Completed;RefreshProjectStatus();Message("Earthworks project completed.");}
        else if(edited)current.projectProgress=project->progress;
    } else if(edited)current.projectProgress=project->progress;
}
void TickProjectRunner()
{
    if(!current.ready)return;
    const double clockHour=static_cast<double>(current.year*366+current.day+current.time)*24.0;
    if(lastSimulationHour<0){lastSimulationHour=clockHour;return;}
    const double elapsed=clockHour-lastSimulationHour;lastSimulationHour=clockHour;
    if(!std::isfinite(elapsed)||elapsed<=0)return;
    const double simulationElapsed=std::min(elapsed,1.0);
    struct Allocation { SavedProject* project=nullptr;double rate=0;unsigned workers=0,machines=0; };
    std::vector<unsigned> processedFacilities;
    std::vector<Allocation> allocations;
    bool schedulerStateChanged=false;
    allocations.reserve(durableState.projects.size());
    for(auto& first:durableState.projects) {
        if(first.status!=ProjectStatus::Running&&first.status!=ProjectStatus::Waiting)continue;
        if(!first.facilityId) {
            first.status=ProjectStatus::Paused;first.needsReview=false;schedulerStateChanged=true;
            Message("Started project has no facility assignment. Assign a facility before resuming.");continue;
        }
        if(std::find(processedFacilities.begin(),processedFacilities.end(),first.facilityId)!=processedFacilities.end())continue;
        const unsigned facilityId=first.facilityId;processedFacilities.push_back(facilityId);
        std::vector<SavedProject*> jobs;
        for(auto& project:durableState.projects)
            if(project.facilityId==facilityId&&(project.status==ProjectStatus::Running||project.status==ProjectStatus::Waiting))jobs.push_back(&project);
        WorkforceView workforce{};
        if(!ReadEarthworksFacilityWorkforce(facilityId,&workforce)||!workforce.buildingValid
            ||!workforce.occupantsValid||!workforce.parkedValid||!workforce.vehiclesValid) {
            for(auto* job:jobs){job->status=ProjectStatus::Paused;job->needsReview=true;schedulerStateChanged=true;}
            Message("Facility workforce or vehicle reading failed. Its projects are paused for review.");continue;
        }
        int workersLeft=static_cast<int>(std::min(workforce.occupants,65536u));
        unsigned machinesLeft=std::min(workforce.eligibleMachineCandidates,static_cast<unsigned>(MaxSavedProjects));
        std::vector<Allocation> facilityAllocations;
        facilityAllocations.reserve(jobs.size());
        for(auto* job:jobs)facilityAllocations.push_back({job,0,0,0});
        if(machinesLeft&&workersLeft>0) {
            // Give each started project one machine in priority order first,
            // then allocate additional machines to higher-priority projects.
            for(auto& allocation:facilityAllocations)if(machinesLeft&&workersLeft>0) {
                const int staff=std::min(workersLeft,10);workersLeft-=staff;--machinesLeft;
                allocation.rate+=MachineRate(staff,ReferenceGroundworksRating);
                allocation.workers+=static_cast<unsigned>(staff);++allocation.machines;
            }
            for(auto& allocation:facilityAllocations)while(machinesLeft&&workersLeft>0) {
                const int staff=std::min(workersLeft,10);workersLeft-=staff;--machinesLeft;
                allocation.rate+=MachineRate(staff,ReferenceGroundworksRating);
                allocation.workers+=static_cast<unsigned>(staff);++allocation.machines;
            }
        } else if(!machinesLeft&&workersLeft>0&&!facilityAllocations.empty()) {
            // Worker-only effort shares this pool with the highest-priority
            // started project; unused workers never get counted twice.
            facilityAllocations.front().rate=WorkerOnlyRate(workersLeft);
            facilityAllocations.front().workers=static_cast<unsigned>(workersLeft);
        }
        for(auto& allocation:facilityAllocations) {
            auto* job=allocation.project;
            job->allocatedWorkers=allocation.workers;job->allocatedMachines=allocation.machines;
            const auto next=allocation.rate>0?ProjectStatus::Running:ProjectStatus::Waiting;
            if(job->status!=next) {
                job->status=next;
                schedulerStateChanged=true;
                host->log("earthworks  project=%llu scheduler state=%s facility=%u rate=%.3f",
                    static_cast<unsigned long long>(job->id),next==ProjectStatus::Running?"running":"waiting",facilityId,allocation.rate);
            }
            allocations.push_back(allocation);
        }
    }
    bool changed=false;
    for(const auto& allocation:allocations)if(allocation.project&&allocation.project->status==ProjectStatus::Running&&allocation.rate>0) {
        const auto before=allocation.project->status;
        ExecuteProjectSlice(allocation.project,allocation.rate,simulationElapsed);
        if(before!=allocation.project->status)changed=true;
    }
    if(changed||schedulerStateChanged||!allocations.empty())RefreshProjectStatus();
}
void Tick()
{
    const bool wasReady = current.ready;
    if (!ReadNative(&current)) { current.ready = false; current.cursorValid = false; current.cameraValid = false; }
    if (wasReady && !current.ready) ResetWorld();
    if (!wasReady && current.ready) {
        if(loadMessagePending) loadMessagePending=false;
        else Message("Click Earthworks, then click two opposite corners.");
        host->log("earthworks  world ready generation=%llu date=%d/%d cursor=(%.2f,%.2f,%.2f)",
            current.generation,current.day,current.year,current.cursor.x,current.cursor.y,current.cursor.z);
    }
    std::deque<Command> pending;
    { std::lock_guard<std::mutex> lock(dataMutex); pending.swap(commands); }
    for (const auto& command:pending) {
        const Plan before=current.plan;
        Process(command);
        if (before.complete && command.action<Action::CaptureTest) {
            Plan check=before;
            if (SamplePlan(&check)) {
                float difference=0;
                for(size_t i=0;i<check.ground.size();++i) difference=std::max(difference,std::abs(check.ground[i]-before.ground[i]));
                host->log("earthworks  terrain readback: samples=%d maxDelta=%.6f after action=%d",GridSide*GridSide,difference,static_cast<int>(command.action));
                if (difference>0.001f) Message("Ground changed since selection. Select the area again to refresh this preview.");
            } else Message("Ground readback unavailable. Select the area again before relying on this preview.");
        }
    }
    current.canUndo = history.CanUndo(); current.canRedo = history.CanRedo();
    current.probe = ReadProbeView();
    current.nativeTerrainCall = ReadNativeTerrainCallProbe();
    if(current.ready)TickWorkforce();
    if(current.ready)TickProjectRunner();
    const auto facilityNow=GetTickCount64();
    if(current.projectQueued && current.projectFacilityId && facilityNow-lastFacilityRefresh>=500) {
        lastFacilityRefresh=facilityNow;RefreshProjectStatus();
    }
    current.workforce=ReadWorkforce();
    { std::lock_guard<std::mutex> lock(dataMutex); published = current; }
}
void HookedLate(__int64 world)
{
    ClearInputLatches();
    void* worldObject=reinterpret_cast<void*>(world);
    if (SaveIsActive(worldObject)) savePending=true;
    originalLate(world);
    if (SaveIsActive(worldObject)) savePending=true;
    else if (savePending) {
        durableState.draft=current.plan;
        if (SaveDurableState(host,worldObject,durableState)) host->log("earthworks  durable state saved with matching terrain fingerprint");
        else host->log("earthworks  durable state was not saved; native save is intact");
        savePending=false;
    }
    try { Tick(); } catch (...) { host->log("earthworks  preview tick failed; disabling input capture"); panelOpen = false; }
}
void HookedLoad()
{
    ResetWorld();
    originalLoad();
    DurableState loaded;
    const auto loadResult=LoadDurableState(host,host->exeBase+0x9D4F10,&loaded);
    if (loadResult==StateLoadResult::Loaded) {
        durableState=std::move(loaded);
        current.plan=durableState.draft;
        current.volumes=Estimate(current.plan);
        RefreshProjectStatus();
        if(current.projectFacilityId)SelectEarthworksFacility(current.projectFacilityId);
        ++current.revision;
        Message("Restored saved Earthworks drafts and projects.");
        loadMessagePending=true;
        host->log("earthworks  companion state restored after terrain fingerprint matched");
    } else if(loadResult==StateLoadResult::TerrainMismatch || loadResult==StateLoadResult::Corrupt) {
        Message(loadResult==StateLoadResult::TerrainMismatch
            ? "Saved Earthworks data belongs to different terrain. No plans were restored; terrain was not changed."
            : "Saved Earthworks data is damaged. No plans were restored; terrain was not changed.");
        loadMessagePending=true;
        host->log("earthworks  companion refused (%s); loaded save remains unchanged",loadResult==StateLoadResult::TerrainMismatch?"terrain mismatch":"corrupt data");
    } else {
        host->log(loadResult==StateLoadResult::Missing
            ? "earthworks  no companion state in this save"
            : "earthworks  companion state unavailable; loaded save remains unchanged");
    }
}
void HookedConstructionAssign(void* world,void* office,void* construction,float amount,bool first,bool second)
{
    if(IsEarthworksBuilding(office))return;
    originalConstructionAssign(world,office,construction,amount,first,second);
}
unsigned char HookedVehicleSlot(void* world,void* building,void* type,uintptr_t slotHint,int* slot)
{
    static char lastRejected[65]{};
    if(IsEarthworksFacility(building)) {
        char rejected[65]{};
        if(!AllowEarthworksVehicleType(type,rejected,sizeof(rejected))) {
            if(slot)*slot=-1;
            if(std::strcmp(lastRejected,rejected)!=0) {
                strcpy_s(lastRejected,rejected);
                host->log("earthworks  rejected vehicle type for dedicated station: %s",rejected);
            }
            return 0;
        }
    }
    return originalVehicleSlot(world,building,type,slotHint,slot);
}
using BoolFn = bool (*)(void*);
using KeyFn = bool (*)(void*,int);
using ButtonFn = bool (*)(void*,char*);
using AxisFn = float (*)(void*,char*,int*,float,float);
using VecFn = float* (*)(void*,float*);
BoolFn leftPress, leftAOff, leftDown, rightPress, rightAOff, rightDown, middleDown;
KeyFn keyDown,keyPress;
ButtonFn buttonDown,buttonPress;
AxisFn axis;
VecFn relative;
#define BOOL_HOOK(name,original,block) bool name(void* self) { const bool value=original(self); return (block) ? false:value; }
BOOL_HOOK(HLeftPress,leftPress,LeftTaken())
BOOL_HOOK(HLeftAOff,leftAOff,LeftTaken())
BOOL_HOOK(HLeftDown,leftDown,LeftTaken())
BOOL_HOOK(HRightPress,rightPress,mouseCapture.load())
BOOL_HOOK(HRightAOff,rightAOff,mouseCapture.load())
BOOL_HOOK(HRightDown,rightDown,mouseCapture.load())
BOOL_HOOK(HMiddleDown,middleDown,mouseCapture.load())
#undef BOOL_HOOK
bool HKeyDown(void* self,int key) { const bool value=keyDown(self,key); return keyboardCapture ? false:value; }
bool HKeyPress(void* self,int key) { const bool value=keyPress(self,key); return keyboardCapture ? false:value; }
bool HButtonDown(void* self,char* binding) { const bool value=buttonDown(self,binding); return mouseCapture || keyboardCapture ? false:value; }
bool HButtonPress(void* self,char* binding) { const bool value=buttonPress(self,binding); return mouseCapture || keyboardCapture ? false:value; }
float HAxis(void* self,char* binding,int* out,float a,float b) { const float value=axis(self,binding,out,a,b); return mouseCapture || keyboardCapture ? 0:value; }
float* HRelative(void* self,float* out) { auto* value=relative(self,out); if (mouseCapture) out[0]=out[1]=out[2]=0; return value; }
struct Import { const char* symbol; void* detour; void** original; };
Import imports[] = {
    {"?GetMouseLeftPress@C3D_INPUT@@QEAA_NXZ",reinterpret_cast<void*>(HLeftPress),reinterpret_cast<void**>(&leftPress)},
    {"?GetMouseLeftPressAOff@C3D_INPUT@@QEAA_NXZ",reinterpret_cast<void*>(HLeftAOff),reinterpret_cast<void**>(&leftAOff)},
    {"?GetMouseLeftDown@C3D_INPUT@@QEAA_NXZ",reinterpret_cast<void*>(HLeftDown),reinterpret_cast<void**>(&leftDown)},
    {"?GetMouseRightPress@C3D_INPUT@@QEAA_NXZ",reinterpret_cast<void*>(HRightPress),reinterpret_cast<void**>(&rightPress)},
    {"?GetMouseRightPressAOff@C3D_INPUT@@QEAA_NXZ",reinterpret_cast<void*>(HRightAOff),reinterpret_cast<void**>(&rightAOff)},
    {"?GetMouseRightDown@C3D_INPUT@@QEAA_NXZ",reinterpret_cast<void*>(HRightDown),reinterpret_cast<void**>(&rightDown)},
    {"?GetMouseMiddleDown@C3D_INPUT@@QEAA_NXZ",reinterpret_cast<void*>(HMiddleDown),reinterpret_cast<void**>(&middleDown)},
    {"?GetKeyDown@C3D_INPUT@@QEAA_NH@Z",reinterpret_cast<void*>(HKeyDown),reinterpret_cast<void**>(&keyDown)},
    {"?GetKeyPress@C3D_INPUT@@QEAA_NH@Z",reinterpret_cast<void*>(HKeyPress),reinterpret_cast<void**>(&keyPress)},
    {"?CommonInterfaceGetButtonDown@C3D_INPUT@@QEAA_NPEAD@Z",reinterpret_cast<void*>(HButtonDown),reinterpret_cast<void**>(&buttonDown)},
    {"?CommonInterfaceGetButtonPress@C3D_INPUT@@QEAA_NPEAD@Z",reinterpret_cast<void*>(HButtonPress),reinterpret_cast<void**>(&buttonPress)},
    {"?CommonInterfaceGetAxisValue@C3D_INPUT@@QEAAMPEADPEAHMM@Z",reinterpret_cast<void*>(HAxis),reinterpret_cast<void**>(&axis)},
    {"?GetMouseRelative@C3D_INPUT@@QEAA?AVC3DVECTOR3@@XZ",reinterpret_cast<void*>(HRelative),reinterpret_cast<void**>(&relative)},
};
}
void NotifyNativeTerrainEdit(float x,float z,float brushRadius) noexcept
{
    NotifyNativeTerrainEditImpl(x,z,brushRadius);
}
bool LeftTaken() { return mapPickCapture || mouseCapture || leftRelease; }
void ClearInputLatches()
{
    __try {
        if (LeftTaken()) { host->exeBase[0xA54E90]=0; host->exeBase[0xA54E91]=0; }
        if (keyboardCapture) host->exeBase[0xA54C91]=0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { }
}
Snapshot ReadSnapshot()
{
    const bool available=AvailabilitySignal();
    std::lock_guard<std::mutex> lock(dataMutex);
    Snapshot out=published;
    if (!available) { out.ready=false; out.cursorValid=false; out.cameraValid=false; }
    return out;
}
void Enqueue(Command command)
{
    std::lock_guard<std::mutex> lock(dataMutex);
    if (commands.size()<64) commands.push_back(command);
}
bool InstallRuntime()
{
    const auto engine=static_cast<HMODULE>(host->engineModule);
    heightQuery = reinterpret_cast<HeightFn>(GetProcAddress(static_cast<HMODULE>(host->engineModule),
        "?CollisionTerrainHeightOverPoint@C3D_TERRAIN@@QEAAMVC3DVECTOR3@@@Z"));
    raiseLower=reinterpret_cast<RaiseFn>(GetProcAddress(engine,"?EditRaiseLower@C3D_TERRAIN@@QEAAXVC3DVECTOR3@@MMMM_N@Z"));
    openHeightmap=reinterpret_cast<TextureFn>(GetProcAddress(engine,"?HeightmapTextureOpen@C3D_TERRAIN@@QEAAXXZ"));
    closeHeightmap=reinterpret_cast<TextureFn>(GetProcAddress(engine,"?HeightmapTextureClose@C3D_TERRAIN@@QEAAXXZ"));
    if (!heightQuery || host->exeSize < 0xA54E92 || host->exeSize < ConstructionAssignRva+sizeof(constructionAssignBytes)
        || host->exeSize < VehicleSlotRva+sizeof(vehicleSlotBytes)
        || !host->findIatSlot || !host->patchIat || !host->installInlineHook
        || std::memcmp(host->exeBase+LateRva,lateBytes,sizeof(lateBytes)) != 0
        || std::memcmp(host->exeBase+LoadRva,loadBytes,sizeof(loadBytes)) != 0
        || std::memcmp(host->exeBase+ConstructionAssignRva,constructionAssignBytes,sizeof(constructionAssignBytes)) != 0
        || std::memcmp(host->exeBase+VehicleSlotRva,vehicleSlotBytes,sizeof(vehicleSlotBytes)) != 0) {
        host->log("earthworks  native preflight failed; planning disabled"); return false;
    }
    for (const auto& item:imports) if (!host->findIatSlot(host->exeModule,"C3DDLL64.dll",item.symbol)) {
        host->log("earthworks  missing input import %s; planning disabled",item.symbol); return false;
    }
    for (const auto& item:imports) if (!host->patchIat(host->exeModule,"C3DDLL64.dll",item.symbol,item.detour,item.original,"Earthworks input capture")) return false;
    if (!host->installInlineHook(host->exeBase+LateRva,reinterpret_cast<void*>(HookedLate),reinterpret_cast<void**>(&originalLate),lateBytes,sizeof(lateBytes),"Earthworks read-only frame")) return false;
    if (!host->installInlineHook(host->exeBase+LoadRva,reinterpret_cast<void*>(HookedLoad),reinterpret_cast<void**>(&originalLoad),loadBytes,sizeof(loadBytes),"Earthworks world boundary")) return false;
    if (!host->installInlineHook(host->exeBase+ConstructionAssignRva,reinterpret_cast<void*>(HookedConstructionAssign),reinterpret_cast<void**>(&originalConstructionAssign),constructionAssignBytes,sizeof(constructionAssignBytes),"Earthworks office assignment filter")) return false;
    if (!host->installInlineHook(host->exeBase+VehicleSlotRva,reinterpret_cast<void*>(HookedVehicleSlot),reinterpret_cast<void**>(&originalVehicleSlot),vehicleSlotBytes,sizeof(vehicleSlotBytes),"Earthworks vehicle slot filter")) return false;
    host->log("earthworks  bindings installed: 13 input imports, frame, world-load, office assignment, and Earthworks vehicle slot filters");
    BindTerrainProbe();
    BindNativeTerrainCallProbe();
    host->log("earthworks  terrain executor bindings %s",raiseLower&&heightQuery&&openHeightmap&&closeHeightmap?"available":"unavailable; projects remain queued");
    return true;
}
}
