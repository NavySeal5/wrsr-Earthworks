// SPDX-License-Identifier: GPL-3.0-only
// Native binding addresses, input interception and camera layout adapted from
// RepublicMP (E3418), commit 630775724abd5e3346eb59f9c66e2873128d74a4.
#include <windows.h>
#include <unknwn.h>
#include "runtime.h"
#include "save_store.h"
#include "vehicle_filter.h"
#include "native_terrain_edit_hook.h"
#include "../core/terrain_grid.h"
#include "../core/productivity.h"
#include "../core/user_settings.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ew {
const TsmHost* host = nullptr;
std::atomic<bool> panelOpen{false}, mouseCapture{false}, keyboardCapture{false}, leftRelease{false}, mapPickCapture{false}, escapeCapture{false};
std::atomic<uint64_t> tableWheelCaptureUntilMs{0};
namespace {
// Native terrain sampling varies by a few millimeters between reads. Keep
// terrain review protection while allowing this observed readback jitter.
constexpr float TerrainReadbackToleranceMeters=0.02f;
std::mutex dataMutex;
Snapshot published, current;
std::deque<Command> commands;
History history;
DurableState durableState;
uint64_t selectedProjectId=0;
uint64_t editingSiteId=0;
unsigned selectedSitePointIndex=0;
uint32_t selectedSitePointsMask=1;
bool siteEditorOpen=false,creatingSite=false;
std::unordered_map<uint64_t,std::string> siteActionErrors;
SiteShapeMode shapeMode=SiteShapeMode::FreeDraw;
bool siteDraftLockToGrid=false;
bool savePending = false;
bool loadMessagePending = false;
bool facilityAssignmentWarningPending = false;
bool facilityIdentityCheckAfterLoad = false;
struct UnresolvedFacilityAssignment { uint64_t projectId=0; size_t index=0; uint32_t facilityId=0; };
std::vector<UnresolvedFacilityAssignment> unresolvedFacilityAssignments;
ULONGLONG lastFacilityRefresh = 0;
ULONGLONG lastProjectFacilityReadings = 0;
ULONGLONG lastWorldFacilityScan = 0;
ULONGLONG lastFacilityIdentityReconcile = 0;
size_t lastUnresolvedFacilityIdentityCount = static_cast<size_t>(-1);
ULONGLONG lastEditorVolumeEstimate = 0;
uint64_t editorVolumeSiteId = 0;
Volumes cachedEditorVolumes{};
bool cachedEditorVolumesValid = false;
Point SiteCenter(const Plan& plan)
{
    if(plan.pointCount>=3) {
        double twiceArea=0.0,weightedX=0.0,weightedZ=0.0;
        for(unsigned i=0;i<plan.pointCount;++i) {
            const auto& a=plan.points[i];
            const auto& b=plan.points[(i+1u)%plan.pointCount];
            const double cross=static_cast<double>(a.x)*b.z-static_cast<double>(b.x)*a.z;
            twiceArea+=cross;
            weightedX+=(static_cast<double>(a.x)+b.x)*cross;
            weightedZ+=(static_cast<double>(a.z)+b.z)*cross;
        }
        if(std::abs(twiceArea)>1.0e-6)
            return {static_cast<float>(weightedX/(3.0*twiceArea)),0,
                static_cast<float>(weightedZ/(3.0*twiceArea))};
    }
    return {(plan.a.x+plan.b.x)*0.5f,0,(plan.a.z+plan.b.z)*0.5f};
}
double EffectiveRateForPlan(const Plan& plan,const Snapshot& snapshot)
{
    const Point center=SiteCenter(plan);
    double total=0;
    for(unsigned i=0;i<snapshot.assignedFacilityCount;++i) {
        const auto& facility=snapshot.assignedFacilities[i];
        if(!facility.machineRateValid)continue;
        double contribution=1.0;
        if(DistanceBasedFacilityContribution()) {
            if(!facility.positionValid)return 0.0;
            const double distance=std::hypot(static_cast<double>(center.x)-facility.position.x,
                static_cast<double>(center.z)-facility.position.z);
            contribution=FacilityDistanceContribution(distance);
        }
        total+=static_cast<double>(facility.eligibleMachineRateUnits)*contribution;
    }
    return total;
}
uint32_t PointMask(unsigned index)
{
    return index<MaxSitePoints?static_cast<uint32_t>(1u<<index):0u;
}
uint32_t ValidPointMask(uint32_t mask,unsigned pointCount)
{
    const uint32_t valid=pointCount>=32?~uint32_t{0}:(pointCount?static_cast<uint32_t>((1u<<pointCount)-1u):0u);
    return mask&valid;
}
void SelectSingleSitePoint(unsigned index)
{
    selectedSitePointIndex=index;
    selectedSitePointsMask=PointMask(index);
}
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
bool projectSchedulerDirty = true;
ULONGLONG lastCompletionCheck = 0;
double SimulationHourClock(const Snapshot& snapshot)
{
    return (static_cast<double>(snapshot.year)*366.0+snapshot.day)*SimulationHoursPerInGameDay+snapshot.time;
}
using ConstructionAssignFn = void (*)(void*,void*,void*,float,bool,bool);
ConstructionAssignFn originalConstructionAssign = nullptr;
using VehicleSlotFn = unsigned char (*)(void*,void*,void*,uintptr_t,int*);
VehicleSlotFn originalVehicleSlot = nullptr;
// WRSR 1.1.1.13 (Steam build 24922914). The instruction prologues remain
// unchanged from 1.1.1.9, but the functions moved in the updated executable.
constexpr size_t LateRva = 0x30D120, LoadRva = 0x430F50, ConstructionAssignRva=0x161CB0, VehicleSlotRva=0x3E3E80;
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
void* TerrainObject();
bool ReadTerrainGrid(void* terrain,TerrainGrid* out);
bool EstimateCurrentTerrain(const Plan& plan,float blendWidth,Volumes* out)
{
    if(!out||!plan.complete||!ValidFootprint(plan))return false;
    TerrainGrid grid{};GridRect rect{};
    if(!ReadTerrainGrid(TerrainObject(),&grid)||!SnapInside(plan,grid,&rect)||rect.Count()>40000)return false;
    const double cellArea=static_cast<double>(grid.stepX)*grid.stepZ;
    const double completionTolerance=static_cast<double>(grid.heightScale)/60000.0*1.1;
    if(!std::isfinite(cellArea)||cellArea<=0||!std::isfinite(completionTolerance))return false;
    Volumes currentVolumes{};
    for(int z=rect.z0;z<=rect.z1;++z)for(int x=rect.x0;x<=rect.x1;++x) {
        const Point point=grid.At(x,z);
        if(!plan.Contains(point.x,point.z))continue;
        float target=0,ground=0;
        if(!plan.BlendedTargetAt(point.x,point.z,blendWidth,&target)||!ReadHeight(point.x,point.z,&ground))return false;
        const double delta=static_cast<double>(target)-ground;
        // Report actionable remaining work on the same native terrain vertices
        // and within the same completion tolerance used by the executor.
        if(std::abs(delta)<=completionTolerance)continue;
        if(delta>0)currentVolumes.fill+=delta*cellArea;
        else currentVolumes.cut-=delta*cellArea;
    }
    if(!std::isfinite(currentVolumes.cut)||!std::isfinite(currentVolumes.fill))return false;
    *out=currentVolumes;
    return true;
}
void UpdateEditorLiveVolumes()
{
    if(!siteEditorOpen||creatingSite||!editingSiteId||!current.editorPlan.complete) {
        current.editorVolumesValid=false;
        return;
    }
    if(editorVolumeSiteId!=editingSiteId) {
        editorVolumeSiteId=editingSiteId;
        cachedEditorVolumesValid=false;
        lastEditorVolumeEstimate=0;
    }
    const auto now=GetTickCount64();
    if(!cachedEditorVolumesValid||now-lastEditorVolumeEstimate>=500) {
        Volumes measured{};
        if(EstimateCurrentTerrain(current.editorPlan,current.terrainCellSize,&measured)) {
            cachedEditorVolumes=measured;
            cachedEditorVolumesValid=true;
            lastEditorVolumeEstimate=now;
        }
    }
    current.editorVolumes=cachedEditorVolumes;
    current.editorVolumesValid=cachedEditorVolumesValid;
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
void UpdateTerrainGridSnapshot()
{
    TerrainGrid grid{};
    current.terrainGridValid=current.ready&&ReadTerrainGrid(TerrainObject(),&grid);
    if(current.terrainGridValid) {
        current.terrainGridOriginX=grid.originX;current.terrainGridOriginZ=grid.originZ;
        current.terrainGridStepX=grid.stepX;current.terrainGridStepZ=grid.stepZ;
        current.terrainCellSize=std::min(grid.stepX,grid.stepZ);
    } else {
        current.terrainGridOriginX=current.terrainGridOriginZ=0;
        current.terrainGridStepX=current.terrainGridStepZ=current.terrainCellSize=0;
    }
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
    GridRect localRect{};
    GridRect* checkedRect=rectOut?rectOut:&localRect;
    if(!plan.complete || !SnapInside(plan,grid,checkedRect)) {
        const float x0=(std::min(plan.a.x,plan.b.x)-grid.originX)/grid.stepX;
        const float x1=(std::max(plan.a.x,plan.b.x)-grid.originX)/grid.stepX;
        const float z0=(std::min(plan.a.z,plan.b.z)-grid.originZ)/grid.stepZ;
        const float z1=(std::max(plan.a.z,plan.b.z)-grid.originZ)/grid.stepZ;
        if(!std::isfinite(x0)||!std::isfinite(x1)||!std::isfinite(z0)||!std::isfinite(z1)
            || x0<3 || z0<3 || x1>static_cast<float>(grid.width-4) || z1>static_cast<float>(grid.depth-4))
            return ProjectAreaError::OutsideHeightmap;
        return ProjectAreaError::TooSmall;
    }
    const int count=checkedRect->Count();
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
bool SiteEditorMatches(uint64_t siteId)
{
    return siteEditorOpen&&((creatingSite&&siteId==0)||(!creatingSite&&editingSiteId==siteId));
}
void ClearSiteActionError(uint64_t siteId)
{
    siteActionErrors.erase(siteId);
    if(SiteEditorMatches(siteId))current.siteActionError[0]=0;
}
void SiteActionError(uint64_t siteId,const char* text)
{
    Message(text);
    siteActionErrors[siteId]=text;
    if(SiteEditorMatches(siteId))strncpy_s(current.siteActionError,text,_TRUNCATE);
}
void SetDefaultDraftSiteName()
{
    char name[MaxSavedNameBytes+1]{};
    sprintf_s(name,"Site-%u",durableState.nextSiteNumber);
    durableState.draftName=name;
}
bool ProjectReservesFootprint(ProjectStatus status)
{
    return status==ProjectStatus::Queued||status==ProjectStatus::Running||status==ProjectStatus::Waiting||status==ProjectStatus::Paused;
}
SavedProject* FindNamedProject(uint64_t id)
{
    for(auto& project:durableState.projects)if(project.id==id)return &project;
    return nullptr;
}
void ResetProjectWorkBacklog(uint64_t projectId)
{
    // workRemainderM3 is terrain work that the executor has not applied yet.
    // It can accumulate when terrain edits are throttled. If the facility
    // roster changes, discard that stale allowance so the next slice uses
    // only the machines currently assigned. Applied terrain and progress are
    // retained; the remainder will accumulate again at the new rate.
    double discarded=0;
    for(auto& site:durableState.earthworks)if(site.projectId==projectId) {
        discarded+=std::max(0.0,static_cast<double>(site.workRemainderM3));
        site.workRemainderM3=0;
    }
    if(discarded>0)host->log("earthworks  project=%llu discarded stale terrain-work backlog=%.3f m3 after facility roster change",
        static_cast<unsigned long long>(projectId),discarded);
}
SavedEarthwork* FindProject(uint64_t id)
{
    for(auto& project:durableState.earthworks)if(project.id==id)return &project;
    return nullptr;
}
bool MoveCompletedSiteToProjectEnd(uint64_t siteId)
{
    auto& sites=durableState.earthworks;
    size_t sourceIndex=sites.size();
    for(size_t i=0;i<sites.size();++i)if(sites[i].id==siteId) {sourceIndex=i;break;}
    if(sourceIndex==sites.size()||sites[sourceIndex].status!=ProjectStatus::Completed)return false;

    const auto projectId=sites[sourceIndex].projectId;
    size_t insertionIndex=sourceIndex+1;
    for(size_t i=sourceIndex+1;i<sites.size();++i)
        if(sites[i].projectId==projectId)insertionIndex=i+1;
    if(insertionIndex==sourceIndex+1)return false;

    SavedEarthwork completed=std::move(sites[sourceIndex]);
    sites.erase(sites.begin()+static_cast<std::ptrdiff_t>(sourceIndex));
    --insertionIndex;
    sites.insert(sites.begin()+static_cast<std::ptrdiff_t>(insertionIndex),std::move(completed));
    return true;
}
void UpdateSiteDraftValidation()
{
    current.siteDraftError[0]=0;
    if(!creatingSite)return;
    auto fail=[](const char* reason){strncpy_s(current.siteDraftError,reason,_TRUNCATE);};
    if(!FindNamedProject(durableState.selectedProjectId)) {
        fail("Create or select a Project before saving this Site.");return;
    }
    if(!current.plan.complete) {
        fail(current.plan.first?"Add at least three Site points, then finish the shape.":"Select at least three points before finishing the Site.");return;
    }
    if(durableState.earthworks.size()>=MaxSavedEarthworks) {
        fail("The save has reached its 128 Site limit. Delete Sites with the X button before adding another.");return;
    }
    if(!ValidFootprint(current.plan)) {
        fail("Site shape is invalid. Check point spacing and make sure its edges do not cross.");return;
    }
    for(unsigned i=0;i<current.plan.pointCount;++i)if(!std::isfinite(current.plan.points[i].y)||std::abs(current.plan.points[i].y)>=100000.0f) {
        fail("Enter a finite target height within the supported range.");return;
    }
    const double maxGrade=MaximumSurfaceGrade(current.plan);
    if(current.plan.safeAngles==SafeAnglesMode::Roads&&maxGrade>MaxRoadGrade+1e-6) {
        fail("Safe angles is set to Roads: this Site exceeds 16% grade (about 9.09 degrees).");return;
    }
    if(current.plan.safeAngles==SafeAnglesMode::Vanilla&&maxGrade>VanillaTerrainReferenceGrade+1e-6) {
        fail("Safe angles is set to Vanilla: this Site exceeds 37 degrees (about 75.4% grade).");return;
    }
    const auto areaError=CheckProjectArea(current.plan,nullptr,nullptr);
    if(areaError!=ProjectAreaError::None) {
        fail(ProjectAreaMessage(areaError));return;
    }
    for(const auto& saved:durableState.earthworks)if(saved.projectId!=durableState.selectedProjectId
        &&ProjectReservesFootprint(saved.status)&&FootprintsOverlap(current.plan,saved.plan)) {
        char message[192]{};
        sprintf_s(message,"This Site overlaps %s in another Project. Move it or cancel that unfinished Site first.",saved.name.c_str());
        fail(message);return;
    }
}
void RefreshProjectStatus();
uint32_t AssignedFacility(const SavedEarthwork& earthwork)
{
    const auto* project=FindNamedProject(earthwork.projectId);
    return project&&!project->facilityIds.empty()?project->facilityIds.front():0;
}
bool SameFacilityAnchor(const SavedProject::FacilityAnchor& anchor,const BuildingReading& building)
{
    if(!anchor.valid||anchor.assetName!=building.assetName)return false;
    const double dx=static_cast<double>(anchor.position.x)-building.position.x;
    const double dz=static_cast<double>(anchor.position.z)-building.position.z;
    return dx*dx+dz*dz<=4.0; // two metres allows small float differences, not a nearby site.
}
void SaveFacilityAnchor(SavedProject::FacilityAnchor& anchor,const BuildingReading& building)
{
    anchor.assetName=building.assetName;
    anchor.position=building.position;
    anchor.valid=building.assetName[0]!=0&&std::isfinite(building.position.x)&&std::isfinite(building.position.y)&&std::isfinite(building.position.z);
}
void ReconcileSavedFacilities()
{
    std::array<BuildingReading,WorldEarthworksFacilities> buildings{};
    unsigned count=0;bool truncated=false;
    if(!ReadWorldEarthworksFacilities(&buildings,&count,&truncated)||truncated)return;
    std::vector<unsigned> usedIds;
    std::vector<std::pair<SavedProject*,size_t>> unresolvedLegacy;
    std::vector<UnresolvedFacilityAssignment> unresolvedAnchored;
    bool changed=false;
    for(auto& project:durableState.projects) {
        if(project.facilityAnchors.size()!=project.facilityIds.size())project.facilityAnchors.resize(project.facilityIds.size());
        for(size_t i=0;i<project.facilityIds.size();++i) {
            auto& anchor=project.facilityAnchors[i];
            unsigned match=static_cast<unsigned>(count);unsigned matches=0;
            if(anchor.valid) {
                for(unsigned j=0;j<count;++j)if(SameFacilityAnchor(anchor,buildings[j])
                    &&std::find(usedIds.begin(),usedIds.end(),buildings[j].id)==usedIds.end()) {match=j;++matches;}
                if(matches==1) {
                    const auto oldId=project.facilityIds[i];project.facilityIds[i]=buildings[match].id;
                    usedIds.push_back(buildings[match].id);
                    if(oldId!=project.facilityIds[i]) {
                        changed=true;
                        host->log("earthworks  restored facility link project=%llu oldId=%u newId=%u asset=%s position=(%.2f,%.2f)",
                            static_cast<unsigned long long>(project.id),oldId,project.facilityIds[i],buildings[match].assetName,
                            buildings[match].position.x,buildings[match].position.z);
                    }
                } else unresolvedAnchored.push_back({project.id,i,project.facilityIds[i]});
                continue;
            }
            for(unsigned j=0;j<count;++j)if(buildings[j].id==project.facilityIds[i]
                &&std::find(usedIds.begin(),usedIds.end(),buildings[j].id)==usedIds.end()) {match=j;++matches;}
            if(matches==1) {
                SaveFacilityAnchor(anchor,buildings[match]);
                usedIds.push_back(buildings[match].id);changed|=anchor.valid;
            } else unresolvedLegacy.emplace_back(&project,i);
        }
    }
    // Old companion files stored only native IDs. Recover those links only
    // when one saved assignment and one unclaimed Earthworks building remain.
    std::vector<unsigned> unclaimed;
    for(unsigned i=0;i<count;++i)if(std::find(usedIds.begin(),usedIds.end(),buildings[i].id)==usedIds.end())unclaimed.push_back(i);
    if(unresolvedLegacy.size()==1&&unresolvedAnchored.empty()&&unclaimed.size()==1) {
        auto [project,index]=unresolvedLegacy.front();const auto& building=buildings[unclaimed.front()];
        project->facilityIds[index]=building.id;SaveFacilityAnchor(project->facilityAnchors[index],building);
        usedIds.push_back(building.id);changed=true;unresolvedLegacy.clear();
        host->log("earthworks  recovered legacy facility link project=%llu newId=%u asset=%s position=(%.2f,%.2f)",
            static_cast<unsigned long long>(project->id),building.id,building.assetName,building.position.x,building.position.z);
    }
    if(changed)RefreshProjectStatus();
    unresolvedFacilityAssignments=std::move(unresolvedAnchored);
    for(const auto& unresolved:unresolvedLegacy)
        unresolvedFacilityAssignments.push_back({unresolved.first->id,unresolved.second,unresolved.first->facilityIds[unresolved.second]});
    const size_t unresolvedCount=unresolvedFacilityAssignments.size();
    if(facilityIdentityCheckAfterLoad) {
        facilityIdentityCheckAfterLoad=false;
        facilityAssignmentWarningPending=unresolvedCount>0;
        if(facilityAssignmentWarningPending)
            host->log("earthworks  post-load warning: %zu facility assignment(s) could not be resolved; user cleanup is available",unresolvedCount);
    }
    if(unresolvedCount!=lastUnresolvedFacilityIdentityCount) {
        if(unresolvedCount)host->log("earthworks  facility links unresolved=%zu unclaimedBuildings=%zu; no ambiguous link was changed",unresolvedCount,unclaimed.size());
        lastUnresolvedFacilityIdentityCount=unresolvedCount;
    }
}
bool FindAssignedFacility(const SavedEarthwork& earthwork,uint32_t* id,BuildingReading* reading)
{
    const auto* project=FindNamedProject(earthwork.projectId);
    if(!project)return false;
    for(const auto facilityId:project->facilityIds) {
        BuildingReading candidate{};
        if(FindEarthworksFacility(facilityId,&candidate)) {
            if(id)*id=facilityId;
            if(reading)*reading=candidate;
            return true;
        }
    }
    return false;
}
const SavedProject* FacilityOwner(uint32_t facilityId)
{
    for(const auto& project:durableState.projects)
        if(std::find(project.facilityIds.begin(),project.facilityIds.end(),facilityId)!=project.facilityIds.end())return &project;
    return nullptr;
}
void Changed()
{
    durableState.draft = current.plan;
    current.volumes = Estimate(current.plan,current.terrainCellSize);
    ++current.revision;
    RefreshProjectStatus();
    host->log("earthworks  draft-only revision=%llu world=%llu complete=%d target=%.3f area=%.2f cut=%.2f fill=%.2f",
        current.revision, current.generation, current.plan.complete ? 1 : 0, current.plan.target,
        static_cast<double>(current.plan.Width())*current.plan.Depth(), current.volumes.cut, current.volumes.fill);
}
void DiscardSiteDraft()
{
    ClearSiteActionError(0);
    current.plan=Plan{};
    history.Reset();
    current.mode=PickMode::None;
    creatingSite=false;
    siteDraftLockToGrid=false;current.lockToGrid=false;
    editingSiteId=0;
    siteEditorOpen=false;
    SetDefaultDraftSiteName();
    Changed();
}
void ResetWorld()
{
    const auto generation = current.generation+1;
    current = Snapshot{}; current.generation = generation;
    siteActionErrors.clear();
    SelectSingleSitePoint(0);
    durableState = DurableState{}; selectedProjectId=0;editingSiteId=0;siteEditorOpen=false;creatingSite=false;shapeMode=SiteShapeMode::FreeDraw;siteDraftLockToGrid=false; savePending = false; loadMessagePending = false;facilityAssignmentWarningPending=false;facilityIdentityCheckAfterLoad=false;unresolvedFacilityAssignments.clear();lastSimulationHour=-1;projectSchedulerDirty=true;
    lastEditorVolumeEstimate=0;editorVolumeSiteId=0;cachedEditorVolumes={};cachedEditorVolumesValid=false;
    lastWorldFacilityScan=0;lastFacilityIdentityReconcile=0;lastUnresolvedFacilityIdentityCount=static_cast<size_t>(-1);
    durableState.projects.push_back({1,"Project 1",{}});
    durableState.selectedProjectId=1;
    history.Reset(); panelOpen = false; mouseCapture = false; keyboardCapture = false; mapPickCapture = false; escapeCapture = false;
    ResetWorkforce();
    RefreshProjectStatus();
    std::lock_guard<std::mutex> lock(dataMutex);
    commands.clear(); published = current;
    host->log("earthworks  world boundary; discarded session preview, generation=%llu", generation);
}
void RefreshProjectStatus()
{
    TerrainGrid terrainGrid{};
    UpdateTerrainGridSnapshot();
    current.lockToGrid=siteDraftLockToGrid;
    current.facilityAssignmentWarning=facilityAssignmentWarningPending;
    current.volumes=Estimate(current.plan,current.terrainCellSize);
    const auto now=GetTickCount64();
    const bool refreshFacilityReadings=now-lastProjectFacilityReadings>=500;
    const auto previousFacilities=current.assignedFacilities;
    const auto previousFacilityCount=current.assignedFacilityCount;
    const auto* selectedContainer=FindNamedProject(durableState.selectedProjectId);
    current.selectedNamedProjectId=durableState.selectedProjectId;
    current.projectContainerName[0]=0;
    if(selectedContainer)strncpy_s(current.projectContainerName,selectedContainer->name.c_str(),_TRUNCATE);
    current.namedProjectCount=std::min(durableState.projects.size(),MaxSavedProjects);
    for(size_t i=0;i<current.namedProjectCount;++i) {
        const auto& source=durableState.projects[i];auto& target=current.namedProjects[i];
        target.id=source.id;strncpy_s(target.name,source.name.c_str(),_TRUNCATE);target.facilityCount=static_cast<unsigned>(source.facilityIds.size());
    }
    current.assignedFacilityIds.fill(0);
    current.assignedFacilities.fill(ProjectFacilityBrief{});
    current.assignedFacilityCount=selectedContainer?static_cast<unsigned>(std::min(selectedContainer->facilityIds.size(),MaxProjectFacilities)):0;
    current.assignedEligibleMachines=0;
    current.assignedMachineRateUnits=0;
    current.assignedMachineReadingsValid=selectedContainer!=nullptr;
    for(unsigned i=0;i<current.assignedFacilityCount;++i) {
        const auto id=selectedContainer->facilityIds[i];
        current.assignedFacilityIds[i]=id;
        auto& target=current.assignedFacilities[i];target.id=id;
        bool cached=false;
        for(unsigned j=0;j<previousFacilityCount;++j)if(previousFacilities[j].id==id) {
            target.eligibleMachines=previousFacilities[j].eligibleMachines;
            target.eligibleMachineRateUnits=previousFacilities[j].eligibleMachineRateUnits;
            target.position=previousFacilities[j].position;
            target.positionValid=previousFacilities[j].positionValid;
            target.machineCountValid=previousFacilities[j].machineCountValid;
            target.machineRateValid=previousFacilities[j].machineRateValid;
            cached=true;break;
        }
        if(!cached||refreshFacilityReadings) {
            WorkforceView facility{};
            target.eligibleMachines=0;target.eligibleMachineRateUnits=0;
            target.machineCountValid=false;target.machineRateValid=false;
            target.positionValid=false;
            if(ReadEarthworksFacilityWorkforce(id,&facility) && facility.buildingValid && facility.selectedIsEarthworksFacility
                && facility.parkedValid && facility.vehiclesValid && facility.unreadableVehicles==0) {
                target.eligibleMachines=facility.eligibleMachineCandidates;
                target.machineCountValid=true;
                target.eligibleMachineRateUnits=facility.eligibleMachineRateUnits;
                target.machineRateValid=facility.machineSkillReadingsValid;
                target.position=facility.building.position;
                target.positionValid=true;
            }
        }
        if(target.machineCountValid)current.assignedEligibleMachines=std::min<unsigned>(
            MaxSavedEarthworks,current.assignedEligibleMachines+target.eligibleMachines);
        else current.assignedMachineReadingsValid=false;
        if(target.machineRateValid) {
            current.assignedMachineRateUnits+=target.eligibleMachineRateUnits;
        } else current.assignedMachineReadingsValid=false;
    }
    if(refreshFacilityReadings||current.assignedFacilityCount!=previousFacilityCount)lastProjectFacilityReadings=now;
    if(!current.ready||now-lastWorldFacilityScan>=1000) {
        current.facilityCandidateCount=0;current.facilityListTruncated=false;
        std::array<BuildingReading,WorldEarthworksFacilities> buildings{};unsigned count=0;bool truncated=false;
        if(current.ready&&ReadWorldEarthworksFacilities(&buildings,&count,&truncated)) {
            current.facilityCandidateCount=count;current.facilityListTruncated=truncated;
            for(unsigned i=0;i<count;++i) {
                auto& target=current.facilityCandidates[i];target=ProjectFacilityBrief{};
                target.id=buildings[i].id;strncpy_s(target.assetName,buildings[i].assetName,_TRUNCATE);
                strncpy_s(target.displayName,buildings[i].displayName,_TRUNCATE);
                target.position=buildings[i].position;target.positionValid=true;
            }
        }
        lastWorldFacilityScan=now;
    }
    for(unsigned i=0;i<current.facilityCandidateCount;++i) {
        auto& target=current.facilityCandidates[i];
        target.assignedProjectId=0;target.assignedProjectName[0]=0;
        if(const auto* owner=FacilityOwner(target.id)) {
            target.assignedProjectId=owner->id;strncpy_s(target.assignedProjectName,owner->name.c_str(),_TRUNCATE);
        }
    }
    current.projectCount=0;
    for(const auto& source:durableState.earthworks)if(source.projectId==durableState.selectedProjectId&&current.projectCount<MaxVisibleProjects) {
        auto& target=current.projects[current.projectCount++];
        target.id=source.id;target.status=source.status;target.facilityId=AssignedFacility(source);
        strncpy_s(target.name,source.name.c_str(),_TRUNCATE);
        target.allocatedWorkers=source.status==ProjectStatus::Running?source.allocatedWorkers:0;
        target.allocatedMachines=source.status==ProjectStatus::Running?source.allocatedMachines:0;
        target.allocatedRateUnits=source.status==ProjectStatus::Running?source.allocatedRateUnits:0;
        target.progress=source.progress;target.needsReview=source.needsReview;target.visible=source.visible;
        target.outline.points=source.plan.points;target.outline.pointCount=source.plan.pointCount;
        target.a=source.plan.a;target.b=source.plan.b;target.target=source.plan.target;
    }
    current.projectTotal=current.projectCount;
    current.projectQueued=false;current.projectId=0;
    current.projectFacilityId=0;
    current.assignedFacilityFound=false;
    current.projectStatus=ProjectStatus::Draft;
    current.projectNeedsReview=false;
    current.projectProgress=0;
    current.editorAllocatedMachines=0;
    current.editorAllocatedRateUnits=0;
    current.selectedSiteName[0]=0;
    current.siteEditorOpen=siteEditorOpen;current.creatingSite=creatingSite;current.editingSiteId=editingSiteId;current.shapeMode=shapeMode;
    current.siteActionError[0]=0;
    const auto actionError=siteActionErrors.find(creatingSite?0:editingSiteId);
    if(siteEditorOpen&&actionError!=siteActionErrors.end())
        strncpy_s(current.siteActionError,actionError->second.c_str(),_TRUNCATE);
    current.selectedSitePoint=selectedSitePointIndex;
    current.selectedSitePointsMask=selectedSitePointsMask;
    strncpy_s(current.siteDraftName,durableState.draftName.c_str(),_TRUNCATE);
    current.editorPlan=Plan{};
    current.projectFacilityDistance=0;
    current.queuedPlan=Plan{};
    auto selected=durableState.earthworks.end();
    if(selectedProjectId)selected=std::find_if(durableState.earthworks.begin(),durableState.earthworks.end(),[](const SavedEarthwork& p){return p.id==selectedProjectId;});
    if(selected==durableState.earthworks.end()) {
        selected=std::find_if(durableState.earthworks.begin(),durableState.earthworks.end(),[&](const SavedEarthwork& p){return p.projectId==durableState.selectedProjectId&&(p.status==ProjectStatus::Queued||p.status==ProjectStatus::Running||p.status==ProjectStatus::Waiting||p.status==ProjectStatus::Paused);});
        if(selected==durableState.earthworks.end())selected=std::find_if(durableState.earthworks.begin(),durableState.earthworks.end(),[&](const SavedEarthwork& p){return p.projectId==durableState.selectedProjectId;});
    }
    if(selected!=durableState.earthworks.end()&&selected->projectId==durableState.selectedProjectId) {
        selectedProjectId=selected->id;current.projectQueued=true;current.projectId=selected->id;current.queuedPlan=selected->plan;
        strncpy_s(current.selectedSiteName,selected->name.c_str(),_TRUNCATE);
        current.projectStatus=selected->status;current.projectNeedsReview=selected->needsReview;current.projectProgress=selected->progress;
        current.editorAllocatedMachines=selected->status==ProjectStatus::Running?selected->allocatedMachines:0;
        current.editorAllocatedRateUnits=selected->status==ProjectStatus::Running?selected->allocatedRateUnits:0;
        current.projectFacilityId=AssignedFacility(*selected);
        if(current.projectFacilityId) {
            BuildingReading facility{};
            if(FindEarthworksFacility(current.projectFacilityId,&facility)) {
                current.assignedFacilityFound=true;
                const Point center=SiteCenter(selected->plan);
                current.projectFacilityDistance=std::hypot(center.x-facility.position.x,center.z-facility.position.z);
            }
        }
    }
    if(siteEditorOpen) {
        if(creatingSite)current.editorPlan=current.plan;
        else if(auto* editing=FindProject(editingSiteId);editing&&editing->projectId==durableState.selectedProjectId) {
            current.editorPlan=editing->plan;
            current.editingSiteId=editing->id;
            strncpy_s(current.selectedSiteName,editing->name.c_str(),_TRUNCATE);
        } else {
            siteEditorOpen=false;editingSiteId=0;current.siteEditorOpen=false;current.editingSiteId=0;
        }
        selectedSitePointIndex=current.editorPlan.pointCount?std::min<unsigned>(selectedSitePointIndex,current.editorPlan.pointCount-1):0;
        selectedSitePointsMask=ValidPointMask(selectedSitePointsMask,current.editorPlan.pointCount);
        current.selectedSitePoint=selectedSitePointIndex;current.selectedSitePointsMask=selectedSitePointsMask;
    }
    if(siteEditorOpen&&current.editorPlan.complete)
        current.assignedMachineRateUnits=EffectiveRateForPlan(current.editorPlan,current);
    else if(current.queuedPlan.complete)
        current.assignedMachineRateUnits=EffectiveRateForPlan(current.queuedPlan,current);
    UpdateEditorLiveVolumes();
}
bool SamplePlan(Plan* plan)
{
    for (int z=0; z<GridSide; ++z) for (int x=0; x<GridSide; ++x) {
        const auto p = plan->Vertex(x,z,true);
        if (!ReadHeight(p.x,p.z,&plan->ground[static_cast<size_t>(z*GridSide+x)])) return false;
    }
    return true;
}
bool FinishCurrentSiteShape()
{
    if(!creatingSite||current.plan.complete){SiteActionError(0,"There is no unfinished Site shape to finish.");return false;}
    current.plan.RefreshBounds();
    if(!ValidFootprint(current.plan)){SiteActionError(0,"Site needs three or more spaced, non-crossing points with a valid area.");return false;}
    if(!SamplePlan(&current.plan)){SiteActionError(0,"Could not sample the Site area. Adjust the shape or keep it inside the terrain.");return false;}
    current.plan.complete=true;current.mode=PickMode::None;history.Commit(current.plan);ClearSiteActionError(0);Changed();Message("Site shape ready. Set the heights of its points, then save the Site.");return true;
}
bool MeasureTerrainWork(const Plan& plan,const TerrainGrid& grid,const GridRect& rect,double* total)
{
    if(!total)return false;
    double volume=0;
    const double cellArea=static_cast<double>(grid.stepX)*grid.stepZ;
    if(!std::isfinite(cellArea)||cellArea<=0)return false;
    for(int z=rect.z0;z<=rect.z1;++z)for(int x=rect.x0;x<=rect.x1;++x) {
        const auto point=grid.At(x,z);float target=0,currentHeight=0;
        if(!plan.Contains(point.x,point.z)||!plan.BlendedTargetAt(point.x,point.z,std::min(grid.stepX,grid.stepZ),&target))continue;
        if(!ReadHeight(point.x,point.z,&currentHeight))return false;
        volume+=std::abs(static_cast<double>(target)-currentHeight)*cellArea;
        if(!std::isfinite(volume)||volume>1.0e12)return false;
    }
    *total=volume;
    return true;
}
void Process(const Command& command)
{
    if (command.generation != current.generation || !current.ready) return;
    switch(command.action) {
    case Action::AddProjectFacility:
    case Action::RemoveProjectFacility:
    case Action::MoveProjectUp:
    case Action::MoveProjectDown:
    case Action::AssignProject:
    case Action::ClearProjectAssignment:
    case Action::StartProject:
    case Action::PauseProject:
    case Action::ResumeProject:
    case Action::ReviewResumeProject:
    case Action::SettingsChanged:
        projectSchedulerDirty=true;
        break;
    default:
        break;
    }
    switch(command.action) {
    case Action::SettingsChanged:
        RefreshProjectStatus();
        ++current.revision;
        break;
    case Action::NewSite:
        if(!FindNamedProject(durableState.selectedProjectId)) {Message("Create or select a Project before creating a Site.");break;}
        if(creatingSite) {siteEditorOpen=true;current.mode=current.plan.complete?PickMode::None:PickMode::DrawSite;RefreshProjectStatus();Message(current.plan.complete?"New Site draft reopened.":"Click vertices to draw the Site boundary, then finish the shape.");break;}
        if(current.plan.first) {creatingSite=true;siteEditorOpen=true;current.mode=current.plan.complete?PickMode::None:PickMode::DrawSite;RefreshProjectStatus();Message(current.plan.complete?"Saved Site draft reopened.":"Continue adding Site boundary points.");break;}
        ClearSiteActionError(0);
        history.Reset(); current.plan = Plan{};shapeMode=SiteShapeMode::FreeDraw;siteDraftLockToGrid=false;current.lockToGrid=false; Changed();
        SetDefaultDraftSiteName();
        editingSiteId=0;creatingSite=true;siteEditorOpen=true;
        SelectSingleSitePoint(0);current.mode = PickMode::DrawSite;
        RefreshProjectStatus();
        Message("Click at least three points around the Site boundary, then finish the shape.");
        break;
    case Action::Select:
        if(!FindNamedProject(durableState.selectedProjectId)) {Message("Create or select a Project before creating a Site.");break;}
        ClearSiteActionError(0);history.Reset();current.plan=Plan{};shapeMode=SiteShapeMode::FreeDraw;siteDraftLockToGrid=false;Changed();SetDefaultDraftSiteName();editingSiteId=0;creatingSite=true;siteEditorOpen=true;SelectSingleSitePoint(0);current.mode=PickMode::DrawSite;RefreshProjectStatus();
        Message("Click at least three points around the Site boundary, then finish the shape.");break;
    case Action::SetShapeMode:
        if(!creatingSite||current.plan.complete||current.plan.pointCount>1||command.value<0.0f||command.value>2.0f)break;
        shapeMode=static_cast<SiteShapeMode>(static_cast<unsigned>(command.value));
        current.shapeMode=shapeMode;
        current.mode=PickMode::DrawSite;
        Message(shapeMode==SiteShapeMode::FreeDraw?"Free draw: click boundary points, then finish the shape.":shapeMode==SiteShapeMode::Square?"Square: click one corner, move to size it, then click to set it.":"Rectangle: click one corner, move to size it, then click the opposite corner.");
        break;
    case Action::SetGridSnap:
        if(!creatingSite||current.plan.pointCount||current.plan.complete)break;
        siteDraftLockToGrid=command.value!=0.0f;current.lockToGrid=siteDraftLockToGrid;
        Message(siteDraftLockToGrid?"Lock to Grid enabled. Click anywhere; new Site points snap to the nearest terrain grid vertex.":"Lock to Grid disabled.");
        break;
    case Action::SetSiteSafeAngles: {
        Plan* plan=creatingSite?&current.plan:nullptr;
        SavedEarthwork* site=creatingSite?nullptr:FindProject(editingSiteId);
        if(!plan&&site&&site->status!=ProjectStatus::Completed)plan=&site->plan;
        if(!plan)break;
        const auto mode=static_cast<unsigned>(command.value);
        if(mode>static_cast<unsigned>(SafeAnglesMode::Vanilla))break;
        plan->safeAngles=static_cast<SafeAnglesMode>(mode);
        ClearSiteActionError(creatingSite?0:editingSiteId);
        if(creatingSite)Changed();else ++current.revision;
        break;
    }
    case Action::CloseSiteEditor:
        if(creatingSite) {DiscardSiteDraft();Message("New Site draft discarded because its editor was closed.");}
        else {siteEditorOpen=false;RefreshProjectStatus();}
        break;
    case Action::CancelSiteDraft:
        if(creatingSite) {DiscardSiteDraft();Message("New Site draft discarded.");}
        break;
    case Action::Sample:
        current.mode = PickMode::Sample;
        Message("Click the ground to sample its height.");
        break;
    case Action::SampleSiteTarget:
        if(creatingSite&&current.plan.pointCount)current.mode=PickMode::SampleSitePointTarget;
        else if(auto* site=FindProject(editingSiteId);site&&site->projectId==durableState.selectedProjectId
            &&site->status!=ProjectStatus::Completed
            ) {
            const bool pausedForEdit=site->status==ProjectStatus::Running||site->status==ProjectStatus::Waiting;
            if(pausedForEdit){site->status=ProjectStatus::Paused;projectSchedulerDirty=true;}
            if(site->plan.pointCount)current.mode=PickMode::SampleSitePointTarget;
            if(pausedForEdit)RefreshProjectStatus();
        }
        else {Message("Select an editable Site before sampling its target height.");break;}
        Message("Click the ground to sample a target height. An active Site is paused while you edit it.");break;
    case Action::CancelPick:
        if(current.mode==PickMode::Sample) Message("Height sampling cancelled.");
        else if(current.mode==PickMode::SampleSitePointTarget) Message("Point height sampling cancelled.");
        else if(current.mode==PickMode::DrawSite) Message("Site drawing paused. The current points are kept.");
        else if(current.mode==PickMode::InspectBuilding) Message("Facility picking cancelled.");
        current.mode=PickMode::None;
        break;
    case Action::FinishSite: {
        FinishCurrentSiteShape();break;
    }
    case Action::Pick: {
        if(current.mode==PickMode::DrawSite&&host&&host->log)
            host->log("earthworks draw-click processed points=%u creating=%d cursorValid=%d cursorX=%.2f cursorZ=%.2f snap=%d grid=%d",
                current.plan.pointCount,static_cast<int>(creatingSite),static_cast<int>(current.cursorValid),current.cursor.x,current.cursor.z,
                static_cast<int>(siteDraftLockToGrid),static_cast<int>(current.terrainGridValid));
        if (!current.cursorValid) { if(siteEditorOpen)SiteActionError(creatingSite?0:editingSiteId,"No ground under the cursor. Try another point.");else Message("No ground under the cursor. Try another point."); break; }
        if(current.mode==PickMode::InspectBuilding) {
            FindWorkforceBuildings(current.cursor);current.mode=PickMode::None;RefreshProjectStatus();
            Message("Select your Earthworks facility in the Projects tab, then assign it to the selected project.");break;
        }
        Point p = current.cursor;
        if(current.mode==PickMode::DrawSite&&creatingSite&&siteDraftLockToGrid&&current.terrainGridValid) {
            TerrainGrid grid{};grid.originX=current.terrainGridOriginX;grid.originZ=current.terrainGridOriginZ;
            grid.stepX=current.terrainGridStepX;grid.stepZ=current.terrainGridStepZ;
            p=SnapToTerrainGrid(p,grid);
        }
        if (!ReadHeight(p.x,p.z,&p.y)) { if(siteEditorOpen)SiteActionError(creatingSite?0:editingSiteId,"Cannot sample terrain here. Try another point.");else Message("Cannot sample terrain here. Try another point."); break; }
        if(current.mode==PickMode::None&&!siteEditorOpen) {
            auto site=std::find_if(durableState.earthworks.begin(),durableState.earthworks.end(),[&](const SavedEarthwork& item) {
                if(item.projectId!=durableState.selectedProjectId)return false;
                return item.plan.Contains(p.x,p.z);
            });
            if(site!=durableState.earthworks.end()) {
                editingSiteId=site->id;selectedProjectId=site->id;creatingSite=false;siteEditorOpen=true;panelOpen=true;SelectSingleSitePoint(0);
                RefreshProjectStatus();Message("Site editor opened.");
            }
        } else if(current.mode==PickMode::SampleSitePointTarget) {
            if(creatingSite) {
                const uint32_t mask=ValidPointMask(selectedSitePointsMask,current.plan.pointCount);
                if(selectedSitePointIndex>=current.plan.pointCount||!mask) {current.mode=PickMode::None;SiteActionError(0,"Select at least one Site point before sampling its height.");break;}
                history.Commit(current.plan);for(unsigned i=0;i<current.plan.pointCount;++i)if(mask&PointMask(i))current.plan.points[i].y=p.y;current.plan.RefreshBounds();
                current.mode=PickMode::None;ClearSiteActionError(0);Changed();Message("Draft point height sampled; terrain remains unchanged.");
            } else {
                auto* site=FindProject(editingSiteId);
                if(!site||site->projectId!=durableState.selectedProjectId||site->status==ProjectStatus::Completed) {
                    current.mode=PickMode::None;SiteActionError(editingSiteId,"The selected Site can no longer be edited.");break;
                }
                const uint32_t mask=ValidPointMask(selectedSitePointsMask,site->plan.pointCount);
                if(selectedSitePointIndex>=site->plan.pointCount||!mask) {current.mode=PickMode::None;SiteActionError(site->id,"Select at least one Site point before sampling its height.");break;}
                if(site->status==ProjectStatus::Running||site->status==ProjectStatus::Waiting){site->status=ProjectStatus::Paused;projectSchedulerDirty=true;}
                if(site->status==ProjectStatus::Paused)site->needsReview=true;
                for(unsigned i=0;i<site->plan.pointCount;++i)if(mask&PointMask(i))site->plan.points[i].y=p.y;
                site->plan.RefreshBounds();current.mode=PickMode::None;selectedProjectId=site->id;siteEditorOpen=true;++current.revision;
                ClearSiteActionError(site->id);RefreshProjectStatus();Message(site->needsReview?"Point height sampled. Review this Site before resuming.":"Point height sampled; terrain remains unchanged.");
            }
        } else if (current.mode == PickMode::Sample) {
            history.Commit(current.plan); current.plan.target = p.y;
            for(unsigned i=0;i<current.plan.pointCount;++i)current.plan.points[i].y=p.y;
            current.plan.RefreshBounds();current.mode = PickMode::None;
            Message("Height sampled. Adjust the target or select a new area."); Changed();
        } else if(current.mode==PickMode::DrawSite) {
            if(shapeMode==SiteShapeMode::FreeDraw&&current.plan.pointCount) {
                const auto& first=current.plan.points[0];
                const double dx=static_cast<double>(p.x)-first.x,dz=static_cast<double>(p.z)-first.z;
                if(dx*dx+dz*dz<static_cast<double>(MinSitePointSpacing)*MinSitePointSpacing) {
                    if(current.plan.pointCount<3)SiteActionError(0,"Add at least three boundary points before closing the shape.");
                    else FinishCurrentSiteShape();
                    goto pick_done;
                }
            }
            if(current.plan.pointCount>=MaxSitePoints){SiteActionError(0,"This Site has reached its 32-point limit.");break;}
            if(shapeMode!=SiteShapeMode::FreeDraw&&current.plan.pointCount==1) {
                Point corners[4]{};BuildTwoCornerShape(current.plan.points[0],p,shapeMode,corners);
                const float width=std::abs(corners[2].x-corners[0].x),depth=std::abs(corners[2].z-corners[0].z);
                if(width<MinSitePointSpacing||depth<MinSitePointSpacing){SiteActionError(0,"Square and Rectangle sides must each be at least 5 m.");goto pick_done;}
                Plan candidate=current.plan;std::copy(std::begin(corners),std::end(corners),candidate.points.begin());candidate.pointCount=4;candidate.first=true;candidate.complete=false;candidate.RefreshBounds();
                if(!ValidFootprint(candidate)){SiteActionError(0,"That shape is invalid. Move farther from the first corner.");goto pick_done;}
                if(!SamplePlan(&candidate)){SiteActionError(0,"Could not sample the Site area. Keep it inside the terrain.");goto pick_done;}
                history.Commit(current.plan);candidate.complete=true;current.plan=std::move(candidate);current.mode=PickMode::None;
                SelectSingleSitePoint(0);ClearSiteActionError(0);Changed();Message("Shape ready. Set point heights, then save the Site.");goto pick_done;
            }
        if(current.plan.pointCount) {
                for(unsigned i=0;i<current.plan.pointCount;++i) {
                    const double dx=static_cast<double>(p.x)-current.plan.points[i].x,dz=static_cast<double>(p.z)-current.plan.points[i].z;
                    if(dx*dx+dz*dz<static_cast<double>(MinSitePointSpacing)*MinSitePointSpacing) {SiteActionError(0,"Site points must be at least 5 m apart.");goto pick_done;}
                }
                p.y=current.plan.points[0].y;
            }
            history.Commit(current.plan);current.plan.points[current.plan.pointCount++]=p;current.plan.first=true;current.plan.complete=false;
            current.plan.RefreshBounds();SelectSingleSitePoint(current.plan.pointCount-1);ClearSiteActionError(0);Message("Site boundary point added.");Changed();
        }
pick_done:
        break;
    }
    case Action::Target:
        if (std::isfinite(command.value) && std::abs(command.value)<100000) {
            history.Commit(current.plan); current.plan.target = command.value;
            for(unsigned i=0;i<current.plan.pointCount;++i)current.plan.points[i].y=command.value;
            current.plan.RefreshBounds();ClearSiteActionError(0);Changed(); Message("Target updated. Actual terrain is unchanged.");
        } else Message("Enter a finite height within the terrain preview's numeric range.");
        break;
    case Action::SetSiteTarget: {
        auto* site=FindProject(command.projectId);
        if(!site||site->projectId!=durableState.selectedProjectId||!std::isfinite(command.value)||std::abs(command.value)>=100000) {
            SiteActionError(command.projectId,"That Site target could not be updated.");break;
        }
        if(site->status==ProjectStatus::Completed) {SiteActionError(site->id,"Finished Sites cannot be edited.");break;}
        const bool pausedForEdit=site->status==ProjectStatus::Running||site->status==ProjectStatus::Waiting;
        if(pausedForEdit){site->status=ProjectStatus::Paused;projectSchedulerDirty=true;}
        if(site->status==ProjectStatus::Paused)site->needsReview=true;
        site->plan.target=command.value;
        for(unsigned i=0;i<site->plan.pointCount;++i)site->plan.points[i].y=command.value;
        site->plan.RefreshBounds();editingSiteId=site->id;selectedProjectId=site->id;siteEditorOpen=true;creatingSite=false;++current.revision;
        ClearSiteActionError(site->id);RefreshProjectStatus();Message(pausedForEdit?"Site paused and target updated. Review terrain before resuming.":site->needsReview?"Target changed. Review this Site before resuming.":"Site target updated; terrain remains unchanged.");break;
    }
    case Action::SelectSitePoint: {
        const Plan* plan=creatingSite?&current.plan:nullptr;
        if(!plan)if(auto* site=FindProject(editingSiteId))plan=&site->plan;
        if(!plan||command.pointIndex>=plan->pointCount)break;
        if(command.value>0.5f) {
            const uint32_t bit=PointMask(command.pointIndex);
            const uint32_t currentMask=ValidPointMask(selectedSitePointsMask,plan->pointCount);
            selectedSitePointsMask=currentMask&bit?currentMask&~bit:currentMask|bit;
            if(selectedSitePointsMask&bit)selectedSitePointIndex=command.pointIndex;
            else if(selectedSitePointsMask) {
                for(unsigned i=0;i<plan->pointCount;++i)if(selectedSitePointsMask&PointMask(i)){selectedSitePointIndex=i;break;}
            }
            else selectedSitePointIndex=command.pointIndex;
        } else {
            SelectSingleSitePoint(command.pointIndex);
        }
        RefreshProjectStatus();break;
    }
    case Action::SetSitePointTarget:
    case Action::AdjustSitePointTarget: {
        auto* site=creatingSite?nullptr:FindProject(editingSiteId);
        Plan* plan=creatingSite?&current.plan:(site?&site->plan:nullptr);
        if(!plan||command.pointIndex>=plan->pointCount||!std::isfinite(command.value)||std::abs(command.value)>=100000) {SiteActionError(site?site->id:0,"Select a valid Site point and height.");break;}
        if(site&&site->status==ProjectStatus::Completed){SiteActionError(site->id,"Finished Sites cannot be edited.");break;}
        const bool relative=command.action==Action::AdjustSitePointTarget;
        const uint32_t selectedMask=ValidPointMask(selectedSitePointsMask,plan->pointCount);
        const uint32_t mask=command.allSitePoints&&relative
            ?(plan->pointCount==MaxSitePoints?UINT32_MAX:((1u<<plan->pointCount)-1u)):selectedMask;
        if(!mask){SiteActionError(site?site->id:0,"Select at least one Site point before changing height.");break;}
        bool valid=true;
        for(unsigned i=0;i<plan->pointCount;++i)if(mask&PointMask(i)) {
            const float height=relative?plan->points[i].y+command.value:command.value;
            if(!std::isfinite(height)||std::abs(height)>=100000)valid=false;
        }
        if(!valid){SiteActionError(site?site->id:0,"The adjusted height is outside the allowed range.");break;}
        if(site) {
            const bool active=site->status==ProjectStatus::Running||site->status==ProjectStatus::Waiting;
            if(active){site->status=ProjectStatus::Paused;projectSchedulerDirty=true;}
            if(site->status==ProjectStatus::Paused)site->needsReview=true;
        }
        for(unsigned i=0;i<plan->pointCount;++i)if(mask&PointMask(i))
            plan->points[i].y=relative?plan->points[i].y+command.value:command.value;
        plan->RefreshBounds();selectedSitePointIndex=command.pointIndex;selectedSitePointsMask=selectedMask;
        if(site){editingSiteId=site->id;selectedProjectId=site->id;siteEditorOpen=true;creatingSite=false;ClearSiteActionError(site->id);++current.revision;RefreshProjectStatus();Message(site->needsReview?"Point height changed. Review this Site before resuming.":"Point height changed; terrain remains unchanged.");}
        else {ClearSiteActionError(0);Changed();Message("Point height changed; terrain is unchanged.");}
        break;
    }
    case Action::AddSitePoint:
    case Action::RemoveSitePoint:
    case Action::MoveSitePoint:
    case Action::MoveSiteShape: {
        auto* site=creatingSite?nullptr:FindProject(editingSiteId);
        Plan* plan=creatingSite?&current.plan:(site?&site->plan:nullptr);
        if(!plan||(!plan->complete&&!(creatingSite&&command.action==Action::MoveSitePoint))
            ||(!creatingSite&&(!site||site->status==ProjectStatus::Completed))) {SiteActionError(site?site->id:0,"Select an editable Site first.");break;}
        Plan edited=*plan;
        const unsigned index=command.pointIndex;
        if(command.action==Action::AddSitePoint) {
            if(edited.pointCount>=MaxSitePoints||index>=edited.pointCount){SiteActionError(site?site->id:0,"Cannot add another point here.");break;}
            const unsigned next=(index+1)%edited.pointCount;
            const double dx=static_cast<double>(edited.points[index].x)-edited.points[next].x,dz=static_cast<double>(edited.points[index].z)-edited.points[next].z;
            if(command.value>0.5f) {
                const double edgeLengthSquared=dx*dx+dz*dz;
                const double ax=static_cast<double>(command.point.x)-edited.points[index].x,az=static_cast<double>(command.point.z)-edited.points[index].z;
                const double bx=static_cast<double>(command.point.x)-edited.points[next].x,bz=static_cast<double>(command.point.z)-edited.points[next].z;
                const double spacingSquared=static_cast<double>(MinSitePointSpacing)*MinSitePointSpacing;
                if(!std::isfinite(command.point.x)||!std::isfinite(command.point.y)||!std::isfinite(command.point.z)
                    ||edgeLengthSquared<4.0*spacingSquared||ax*ax+az*az<spacingSquared||bx*bx+bz*bz<spacingSquared) {
                    SiteActionError(site?site->id:0,"Choose a point at least 5 m from both ends of the edge.");break;
                }
            } else if(dx*dx+dz*dz<4.0*MinSitePointSpacing*MinSitePointSpacing) {
                SiteActionError(site?site->id:0,"This edge is too short to add a point while keeping the 5 m spacing.");break;
            }
            const Point insertedPoint=command.value>0.5f?command.point:Point{(edited.points[index].x+edited.points[next].x)*0.5f,(edited.points[index].y+edited.points[next].y)*0.5f,(edited.points[index].z+edited.points[next].z)*0.5f};
            for(unsigned i=edited.pointCount;i>next;--i)edited.points[i]=edited.points[i-1];
            edited.points[next]=insertedPoint;++edited.pointCount;selectedSitePointIndex=next;
        } else if(command.action==Action::RemoveSitePoint) {
            if(edited.pointCount<=3||index>=edited.pointCount){SiteActionError(site?site->id:0,"A Site needs at least three points.");break;}
            for(unsigned i=index;i+1<edited.pointCount;++i)edited.points[i]=edited.points[i+1];
            --edited.pointCount;selectedSitePointIndex=std::min<unsigned>(index,edited.pointCount-1);
        } else if(command.action==Action::MoveSitePoint) {
            if(index>=edited.pointCount||!std::isfinite(command.point.x)||!std::isfinite(command.point.z)){SiteActionError(site?site->id:0,"Select a valid point to move.");break;}
            edited.points[index].x=command.point.x;edited.points[index].z=command.point.z;selectedSitePointIndex=index;
        } else {
            if(!edited.complete||!std::isfinite(command.point.x)||!std::isfinite(command.point.z)
                ||(std::abs(command.point.x)<0.5f&&std::abs(command.point.z)<0.5f)) {SiteActionError(site?site->id:0,"Drag the Site at least 0.5 m to move it.");break;}
            for(unsigned i=0;i<edited.pointCount;++i){edited.points[i].x+=command.point.x;edited.points[i].z+=command.point.z;}
        }
        edited.RefreshBounds();
        if(edited.complete) {
            if(!ValidFootprint(edited)){SiteActionError(site?site->id:0,"That edit makes the Site invalid. Keep points at least 5 m apart and avoid crossing edges.");break;}
            const uint64_t editedProjectId=site?site->projectId:durableState.selectedProjectId;
            bool overlap=false;
            for(const auto& other:durableState.earthworks)if(other.id!=(site?site->id:0)
                &&other.projectId!=editedProjectId
                &&ProjectReservesFootprint(other.status)&&FootprintsOverlap(edited,other.plan))overlap=true;
            if(overlap){SiteActionError(site?site->id:0,"That edit overlaps an unfinished Site in another Project.");break;}
            if(!SamplePlan(&edited)){SiteActionError(site?site->id:0,"Could not resample the edited Site. Its previous shape is kept.");break;}
        } else {
            bool tooClose=false;
            for(unsigned i=0;i<edited.pointCount;++i)for(unsigned j=i+1;j<edited.pointCount;++j) {
                const double dx=static_cast<double>(edited.points[i].x)-edited.points[j].x,dz=static_cast<double>(edited.points[i].z)-edited.points[j].z;
                if(dx*dx+dz*dz<static_cast<double>(MinSitePointSpacing)*MinSitePointSpacing)tooClose=true;
            }
            if(tooClose){SiteActionError(site?site->id:0,"Site points must be at least 5 m apart.");break;}
        }
        if(site) {
            if(site->status==ProjectStatus::Running||site->status==ProjectStatus::Waiting){site->status=ProjectStatus::Paused;projectSchedulerDirty=true;}
            if(site->status==ProjectStatus::Paused)site->needsReview=true;
        }
        *plan=std::move(edited);selectedSitePointsMask=PointMask(selectedSitePointIndex);
        if(site){if(site->status==ProjectStatus::Paused)site->needsReview=true;editingSiteId=site->id;selectedProjectId=site->id;siteEditorOpen=true;creatingSite=false;ClearSiteActionError(site->id);++current.revision;RefreshProjectStatus();Message(site->needsReview?"Site geometry changed. Review terrain before resuming.":"Site geometry updated; terrain is unchanged.");}
        else {ClearSiteActionError(0);Changed();Message("Draft Site geometry updated.");}
        break;
    }
    case Action::Clear:
        history.Commit(current.plan); current.plan = Plan{}; current.mode = PickMode::None;
        Changed(); Message("Preview cleared. Select area to begin again."); break;
    case Action::Undo:
        current.plan = history.Undo(current.plan);
        current.mode = current.plan.complete ? PickMode::None : PickMode::DrawSite;
        Changed(); Message("Undid the last draft edit."); break;
    case Action::Redo:
        current.plan = history.Redo(current.plan);
        current.mode = current.plan.complete ? PickMode::None : PickMode::DrawSite;
        Changed(); Message("Redid the draft edit."); break;
    case Action::ConfirmProject: {
        UpdateSiteDraftValidation();
        if(current.siteDraftError[0]) {SiteActionError(0,current.siteDraftError);break;}
        std::string siteName=command.text[0]?command.text:durableState.draftName;
        while(!siteName.empty()&&std::isspace(static_cast<unsigned char>(siteName.front())))siteName.erase(siteName.begin());
        while(!siteName.empty()&&std::isspace(static_cast<unsigned char>(siteName.back())))siteName.pop_back();
        if(siteName.empty()||siteName.size()>MaxSavedNameBytes) {SiteActionError(0,"Enter a Site name of 1 to 48 characters.");break;}
        bool invalidName=false;for(unsigned char c:siteName)if(c<0x20||c==0x7f)invalidName=true;
        if(invalidName) {SiteActionError(0,"Site names cannot contain control characters.");break;}
        bool duplicateName=false;
        for(const auto& existing:durableState.earthworks)
            if(existing.projectId==durableState.selectedProjectId&&_stricmp(existing.name.c_str(),siteName.c_str())==0)duplicateName=true;
        if(duplicateName) {SiteActionError(0,"A Site with that name already exists in this Project.");break;}
        SavedEarthwork project;
        project.id=1;
        for(const auto& saved:durableState.earthworks)project.id=std::max(project.id,saved.id+1);
        project.projectId=durableState.selectedProjectId;
        project.name=siteName;
        ++durableState.nextSiteNumber;
        SetDefaultDraftSiteName();
        project.status=ProjectStatus::Queued;
        project.plan=current.plan;
        durableState.earthworks.push_back(project);
        selectedProjectId=project.id;
        editingSiteId=project.id;creatingSite=false;siteEditorOpen=true;
        ClearSiteActionError(0);
        RefreshProjectStatus();
        current.plan=Plan{};
        current.mode=PickMode::None;
        history.Reset();
        Changed();
        host->log("earthworks  site confirmed and queued id=%llu name=%s priority=%zu",static_cast<unsigned long long>(project.id),project.name.c_str(),durableState.earthworks.size());
        Message("Earthwork site queued in the selected project.");
        break;
    }
    case Action::SetDraftSiteName: {
        if(!creatingSite)break;
        std::string name=command.text;
        while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.front())))name.erase(name.begin());
        while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.back())))name.pop_back();
        if(name.empty()||name.size()>MaxSavedNameBytes)break;
        bool invalid=false;for(unsigned char c:name)if(c<0x20||c==0x7f)invalid=true;
        if(invalid)break;
        durableState.draftName=std::move(name);
        ClearSiteActionError(0);
        RefreshProjectStatus();
        break;
    }
    case Action::SelectNamedProject:
        if(FindNamedProject(command.projectId)) {
            if(creatingSite) {Message("Save or discard the current Site draft before switching Projects.");break;}
            durableState.selectedProjectId=command.projectId;selectedProjectId=0;editingSiteId=0;siteEditorOpen=false;creatingSite=false;RefreshProjectStatus();
            Message("Project selected. Its Sites are shown below.");
        }
        break;
    case Action::CreateNamedProject: {
        std::string name=command.text;while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.front())))name.erase(name.begin());
        while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.back())))name.pop_back();
        if(name.empty()||name.size()>MaxSavedNameBytes) {Message("Enter a Project name of 1 to 48 characters.");break;}
        bool duplicate=false;for(const auto& item:durableState.projects)if(_stricmp(item.name.c_str(),name.c_str())==0)duplicate=true;
        if(duplicate) {Message("A Project with that name already exists.");break;}
        if(durableState.projects.size()>=MaxSavedProjects) {Message("Project limit reached.");break;}
        SavedProject project;project.id=1;for(const auto& item:durableState.projects)project.id=std::max(project.id,item.id+1);
        project.name=std::move(name);durableState.projects.push_back(std::move(project));durableState.selectedProjectId=durableState.projects.back().id;
        if(!creatingSite) {selectedProjectId=0;editingSiteId=0;siteEditorOpen=false;}
        RefreshProjectStatus();
        Message(creatingSite?"Project created and selected; the current Site draft will be saved in it.":"Project created.");
        break;
    }
    case Action::RenameNamedProject: {
        auto* project=FindNamedProject(command.projectId);if(!project)break;
        std::string name=command.text;while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.front())))name.erase(name.begin());
        while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.back())))name.pop_back();
        if(name.empty()||name.size()>MaxSavedNameBytes) {Message("Enter a Project name of 1 to 48 characters.");break;}
        bool duplicate=false;for(const auto& item:durableState.projects)if(item.id!=project->id&&_stricmp(item.name.c_str(),name.c_str())==0)duplicate=true;
        if(duplicate) {Message("A Project with that name already exists.");break;}
        project->name=std::move(name);RefreshProjectStatus();Message("Project renamed.");break;
    }
    case Action::DeleteNamedProject: {
        if(creatingSite&&command.projectId==durableState.selectedProjectId) {Message("Save or discard the current Site draft before deleting its Project.");break;}
        const auto project=std::find_if(durableState.projects.begin(),durableState.projects.end(),[&](const SavedProject& item){return item.id==command.projectId;});
        if(project==durableState.projects.end())break;
        const bool containsSites=std::any_of(durableState.earthworks.begin(),durableState.earthworks.end(),[&](const SavedEarthwork& site) {
            return site.projectId==project->id;
        });
        if(containsSites) {Message("Delete each Site with its X button before deleting this Project.");break;}
        const auto deletedId=project->id;
        durableState.projects.erase(project);
        if(durableState.selectedProjectId==deletedId)durableState.selectedProjectId=durableState.projects.empty()?0:durableState.projects.front().id;
        selectedProjectId=0;editingSiteId=0;siteEditorOpen=false;creatingSite=false;RefreshProjectStatus();Message("Project deleted.");break;
    }
    case Action::RenameSite: {
        auto* site=FindProject(command.projectId);if(!site||site->projectId!=durableState.selectedProjectId)break;
        std::string name=command.text;while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.front())))name.erase(name.begin());
        while(!name.empty()&&std::isspace(static_cast<unsigned char>(name.back())))name.pop_back();
        if(name.empty()||name.size()>MaxSavedNameBytes) {SiteActionError(site->id,"Enter a Site name of 1 to 48 characters.");break;}
        bool duplicate=false;for(const auto& item:durableState.earthworks)if(item.projectId==site->projectId&&item.id!=site->id&&_stricmp(item.name.c_str(),name.c_str())==0)duplicate=true;
        if(duplicate) {SiteActionError(site->id,"A Site with that name already exists in this Project.");break;}
        site->name=std::move(name);ClearSiteActionError(site->id);RefreshProjectStatus();Message("Site renamed.");break;
    }
    case Action::AddProjectFacility: {
        auto* project=FindNamedProject(durableState.selectedProjectId);if(!project)break;
        const auto id=static_cast<uint32_t>(command.projectId);BuildingReading facility{};
        if(!id||!FindEarthworksFacility(id,&facility)) {Message("That Earthworks facility is no longer available.");break;}
        const auto* owner=FacilityOwner(id);
        if(owner&&owner->id!=project->id) {Message("That facility is already assigned to another Project.");break;}
        if(std::find(project->facilityIds.begin(),project->facilityIds.end(),id)!=project->facilityIds.end())break;
        if(project->facilityIds.size()>=MaxProjectFacilities) {Message("This Project has reached its facility limit.");break;}
        project->facilityIds.push_back(id);
        project->facilityAnchors.resize(project->facilityIds.size());
        SaveFacilityAnchor(project->facilityAnchors.back(),facility);
        ResetProjectWorkBacklog(project->id);
        RefreshProjectStatus();Message("Facility added to this Project.");break;
    }
    case Action::RemoveProjectFacility: {
        auto* project=FindNamedProject(durableState.selectedProjectId);if(!project)break;
        const auto id=static_cast<uint32_t>(command.projectId);
        const auto it=std::find(project->facilityIds.begin(),project->facilityIds.end(),id);
        if(it==project->facilityIds.end()) {Message("This facility is not assigned to the selected Project.");break;}
        const auto index=static_cast<size_t>(std::distance(project->facilityIds.begin(),it));
        project->facilityIds.erase(it);
        if(index<project->facilityAnchors.size())project->facilityAnchors.erase(project->facilityAnchors.begin()+index);
        ResetProjectWorkBacklog(project->id);
        ++current.revision;
        RefreshProjectStatus();
        host->log("earthworks  facility removed from project=%llu facility=%u remaining=%zu",
            static_cast<unsigned long long>(project->id),id,project->facilityIds.size());
        Message(project->facilityIds.empty()
            ?"Facility removed. Active Sites will pause with progress preserved because the Project has no assigned facilities."
            :"Facility removed. Active Site work will be rebalanced across the remaining facilities.");
        break;
    }
    case Action::SelectProject:
        if(auto* site=FindProject(command.projectId);site&&site->projectId==durableState.selectedProjectId) {
            selectedProjectId=site->id;editingSiteId=site->id;creatingSite=false;siteEditorOpen=true;SelectSingleSitePoint(0);RefreshProjectStatus();
        }
        break;
    case Action::SetSiteVisibility: {
        auto* site=FindProject(command.projectId);
        if(!site||site->projectId!=durableState.selectedProjectId)break;
        site->visible=command.value!=0.0f;
        ++current.revision;RefreshProjectStatus();
        Message(site->visible?"Site shown on the terrain.":"Site hidden from the terrain.");
        break;
    }
    case Action::SetProjectSitesVisibility: {
        auto* project=FindNamedProject(command.projectId);
        if(!project||project->id!=durableState.selectedProjectId)break;
        const bool visible=command.value!=0.0f;
        bool changed=false;
        for(auto& site:durableState.earthworks)if(site.projectId==project->id&&site.visible!=visible) {
            site.visible=visible;changed=true;
        }
        if(changed)++current.revision;
        RefreshProjectStatus();
        Message(visible?"All Sites in this Project are shown on the terrain.":"All Sites in this Project are hidden from the terrain.");
        break;
    }
    case Action::RemoveProject: {
        const auto project=std::find_if(durableState.earthworks.begin(),durableState.earthworks.end(),[&](const SavedEarthwork& item) {
            return item.id==command.projectId;
        });
        if(project==durableState.earthworks.end())break;
        projectSchedulerDirty=true;
        if(selectedProjectId==project->id)selectedProjectId=0;
        const auto removedId=project->id;
        durableState.earthworks.erase(project);
        if(editingSiteId==removedId){editingSiteId=0;siteEditorOpen=false;}
        RefreshProjectStatus();
        host->log("earthworks  site deleted id=%llu",static_cast<unsigned long long>(removedId));
        Message("Site deleted. Terrain already changed is kept.");
        break;
    }
    case Action::MoveProjectUp:
    case Action::MoveProjectDown: {
        auto it=std::find_if(durableState.earthworks.begin(),durableState.earthworks.end(),[&](const SavedEarthwork& p){return p.id==command.projectId;});
        if(it==durableState.earthworks.end()||!ProjectReservesFootprint(it->status))break;
        if(command.action==Action::MoveProjectUp) {
            auto prior=it;bool found=false;
            while(prior!=durableState.earthworks.begin()){--prior;if(prior->projectId==it->projectId&&ProjectReservesFootprint(prior->status)){found=true;break;}}
            if(found)std::iter_swap(it,prior);
        } else {
            auto next=it;bool found=false;
            while(++next!=durableState.earthworks.end())if(next->projectId==it->projectId&&ProjectReservesFootprint(next->status)){found=true;break;}
            if(found)std::iter_swap(it,next);
        }
        RefreshProjectStatus();Message("Site priority order updated.");break;
    }
    case Action::AssignProject: {
        if(!current.projectQueued) { Message("Select a project before assigning a facility."); break; }
        if(current.projectStatus!=ProjectStatus::Queued&&current.projectStatus!=ProjectStatus::Paused) {Message("Only queued or paused projects can be assigned or reassigned.");break;}
        const auto workforce=ReadWorkforce();
        if(!workforce.selected || !workforce.buildingValid || !workforce.building.earthworksFacility) {
            Message("Select a recognized Earthworks facility in the inspector first.");break;
        }
        for(auto& project:durableState.earthworks)if(project.id==current.projectId && (project.status==ProjectStatus::Queued||project.status==ProjectStatus::Paused)) {
            auto* namedProject=FindNamedProject(project.projectId);
            if(!namedProject)break;
            namedProject->facilityIds.assign(1,workforce.building.id);
            namedProject->facilityAnchors.resize(1);SaveFacilityAnchor(namedProject->facilityAnchors.front(),workforce.building);
            ResetProjectWorkBacklog(namedProject->id);
            RefreshProjectStatus();
            SelectEarthworksFacility(workforce.building.id);
            Message(project.status==ProjectStatus::Paused
                ? (project.needsReview?"Project reassigned. Progress is kept; review the footprint before resuming.":"Project reassigned while paused. Progress is kept.")
                : "Project assigned. It will not start until you press Start.");
            break;
        }
        break;
    }
    case Action::ClearProjectAssignment:
        for(auto& project:durableState.earthworks)if(project.id==current.projectId && project.status==ProjectStatus::Queued) {
            if(auto* namedProject=FindNamedProject(project.projectId)) {
                if(!namedProject->facilityIds.empty())ResetProjectWorkBacklog(namedProject->id);
                namedProject->facilityIds.clear();namedProject->facilityAnchors.clear();
            }
        }
        RefreshProjectStatus();Message("Facility assignment cleared. The queued project remains saved.");break;
    case Action::StartProject:
    case Action::RestartProject: {
        uint64_t completedSiteId=0;
        for(auto& project:durableState.earthworks) {
            const bool restarting=command.action==Action::RestartProject;
            const auto initialStatus=project.status;
            const bool restartableStatus=initialStatus==ProjectStatus::Completed;
            if(project.id!=command.projectId||(restarting?!restartableStatus:initialStatus!=ProjectStatus::Queued))continue;
            const char* retainedStatus="The Site remains completed.";
            BuildingReading facility{};WorkforceView workforce{};uint32_t facilityId=0;
            ClearSiteActionError(project.id);
            if(!raiseLower || !heightQuery || !openHeightmap || !closeHeightmap) {
                if(restarting){char message[112]{};sprintf_s(message,"Native terrain writer is unavailable. %s",retainedStatus);SiteActionError(project.id,message);}
                else SiteActionError(project.id,"Native terrain writer is unavailable for this game build. Site remains queued.");
                break;
            }
            if(!FindAssignedFacility(project,&facilityId,&facility)) {
                if(restarting){char message[112]{};sprintf_s(message,"Assigned Earthworks facility is missing or unreadable. %s",retainedStatus);SiteActionError(project.id,message);}
                else SiteActionError(project.id,"Assigned Earthworks facility is missing or unreadable.");
                break;
            }
            if(!ReadEarthworksFacilityWorkforce(facilityId,&workforce)||!workforce.buildingValid
                ||!workforce.vehiclesValid||!workforce.parkedValid) {
                if(restarting){char message[112]{};sprintf_s(message,"Assigned facility readings are incomplete. %s",retainedStatus);SiteActionError(project.id,message);}
                else SiteActionError(project.id,"Assigned facility readings are incomplete. Try again when the building can be read.");
                break;
            }
            if(restarting) {
                const bool overlapsOtherProject=std::any_of(durableState.earthworks.begin(),durableState.earthworks.end(),[&](const SavedEarthwork& other) {
                    return other.id!=project.id&&other.projectId!=project.projectId
                        &&ProjectReservesFootprint(other.status)&&FootprintsOverlap(project.plan,other.plan);
                });
                if(overlapsOtherProject) {SiteActionError(project.id,"This Site overlaps an unfinished Site in another Project. Move or cancel that Site before restarting.");break;}
            }
            TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project.plan,&grid,&rect);
            if(areaError!=ProjectAreaError::None) {
                host->log("earthworks  start rejected project=%llu facility=%u area=%s grid=%dx%d origin=(%.3f,%.3f) step=(%.3f,%.3f) plan=(%.3f,%.3f)..(%.3f,%.3f)",
                    static_cast<unsigned long long>(project.id),facilityId,ProjectAreaErrorText(areaError),grid.width,grid.depth,
                    grid.originX,grid.originZ,grid.stepX,grid.stepZ,project.plan.a.x,project.plan.a.z,project.plan.b.x,project.plan.b.z);
                SiteActionError(project.id,ProjectAreaMessage(areaError));break;
            }
            Plan refreshed=project.plan;
            if(!SamplePlan(&refreshed)) {
                if(restarting){char message[112]{};sprintf_s(message,"Cannot read the current Site terrain. %s",retainedStatus);SiteActionError(project.id,message);}
                else SiteActionError(project.id,"Cannot recheck every planned terrain sample. Replan this Site before starting.");
                break;
            }
            float maxDifference=0;
            if(!restarting)for(size_t i=0;i<refreshed.ground.size();++i)maxDifference=std::max(maxDifference,std::abs(refreshed.ground[i]-project.plan.ground[i]));
            if(!restarting&&maxDifference>TerrainReadbackToleranceMeters) {
                host->log("earthworks  start rejected project=%llu terrain changed after planning maxDelta=%.6f tolerance=%.3f area=(%.3f,%.3f)..(%.3f,%.3f)",
                    static_cast<unsigned long long>(project.id),maxDifference,TerrainReadbackToleranceMeters,project.plan.a.x,project.plan.a.z,project.plan.b.x,project.plan.b.z);
                SiteActionError(project.id,"Terrain changed since planning. Replan the footprint before starting.");break;
            }
            double totalWork=0;
            if(!MeasureTerrainWork(refreshed,grid,rect,&totalWork)) {
                if(restarting){char message[112]{};sprintf_s(message,"Could not measure current terrain work. %s",retainedStatus);SiteActionError(project.id,message);}
                else SiteActionError(project.id,"Could not measure the Site's starting terrain work. It remains queued.");
                break;
            }
            if(restarting)project.plan=std::move(refreshed);
            project.totalWorkM3=static_cast<float>(totalWork);
            project.progress=totalWork<=1e-6?1.0f:(restarting?0.0f:std::clamp(project.progress,0.0f,0.999999f));
            project.status=totalWork<=1e-6?ProjectStatus::Completed:ProjectStatus::Running;
            if(project.status==ProjectStatus::Completed)completedSiteId=project.id;
            project.needsReview=false;project.workRemainderM3=0;project.allocatedRateUnits=0;project.allocatedMachines=0;project.allocatedWorkers=0;
            host->log("earthworks  site=%llu %s terrain work=%.3f m3",static_cast<unsigned long long>(project.id),restarting?"restarted":"started",totalWork);
            lastSimulationHour=SimulationHourClock(current);
            ClearSiteActionError(project.id);
            if(!completedSiteId)RefreshProjectStatus();
            Message(totalWork<=1e-6?"Site is already at its saved target height.":(restarting?"Site restarted from the current terrain using its saved shape and target heights.":"Site started. It works when its priority receives Project facility capacity."));break;
        }
        if(completedSiteId) {MoveCompletedSiteToProjectEnd(completedSiteId);RefreshProjectStatus();}
        break;
    }
    case Action::PauseProject:
        for(auto& project:durableState.earthworks)if(project.id==command.projectId && (project.status==ProjectStatus::Running||project.status==ProjectStatus::Waiting)) {
            project.status=ProjectStatus::Paused;project.allocatedRateUnits=0;project.allocatedMachines=0;project.allocatedWorkers=0;RefreshProjectStatus();Message("Site paused. Current terrain and progress are kept.");break;
        }
        break;
    case Action::ResumeProject:
        for(auto& project:durableState.earthworks)if(project.id==command.projectId && project.status==ProjectStatus::Paused && !project.needsReview) {
            BuildingReading facility{};uint32_t facilityId=0;
            ClearSiteActionError(project.id);
            if(!FindAssignedFacility(project,&facilityId,&facility)){SiteActionError(project.id,"Assign this paused Project to an available Earthworks facility first.");break;}
            TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project.plan,&grid,&rect);
            if(areaError!=ProjectAreaError::None) {SiteActionError(project.id,ProjectAreaMessage(areaError));break;}
            project.status=ProjectStatus::Running;lastSimulationHour=SimulationHourClock(current);
            ClearSiteActionError(project.id);RefreshProjectStatus();Message("Site resumed.");break;
        }
        break;
    case Action::ReviewResumeProject:
    {
        uint64_t completedSiteId=0;
        for(auto& project:durableState.earthworks)if(project.id==command.projectId && project.status==ProjectStatus::Paused && project.needsReview) {
            BuildingReading facility{};uint32_t facilityId=0;
            ClearSiteActionError(project.id);
            if(!FindAssignedFacility(project,&facilityId,&facility)){SiteActionError(project.id,"Assign this paused Project to an available Earthworks facility before reviewing it.");break;}
            TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project.plan,&grid,&rect);
            if(areaError!=ProjectAreaError::None) {SiteActionError(project.id,ProjectAreaMessage(areaError));break;}
            if(!SamplePlan(&project.plan)) {SiteActionError(project.id,"Could not read the Site footprint. It remains paused for review.");break;}
            double remainingWork=0;
            if(!MeasureTerrainWork(project.plan,grid,rect,&remainingWork)||!std::isfinite(remainingWork)||remainingWork<0) {
                SiteActionError(project.id,"Could not recalculate this Site's remaining work. It remains paused for review.");break;
            }
            const double oldProgress=std::clamp(static_cast<double>(project.progress),0.0,1.0);
            const double completedWork=project.totalWorkM3>1e-6
                ?std::clamp(static_cast<double>(project.totalWorkM3)*oldProgress,0.0,1.0e12)
                :std::clamp(remainingWork*oldProgress/std::max(1e-6,1.0-oldProgress),0.0,1.0e12);
            if(remainingWork<=1e-6) {
                project.totalWorkM3=static_cast<float>(completedWork);
                project.progress=1.0f;
                project.workRemainderM3=0;
                project.needsReview=false;project.status=ProjectStatus::Completed;
                completedSiteId=project.id;
                host->log("earthworks  site=%llu reviewed progress rebase old=%.5f completed=%.3f remaining=%.3f total=%.3f new=1.00000",
                    static_cast<unsigned long long>(project.id),oldProgress,completedWork,remainingWork,completedWork);
                ClearSiteActionError(project.id);Message("Footprint reviewed. The revised target is already satisfied; the Site is complete.");break;
            }
            const double recalculatedTotal=std::min(1.0e12,completedWork+remainingWork);
            project.totalWorkM3=static_cast<float>(recalculatedTotal);
            project.progress=recalculatedTotal>1e-6
                ?static_cast<float>(std::clamp(completedWork/recalculatedTotal,0.0,0.999999)):0.0f;
            project.workRemainderM3=static_cast<float>(std::min(static_cast<double>(project.workRemainderM3),remainingWork));
            host->log("earthworks  site=%llu reviewed progress rebase old=%.5f completed=%.3f remaining=%.3f total=%.3f new=%.5f",
                static_cast<unsigned long long>(project.id),oldProgress,completedWork,remainingWork,recalculatedTotal,
                static_cast<double>(project.progress));
            project.needsReview=false;project.status=ProjectStatus::Running;
            lastSimulationHour=SimulationHourClock(current);
            ClearSiteActionError(project.id);RefreshProjectStatus();Message("Footprint reviewed. Progress was recalculated and the Site resumed toward its saved target.");break;
        }
        if(completedSiteId) {MoveCompletedSiteToProjectEnd(completedSiteId);RefreshProjectStatus();}
        break;
    }
    case Action::InspectBuilding:
        current.mode=PickMode::InspectBuilding;Message("Click near your Earthworks facility on the map. Select it in the Projects tab.");break;
    case Action::ChooseBuilding:
        if(std::isfinite(command.value) && command.value>=0 && command.value<WorkforceCandidates) {
            SelectWorkforceBuilding(static_cast<unsigned>(command.value));RefreshProjectStatus();
        }
        break;
    case Action::ClearInspector: ResetWorkforce();current.mode=PickMode::None;break;
    case Action::LogInspector: LogWorkforce();break;
    case Action::RefreshFacilities:
        lastWorldFacilityScan=0;
        lastProjectFacilityReadings=0;
        RefreshProjectStatus();
        ++current.revision;
        Message("Facility names refreshed from the buildings.");
        host->log("earthworks  manually refreshed facility names from native building instances");
        break;
    case Action::DismissFacilityWarning:
        facilityAssignmentWarningPending=false;
        current.facilityAssignmentWarning=false;
        break;
    case Action::RemoveUnresolvedFacilityAssignments: {
        if(!facilityAssignmentWarningPending||unresolvedFacilityAssignments.empty())break;
        DurableState cleaned=durableState;
        size_t removed=0;
        std::vector<uint64_t> changedProjectIds;
        for(auto& project:cleaned.projects) {
            for(size_t i=project.facilityIds.size();i>0;--i) {
                const size_t index=i-1;
                const bool unresolved=std::any_of(unresolvedFacilityAssignments.begin(),unresolvedFacilityAssignments.end(),
                    [&](const UnresolvedFacilityAssignment& entry){return entry.projectId==project.id&&entry.index==index&&entry.facilityId==project.facilityIds[index];});
                if(!unresolved)continue;
                project.facilityIds.erase(project.facilityIds.begin()+static_cast<std::ptrdiff_t>(index));
                if(index<project.facilityAnchors.size())project.facilityAnchors.erase(project.facilityAnchors.begin()+static_cast<std::ptrdiff_t>(index));
                if(std::find(changedProjectIds.begin(),changedProjectIds.end(),project.id)==changedProjectIds.end())
                    changedProjectIds.push_back(project.id);
                ++removed;
            }
        }
        if(!removed) {
            lastFacilityIdentityReconcile=0;
            ReconcileSavedFacilities();
            if(unresolvedFacilityAssignments.empty()) {
                facilityAssignmentWarningPending=false;
                current.facilityAssignmentWarning=false;
                Message("No unresolved facility assignments remain.");
            } else {
                Message("Assignments changed before cleanup. The unresolved links were checked again.");
            }
            RefreshProjectStatus();break;
        }
        if(!SaveDurableState(host,host->exeBase+0x9D4F10,cleaned)) {
            Message("Could not update the saved Earthworks assignments. The warning remains open.");
            host->log("earthworks  refused to remove %zu unresolved assignment(s): companion save update failed",removed);
            break;
        }
        durableState=std::move(cleaned);
        for(const auto projectId:changedProjectIds)ResetProjectWorkBacklog(projectId);
        unresolvedFacilityAssignments.clear();facilityAssignmentWarningPending=false;
        lastFacilityIdentityReconcile=0;
        ReconcileSavedFacilities();
        facilityAssignmentWarningPending=!unresolvedFacilityAssignments.empty();
        current.facilityAssignmentWarning=false;
        RefreshProjectStatus();
        current.facilityAssignmentWarning=facilityAssignmentWarningPending;
        Message(facilityAssignmentWarningPending
            ? "Removed unresolved facility assignments. Some unresolved links remain."
            : "Removed unresolved facility assignments from this save.");
        host->log("earthworks  removed %zu unresolved facility assignment(s) from the active save",removed);
        break;
    }
    }
}
void NotifyNativeTerrainEditImpl(float x,float z,float brushRadius) noexcept
{
    if(!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(brushRadius))return;
    for(auto& project:durableState.earthworks)if(project.status==ProjectStatus::Running||project.status==ProjectStatus::Waiting) {
        const float radius=std::clamp(std::abs(brushRadius),0.0f,1000.0f);
        bool overlaps=project.plan.Contains(x,z);
        for(unsigned i=0;!overlaps&&i<project.plan.pointCount;++i) {
            const auto& a=project.plan.points[i];const auto& b=project.plan.points[(i+1)%project.plan.pointCount];
            const double vx=static_cast<double>(b.x)-a.x,vz=static_cast<double>(b.z)-a.z;
            const double length=vx*vx+vz*vz;
            const double t=length>0?std::clamp(((x-a.x)*vx+(z-a.z)*vz)/length,0.0,1.0):0.0;
            const double dx=x-(a.x+t*vx),dz=z-(a.z+t*vz);
            overlaps=dx*dx+dz*dz<=static_cast<double>(radius)*radius;
        }
        if(overlaps) {
            project.status=ProjectStatus::Paused;project.needsReview=true;
            project.allocatedRateUnits=0;project.allocatedMachines=0;project.allocatedWorkers=0;projectSchedulerDirty=true;
            RefreshProjectStatus();
            SiteActionError(project.id,"Native terrain edit overlapped this Site. It is paused; review the footprint before resuming.");
            host->log("earthworks  project=%llu paused for overlapping native terrain edit at (%.2f,%.2f), radius=%.2f",
                static_cast<unsigned long long>(project.id),x,z,radius);
            break;
        }
    }
}
void ExecuteProjectSlice(SavedEarthwork* project,double relativeRate,double elapsed)
{
    if(!project||relativeRate<=0||elapsed<=0)return;
    TerrainGrid grid{};GridRect rect{};const auto areaError=CheckProjectArea(project->plan,&grid,&rect);
    if(areaError==ProjectAreaError::TerrainMissing)return;
    if(areaError!=ProjectAreaError::None) {
        project->status=ProjectStatus::Paused;project->needsReview=false;project->allocatedRateUnits=0;project->allocatedMachines=0;project->allocatedWorkers=0;projectSchedulerDirty=true;RefreshProjectStatus();
        host->log("earthworks  project=%llu paused by area check: %s grid=%dx%d origin=(%.3f,%.3f) step=(%.3f,%.3f) plan=(%.3f,%.3f)..(%.3f,%.3f)",
            static_cast<unsigned long long>(project->id),ProjectAreaErrorText(areaError),grid.width,grid.depth,
            grid.originX,grid.originZ,grid.stepX,grid.stepZ,project->plan.a.x,project->plan.a.z,project->plan.b.x,project->plan.b.z);
        SiteActionError(project->id,ProjectAreaMessage(areaError));return;
    }
    void* terrain=TerrainObject();if(!terrain)return;
    const int count=rect.Count();
    const double cellArea=static_cast<double>(grid.stepX)*grid.stepZ;
    const double precision=static_cast<double>(grid.heightScale)/60000.0;
    const int batch=std::min(count,64);
    const double threshold=cellArea*precision*batch;
    // Each displayed Speed/Level point completes 250 m3 per in-game day at
    // 1x (60 simulation hours per day). Execution and forecasts share this
    // conversion; the global speed multiplier scales it.
    const double currentRateAllowance=relativeRate*ConstructionSpeedMultiplier()
        *VolumePerRateUnitPerSimulationHour*std::min(elapsed,1.0);
    const double maximumBufferedWork=currentRateAllowance+threshold;
    const double previousRemainder=std::max(0.0,static_cast<double>(project->workRemainderM3));
    const double pendingWork=previousRemainder+currentRateAllowance;
    if(pendingWork>maximumBufferedWork&&previousRemainder>maximumBufferedWork) {
        host->log("earthworks  site=%llu trimmed stale work backlog %.3f -> %.3f m3 at current rate %.3f units",
            static_cast<unsigned long long>(project->id),previousRemainder,maximumBufferedWork,relativeRate);
    }
    // Keep at most one current simulation slice plus the terrain precision
    // buffer. Older saved backlog must not let a reduced facility roster keep
    // executing at its former rate.
    project->workRemainderM3=static_cast<float>(std::min(pendingWork,maximumBufferedWork));
    if(project->workRemainderM3<threshold)return;
    const double maxStep=0.05;
    const double requestedStep=project->workRemainderM3/(cellArea*batch);
    const float step=static_cast<float>(std::min(requestedStep,maxStep));
    if(!std::isfinite(step) || step<=0)return;
    if(!openHeightmap || !closeHeightmap || !raiseLower || !heightQuery) {
        host->log("earthworks  site=%llu paused for review: terrain writer unavailable open=%d close=%d edit=%d read=%d",
            static_cast<unsigned long long>(project->id),openHeightmap!=nullptr,closeHeightmap!=nullptr,
            raiseLower!=nullptr,heightQuery!=nullptr);
        project->status=ProjectStatus::Paused;project->needsReview=true;project->allocatedRateUnits=0;project->allocatedMachines=0;project->allocatedWorkers=0;projectSchedulerDirty=true;RefreshProjectStatus();
        SiteActionError(project->id,"Native terrain writer became unavailable. The Site paused for review.");return;
    }
    __try { openHeightmap(terrain); } __except(EXCEPTION_EXECUTE_HANDLER) {
        host->log("earthworks  site=%llu paused for review: opening terrain writer raised exception=0x%08lX",
            static_cast<unsigned long long>(project->id),GetExceptionCode());
        project->status=ProjectStatus::Paused;project->needsReview=true;project->allocatedRateUnits=0;project->allocatedMachines=0;project->allocatedWorkers=0;projectSchedulerDirty=true;RefreshProjectStatus();SiteActionError(project->id,"Terrain writer failed to open. The Site paused for review.");return;
    }
    double usedVolume=0;int edited=0;bool failed=false;const char* failureStage="";Point failurePoint{};
    for(uint32_t scan=0;scan<static_cast<uint32_t>(count)&&edited<batch;++scan) {
        const uint32_t linear=(project->executionCursor++)%static_cast<uint32_t>(count);
        const int x=rect.x0+static_cast<int>(linear%static_cast<uint32_t>(rect.x1-rect.x0+1));
        const int z=rect.z0+static_cast<int>(linear/static_cast<uint32_t>(rect.x1-rect.x0+1));
        const auto point=grid.At(x,z);float currentHeight=0,targetHeight=0;
        if(!project->plan.Contains(point.x,point.z)||!project->plan.BlendedTargetAt(point.x,point.z,std::min(grid.stepX,grid.stepZ),&targetHeight))continue;
        if(!ReadHeight(point.x,point.z,&currentHeight)) {failed=true;failureStage="read-before-edit";failurePoint=point;break;}
        const float difference=targetHeight-currentHeight;
        if(std::abs(difference)<=precision)continue;
        const float delta=std::copysign(std::min(std::abs(difference),step),difference);
        if(!ApplyProjectVertex(terrain,grid,x,z,delta)) {failed=true;failureStage="apply-edit";failurePoint=point;break;}
        float afterHeight=0;
        if(!ReadHeight(point.x,point.z,&afterHeight)) {failed=true;failureStage="read-after-edit";failurePoint=point;break;}
        const double applied=std::abs(static_cast<double>(afterHeight-currentHeight));
        usedVolume+=applied*cellArea;if(applied>0)++edited;
    }
    __try { closeHeightmap(terrain); } __except(EXCEPTION_EXECUTE_HANDLER) {
        failed=true;failureStage="close-writer";
    }
    if(failed) {
        host->log("earthworks  site=%llu paused for review: terrain operation=%s at (%.3f,%.3f)",
            static_cast<unsigned long long>(project->id),failureStage,failurePoint.x,failurePoint.z);
        project->status=ProjectStatus::Paused;project->needsReview=true;project->allocatedRateUnits=0;project->allocatedMachines=0;project->allocatedWorkers=0;projectSchedulerDirty=true;RefreshProjectStatus();
        SiteActionError(project->id,"Terrain edit/readback failed. The Site paused for review; inspect the area before resuming.");return;
    }
    project->workRemainderM3=static_cast<float>(std::max(0.0,static_cast<double>(project->workRemainderM3)-usedVolume));
    if(project->totalWorkM3<=1e-6) {
        double remaining=0;
        if(MeasureTerrainWork(project->plan,grid,rect,&remaining)&&remaining>1e-6) {
            const double fractionRemaining=std::max(1e-6,1.0-static_cast<double>(project->progress));
            project->totalWorkM3=static_cast<float>(std::min(1.0e12,remaining/fractionRemaining));
            host->log("earthworks  site=%llu initialized legacy progress denominator=%.3f m3",
                static_cast<unsigned long long>(project->id),project->totalWorkM3);
        }
    }
    const double total=project->totalWorkM3;
    if(total>0)project->progress=std::clamp(project->progress+static_cast<float>(usedVolume/total),0.0f,1.0f);
    const auto now=GetTickCount64();
    if(now-lastCompletionCheck>=500) {
        lastCompletionCheck=now;bool complete=true;
        for(int z=rect.z0;z<=rect.z1 && complete;++z)for(int x=rect.x0;x<=rect.x1;++x) {
            const auto point=grid.At(x,z);float value=0,targetHeight=0;
            if(!project->plan.Contains(point.x,point.z)||!project->plan.BlendedTargetAt(point.x,point.z,std::min(grid.stepX,grid.stepZ),&targetHeight))continue;
            if(!ReadHeight(point.x,point.z,&value) || std::abs(targetHeight-value)>precision*1.1) {complete=false;break;}
        }
        if(complete) {project->progress=1;project->status=ProjectStatus::Completed;project->allocatedRateUnits=0;project->allocatedMachines=0;project->allocatedWorkers=0;projectSchedulerDirty=true;Message("Earthworks Site completed.");}
        else if(edited)current.projectProgress=project->progress;
    } else if(edited)current.projectProgress=project->progress;
}
void TickProjectRunner()
{
    if(!current.ready)return;
    const double clockHour=SimulationHourClock(current);
    double simulationElapsed=0;
    if(lastSimulationHour<0)lastSimulationHour=clockHour;
    else {
        const double elapsed=clockHour-lastSimulationHour;
        lastSimulationHour=clockHour;
        if(std::isfinite(elapsed)&&elapsed>0)simulationElapsed=std::min(elapsed,1.0);
    }
    // Rebalance after scheduling changes even when the simulation is paused.
    // Terrain work itself remains tied to positive simulation time.
    if(simulationElapsed<=0&&!projectSchedulerDirty)return;
    projectSchedulerDirty=false;
    struct Allocation { SavedEarthwork* project=nullptr;double rate=0;unsigned machines=0; };
    std::vector<uint64_t> processedProjects;
    std::vector<Allocation> allocations;
    bool schedulerStateChanged=false;
    allocations.reserve(durableState.earthworks.size());
    for(auto& first:durableState.earthworks) {
        if(first.status!=ProjectStatus::Running&&first.status!=ProjectStatus::Waiting)continue;
        if(std::find(processedProjects.begin(),processedProjects.end(),first.projectId)!=processedProjects.end())continue;
        processedProjects.push_back(first.projectId);
        const auto* namedProject=FindNamedProject(first.projectId);
        std::vector<SavedEarthwork*> jobs;
        for(auto& site:durableState.earthworks)
            if(site.projectId==first.projectId&&(site.status==ProjectStatus::Running||site.status==ProjectStatus::Waiting))jobs.push_back(&site);
        if(!namedProject||namedProject->facilityIds.empty()) {
            for(auto* job:jobs) {
                job->status=ProjectStatus::Paused;job->needsReview=false;schedulerStateChanged=true;
                SiteActionError(job->id,"This Site paused because its Project has no assigned Earthworks facility.");
            }
            continue;
        }
        std::vector<double> availableMachineRateUnits;
        bool facilityReadFailed=false;uint32_t failedFacilityId=0;WorkforceView failedFacility{};
        for(const auto facilityId:namedProject->facilityIds) {
            WorkforceView workforce{};
            if(!ReadEarthworksFacilityWorkforce(facilityId,&workforce)||!workforce.buildingValid
                ||!workforce.parkedValid||!workforce.vehiclesValid) {
                facilityReadFailed=true;failedFacilityId=facilityId;failedFacility=workforce;break;
            }
            for(unsigned i=0;i<workforce.rowCount;++i) {
                const auto& vehicle=workforce.vehicles[i];
                if(vehicle.parked&&vehicle.earthworksAllowed&&vehicle.groundworksMachine
                    &&vehicle.displayedLevelValid&&vehicle.constructionRateUnits>0) {
                    double contribution=1.0;
                    if(DistanceBasedFacilityContribution()) {
                        const Point center=jobs.empty()?Point{}:SiteCenter(jobs.front()->plan);
                        const double distance=std::hypot(static_cast<double>(center.x)-workforce.building.position.x,
                            static_cast<double>(center.z)-workforce.building.position.z);
                        contribution=FacilityDistanceContribution(distance);
                    }
                    availableMachineRateUnits.push_back(static_cast<double>(vehicle.constructionRateUnits)*contribution);
                }
            }
        }
        if(facilityReadFailed) {
            bool newlyWaiting=false;
            for(auto* job:jobs) {
                newlyWaiting|=job->status!=ProjectStatus::Waiting||job->needsReview;
                job->status=ProjectStatus::Waiting;job->needsReview=false;schedulerStateChanged=true;
            }
            host->log("earthworks  project=%llu waiting: facility=%u reading unavailable building=%d parked=%d vehicles=%d; terrain review not required",
                static_cast<unsigned long long>(first.projectId),failedFacilityId,failedFacility.buildingValid,
                failedFacility.parkedValid,failedFacility.vehiclesValid);
            if(newlyWaiting)for(auto* job:jobs)
                SiteActionError(job->id,"Assigned facility readings are temporarily unavailable. This Site will wait and retry automatically.");
            continue;
        }
        if(jobs.empty())continue;
        std::sort(availableMachineRateUnits.begin(),availableMachineRateUnits.end(),std::greater<double>());
        size_t machineCursor=0;
        std::vector<Allocation> projectAllocations;
        projectAllocations.reserve(jobs.size());
        for(auto* job:jobs) {
            job->allocatedRateUnits=0;
            projectAllocations.push_back({job,0,0});
        }
        if(machineCursor<availableMachineRateUnits.size()&&!projectAllocations.empty()) {
            // FIFO within this named Project: the first started, eligible Site
            // in the persisted priority order receives the full machine pool.
            // Lower-priority started Sites remain Waiting until it completes.
            // Paused and review-blocked Sites are not in `jobs`, so they are
            // skipped and the next eligible started Site receives the pool.
            auto& allocation=projectAllocations.front();
            while(machineCursor<availableMachineRateUnits.size()) {
                const auto rateUnits=availableMachineRateUnits[machineCursor++];
                allocation.rate+=rateUnits;
                ++allocation.machines;
                allocation.project->allocatedRateUnits+=rateUnits;
            }
        }
        for(auto& allocation:projectAllocations) {
            auto* job=allocation.project;
            job->allocatedWorkers=0;job->allocatedMachines=allocation.machines;
            const auto next=allocation.rate>0?ProjectStatus::Running:ProjectStatus::Waiting;
            if(job->status!=next) {
                job->status=next;schedulerStateChanged=true;
                host->log("earthworks  site=%llu scheduler state=%s project=%llu facilities=%zu rate=%.3f",
                    static_cast<unsigned long long>(job->id),next==ProjectStatus::Running?"running":"waiting",
                    static_cast<unsigned long long>(job->projectId),namedProject->facilityIds.size(),allocation.rate);
            }
            allocations.push_back(allocation);
        }
    }
    bool changed=false;
    std::vector<uint64_t> completedSiteIds;
    for(const auto& allocation:allocations)if(simulationElapsed>0&&allocation.project&&allocation.project->status==ProjectStatus::Running&&allocation.rate>0) {
        const auto before=allocation.project->status;
        ExecuteProjectSlice(allocation.project,allocation.rate,simulationElapsed);
        if(before!=allocation.project->status) {
            changed=true;
            if(allocation.project->status==ProjectStatus::Completed)completedSiteIds.push_back(allocation.project->id);
        }
    }
    for(const auto siteId:completedSiteIds)MoveCompletedSiteToProjectEnd(siteId);
    if(changed||schedulerStateChanged||!allocations.empty()||projectSchedulerDirty)RefreshProjectStatus();
}
void Tick()
{
    const bool wasReady = current.ready;
    if (!ReadNative(&current)) { current.ready = false; current.cursorValid = false; current.cameraValid = false; }
    if (wasReady && !current.ready) ResetWorld();
    UpdateTerrainGridSnapshot();
    const auto reconcileNow=GetTickCount64();
    if(current.ready&&reconcileNow-lastFacilityIdentityReconcile>=1000) {
        lastFacilityIdentityReconcile=reconcileNow;
        ReconcileSavedFacilities();
    }
    if (!wasReady && current.ready) {
        if(loadMessagePending) loadMessagePending=false;
        else current.message[0]=0;
        host->log("earthworks  world ready generation=%llu date=%d/%d cursor=(%.2f,%.2f,%.2f)",
            current.generation,current.day,current.year,current.cursor.x,current.cursor.y,current.cursor.z);
    }
    std::deque<Command> pending;
    { std::lock_guard<std::mutex> lock(dataMutex); pending.swap(commands); }
    for (const auto& command:pending) {
        const Plan before=current.plan;
        Process(command);
        if (before.complete && command.action<=Action::ReviewResumeProject) {
            Plan check=before;
            if (SamplePlan(&check)) {
                float difference=0;
                for(size_t i=0;i<check.ground.size();++i) difference=std::max(difference,std::abs(check.ground[i]-before.ground[i]));
                host->log("earthworks  terrain readback: samples=%d maxDelta=%.6f after action=%d",GridSide*GridSide,difference,static_cast<int>(command.action));
                if (difference>TerrainReadbackToleranceMeters) Message("Ground changed since selection. Select the area again to refresh this preview.");
            } else Message("Ground readback unavailable. Select the area again before relying on this preview.");
        }
    }
    current.canUndo = history.CanUndo(); current.canRedo = history.CanRedo();
    if(current.ready)TickWorkforce();
    if(current.ready)TickProjectRunner();
    UpdateEditorLiveVolumes();
    const auto facilityNow=GetTickCount64();
    if(current.projectQueued && current.projectFacilityId && facilityNow-lastFacilityRefresh>=500) {
        lastFacilityRefresh=facilityNow;RefreshProjectStatus();
    }
    current.workforce=ReadWorkforce();
    UpdateSiteDraftValidation();
    current.facilityAssignmentWarning=facilityAssignmentWarningPending;
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
        facilityIdentityCheckAfterLoad=true;
        lastFacilityIdentityReconcile=0;lastUnresolvedFacilityIdentityCount=static_cast<size_t>(-1);
        ReconcileSavedFacilities();
        current.plan=durableState.draft;
        RefreshProjectStatus();
        if(current.projectFacilityId)SelectEarthworksFacility(current.projectFacilityId);
        ++current.revision;
        Message("Restored saved Earthworks drafts and projects.");
        loadMessagePending=true;
        host->log("earthworks  companion state restored after terrain fingerprint matched");
    } else if(loadResult==StateLoadResult::TerrainMismatch || loadResult==StateLoadResult::Corrupt) {
        Message(loadResult==StateLoadResult::TerrainMismatch
            ? "Saved Earthworks data belongs to different terrain. No plans were restored; terrain was not changed."
            : "Saved Earthworks data is damaged or from an older WIP version. No plans were restored; terrain was not changed.");
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
std::atomic<bool> loggedDirectInputKeyboardHook{false};
using DirectInput8CreateFn = HRESULT (WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
using DirectInputCreateDeviceFn = HRESULT (STDMETHODCALLTYPE*)(void*,const GUID*,void**,IUnknown*);
using DirectInputGetDeviceStateFn = HRESULT (STDMETHODCALLTYPE*)(void*,DWORD,LPVOID);
using DirectInputGetDeviceDataFn = HRESULT (STDMETHODCALLTYPE*)(void*,DWORD,void*,DWORD*,DWORD);
DirectInput8CreateFn directInput8Create=nullptr;
DirectInputCreateDeviceFn directInputCreateDevice=nullptr;
DirectInputGetDeviceStateFn directInputGetDeviceState=nullptr;
DirectInputGetDeviceDataFn directInputGetDeviceData=nullptr;
constexpr size_t DirectInput8CreateDeviceSlot=3;
constexpr size_t DirectInputDeviceStateSlot=9;
constexpr size_t DirectInputDeviceDataSlot=10;
constexpr size_t DirectInput8VtableCount=11;
constexpr size_t DirectInputDeviceVtableCount=32;
struct DirectInputDeviceData { DWORD offset,data,timeStamp,sequence; ULONG_PTR applicationData; };
bool IsDigitScanCode(DWORD code)
{
    return (code>=0x02&&code<=0x0B)
        || (code>=0x47&&code<=0x49)||(code>=0x4B&&code<=0x4D)
        || (code>=0x4F&&code<=0x53);
}
bool CloneAndPatchVtable(void* instance,size_t entries,size_t slot,void* detour,void** original)
{
    if(!instance||slot>=entries||!detour||!original)return false;
    auto** table=*reinterpret_cast<void***>(instance);
    if(!table)return false;
    if(table[slot]==detour)return true;
    auto** copy=static_cast<void**>(HeapAlloc(GetProcessHeap(),0,entries*sizeof(void*)));
    if(!copy)return false;
    std::memcpy(copy,table,entries*sizeof(void*));
    *original=copy[slot];
    copy[slot]=detour;
    *reinterpret_cast<void***>(instance)=copy;
    return *original!=nullptr;
}
HRESULT STDMETHODCALLTYPE HDirectInputGetDeviceState(void* self,DWORD size,LPVOID data)
{
    const HRESULT result=directInputGetDeviceState(self,size,data);
    if(SUCCEEDED(result)&&keyboardCapture&&data&&size>=256) {
        auto* keys=static_cast<BYTE*>(data);
        for(DWORD code=0;code<256;++code)if(IsDigitScanCode(code))keys[code]=0;
    }
    return result;
}
HRESULT STDMETHODCALLTYPE HDirectInputGetDeviceData(void* self,DWORD recordSize,void* data,DWORD* count,DWORD flags)
{
    const HRESULT result=directInputGetDeviceData(self,recordSize,data,count,flags);
    if(FAILED(result)||!keyboardCapture||!data||!count||recordSize!=sizeof(DirectInputDeviceData))return result;
    auto* records=static_cast<DirectInputDeviceData*>(data);
    DWORD write=0;
    for(DWORD read=0;read<*count;++read) {
        if(IsDigitScanCode(records[read].offset))continue;
        if(write!=read)records[write]=records[read];
        ++write;
    }
    *count=write;
    return result;
}
HRESULT STDMETHODCALLTYPE HDirectInputCreateDevice(void* self,const GUID* guid,void** device,IUnknown* outer)
{
    const HRESULT result=directInputCreateDevice(self,guid,device,outer);
    static const GUID systemKeyboard={0x6F1D2B61,0xD5A0,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
    if(SUCCEEDED(result)&&guid&&device&&*device&&std::memcmp(guid,&systemKeyboard,sizeof(GUID))==0) {
        auto*** table=reinterpret_cast<void***>(*device);
        if(table&&*table) {
            auto** vtable=*table;
            bool stateHooked=vtable[DirectInputDeviceStateSlot]==reinterpret_cast<void*>(HDirectInputGetDeviceState);
            bool dataHooked=vtable[DirectInputDeviceDataSlot]==reinterpret_cast<void*>(HDirectInputGetDeviceData);
            if(vtable[DirectInputDeviceStateSlot]!=reinterpret_cast<void*>(HDirectInputGetDeviceState)) {
                stateHooked=CloneAndPatchVtable(*device,DirectInputDeviceVtableCount,DirectInputDeviceStateSlot,
                    reinterpret_cast<void*>(HDirectInputGetDeviceState),reinterpret_cast<void**>(&directInputGetDeviceState));
            }
            if(vtable[DirectInputDeviceDataSlot]!=reinterpret_cast<void*>(HDirectInputGetDeviceData)) {
                dataHooked=CloneAndPatchVtable(*device,DirectInputDeviceVtableCount,DirectInputDeviceDataSlot,
                    reinterpret_cast<void*>(HDirectInputGetDeviceData),reinterpret_cast<void**>(&directInputGetDeviceData));
            }
            if(!loggedDirectInputKeyboardHook.exchange(true))
                host->log("earthworks  DirectInput keyboard device filter: state=%d buffered=%d",
                    static_cast<int>(stateHooked),static_cast<int>(dataHooked));
        }
    }
    return result;
}
HRESULT WINAPI HDirectInput8Create(HINSTANCE instance,DWORD version,REFIID iid,LPVOID* out,LPUNKNOWN outer)
{
    const HRESULT result=directInput8Create(instance,version,iid,out,outer);
    if(SUCCEEDED(result)&&out&&*out)
        CloneAndPatchVtable(*out,DirectInput8VtableCount,DirectInput8CreateDeviceSlot,
            reinterpret_cast<void*>(HDirectInputCreateDevice),reinterpret_cast<void**>(&directInputCreateDevice));
    return result;
}
#define BOOL_HOOK(name,original,block) bool name(void* self) { const bool value=original(self); return (block) ? false:value; }
BOOL_HOOK(HLeftPress,leftPress,LeftTaken())
BOOL_HOOK(HLeftAOff,leftAOff,LeftTaken())
BOOL_HOOK(HLeftDown,leftDown,LeftTaken())
BOOL_HOOK(HRightPress,rightPress,mouseCapture.load())
BOOL_HOOK(HRightAOff,rightAOff,mouseCapture.load())
BOOL_HOOK(HRightDown,rightDown,mouseCapture.load())
BOOL_HOOK(HMiddleDown,middleDown,mouseCapture.load())
#undef BOOL_HOOK
bool HKeyDown(void* self,int key) { const bool value=keyDown(self,key); return keyboardCapture || escapeCapture ? false:value; }
bool HKeyPress(void* self,int key) { const bool value=keyPress(self,key); return keyboardCapture || escapeCapture ? false:value; }
bool HButtonDown(void* self,char* binding) { const bool value=buttonDown(self,binding); return keyboardCapture || escapeCapture ? false:value; }
bool HButtonPress(void* self,char* binding) { const bool value=buttonPress(self,binding); return keyboardCapture || escapeCapture ? false:value; }
float HAxis(void* self,char* binding,int* out,float a,float b) { const float value=axis(self,binding,out,a,b); return keyboardCapture || escapeCapture || GetTickCount64()<=tableWheelCaptureUntilMs.load(std::memory_order_acquire) ? 0:value; }
float* HRelative(void* self,float* out) { auto* value=relative(self,out); if (mouseCapture||GetTickCount64()<=tableWheelCaptureUntilMs.load(std::memory_order_acquire)) out[0]=out[1]=out[2]=0; return value; }
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
// Map-pick mode itself must not blanket-block all native left clicks. The
// window procedure sets leftRelease only after it has classified a click as
// belonging to Earthworks; clicks over native UI must still reach the game.
bool LeftTaken() { return mouseCapture || leftRelease; }
void ClearInputLatches()
{
    __try {
        if (LeftTaken()) { host->exeBase[0xA54E90]=0; host->exeBase[0xA54E91]=0; }
        if (keyboardCapture) host->exeBase[0xA54C91]=0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { }
}
const Snapshot& ReadSnapshot()
{
    // Snapshots contain several large bounded tables. Reuse per-thread storage
    // instead of making a large automatic object (or a by-value return) in the
    // game render and window callbacks, whose stacks are limited.
    static thread_local std::unique_ptr<Snapshot> storage;
    if (!storage) storage = std::make_unique<Snapshot>();
    auto& out = *storage;
    const bool available=AvailabilitySignal();
    std::lock_guard<std::mutex> lock(dataMutex);
    out=published;
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
    if (!heightQuery) { host->log("earthworks  native preflight failed: terrain-height export unavailable; planning disabled"); return false; }
    if (host->exeSize < 0xA54E92 || host->exeSize < LateRva+sizeof(lateBytes)
        || host->exeSize < LoadRva+sizeof(loadBytes)
        || host->exeSize < ConstructionAssignRva+sizeof(constructionAssignBytes)
        || host->exeSize < VehicleSlotRva+sizeof(vehicleSlotBytes)) {
        host->log("earthworks  native preflight failed: game image is smaller than required bindings; planning disabled"); return false;
    }
    if (!host->findIatSlot || !host->patchIat || !host->installInlineHook) {
        host->log("earthworks  native preflight failed: Tesmio hook API unavailable; planning disabled"); return false;
    }
    if (std::memcmp(host->exeBase+LateRva,lateBytes,sizeof(lateBytes)) != 0) {
        host->log("earthworks  native preflight failed: frame hook mismatch at EXE+0x%llX; planning disabled",
            static_cast<unsigned long long>(LateRva)); return false;
    }
    if (std::memcmp(host->exeBase+LoadRva,loadBytes,sizeof(loadBytes)) != 0) {
        host->log("earthworks  native preflight failed: world-load hook mismatch at EXE+0x%llX; planning disabled",
            static_cast<unsigned long long>(LoadRva)); return false;
    }
    if (std::memcmp(host->exeBase+ConstructionAssignRva,constructionAssignBytes,sizeof(constructionAssignBytes)) != 0) {
        host->log("earthworks  native preflight failed: construction-assignment hook mismatch at EXE+0x%llX; planning disabled",
            static_cast<unsigned long long>(ConstructionAssignRva)); return false;
    }
    if (std::memcmp(host->exeBase+VehicleSlotRva,vehicleSlotBytes,sizeof(vehicleSlotBytes)) != 0) {
        host->log("earthworks  native preflight failed: vehicle-slot hook mismatch at EXE+0x%llX; planning disabled",
            static_cast<unsigned long long>(VehicleSlotRva)); return false;
    }
    for (const auto& item:imports) if (!host->findIatSlot(host->exeModule,"C3DDLL64.dll",item.symbol)) {
        host->log("earthworks  missing input import %s; planning disabled",item.symbol); return false;
    }
    if(!host->findIatSlot(host->engineModule,"dinput8.dll","DirectInput8Create")) {
        host->log("earthworks  missing engine DirectInput8Create import; planning disabled"); return false;
    }
    for (const auto& item:imports) if (!host->patchIat(host->exeModule,"C3DDLL64.dll",item.symbol,item.detour,item.original,"Earthworks input capture")) return false;
    if(!host->patchIat(host->engineModule,"dinput8.dll","DirectInput8Create",reinterpret_cast<void*>(HDirectInput8Create),
        reinterpret_cast<void**>(&directInput8Create),"Earthworks DirectInput digit capture"))return false;
    if (!host->installInlineHook(host->exeBase+LateRva,reinterpret_cast<void*>(HookedLate),reinterpret_cast<void**>(&originalLate),lateBytes,sizeof(lateBytes),"Earthworks read-only frame")) return false;
    if (!host->installInlineHook(host->exeBase+LoadRva,reinterpret_cast<void*>(HookedLoad),reinterpret_cast<void**>(&originalLoad),loadBytes,sizeof(loadBytes),"Earthworks world boundary")) return false;
    if (!host->installInlineHook(host->exeBase+ConstructionAssignRva,reinterpret_cast<void*>(HookedConstructionAssign),reinterpret_cast<void**>(&originalConstructionAssign),constructionAssignBytes,sizeof(constructionAssignBytes),"Earthworks office assignment filter")) return false;
    if (!host->installInlineHook(host->exeBase+VehicleSlotRva,reinterpret_cast<void*>(HookedVehicleSlot),reinterpret_cast<void**>(&originalVehicleSlot),vehicleSlotBytes,sizeof(vehicleSlotBytes),"Earthworks vehicle slot filter")) return false;
    host->log("earthworks  bindings installed: 13 C3D input imports, DirectInput digit filter, frame, world-load, office assignment, and vehicle slot filter");
    BindNativeTerrainEditHook();
    host->log("earthworks  terrain executor bindings %s",raiseLower&&heightQuery&&openHeightmap&&closeHeightmap?"available":"unavailable; projects remain queued");
    return true;
}
}
