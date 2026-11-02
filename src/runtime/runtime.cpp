// SPDX-License-Identifier: GPL-3.0-only
// Native binding addresses, input interception and camera layout adapted from
// RepublicMP (E3418), commit 630775724abd5e3346eb59f9c66e2873128d74a4.
#include <windows.h>
#include "runtime.h"
#include "save_store.h"
#include "vehicle_filter.h"
#include "native_terrain_probe.h"
#include "../core/productivity.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>
#include <mutex>
#include <utility>

namespace ew {
const TsmHost* host = nullptr;
std::atomic<bool> panelOpen{false}, mouseCapture{false}, keyboardCapture{false}, leftRelease{false};
namespace {
std::mutex dataMutex;
Snapshot published, current;
std::deque<Command> commands;
History history;
DurableState durableState;
bool savePending = false;
bool loadMessagePending = false;
ULONGLONG lastFacilityRefresh = 0;
using LateFn = void (*)(__int64);
using LoadFn = void (*)();
using HeightFn = float (*)(void*, const float*);
LateFn originalLate = nullptr;
LoadFn originalLoad = nullptr;
HeightFn heightQuery = nullptr;
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
bool AvailabilitySignal()
{
    // These two fixed-address scalars outlive worlds. Late stops running in
    // menus, so the renderer also gates snapshots on this lifecycle signal.
    // No terrain/camera object is followed or queried from the renderer.
    __try { return host->exeBase[0x993C65] != 0 && *reinterpret_cast<void**>(host->exeBase+0x9E46B0) != nullptr; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void Message(const char* text) { strcpy_s(current.message, text); }
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
    durableState = DurableState{}; savePending = false; loadMessagePending = false;
    history.Reset(); panelOpen = false; mouseCapture = false; keyboardCapture = false;
    ProbeWorldBoundary();
    ResetWorkforce();
    std::lock_guard<std::mutex> lock(dataMutex);
    commands.clear(); published = current;
    host->log("earthworks  world boundary; discarded session preview, generation=%llu", generation);
}
void RefreshProjectStatus()
{
    current.projectQueued=false;
    current.projectId=0;
    current.projectFacilityId=0;
    current.assignedFacilityFound=false;
    current.projectFacilityDistance=0;
    current.queuedPlan=Plan{};
    for (const auto& project : durableState.projects) {
        if (project.status==ProjectStatus::Queued) {
            current.projectQueued=true; current.projectId=project.id; current.queuedPlan=project.plan; break;
        }
    }
    for(const auto& project:durableState.projects)if(project.status==ProjectStatus::Queued) {
        current.projectFacilityId=project.facilityId;
        if(project.facilityId) {
            BuildingReading facility{};
            if(FindEarthworksFacility(project.facilityId,&facility)) {
                current.assignedFacilityFound=true;
                const Point center{(project.plan.a.x+project.plan.b.x)*0.5f,0,(project.plan.a.z+project.plan.b.z)*0.5f};
                current.projectFacilityDistance=std::hypot(center.x-facility.position.x,center.z-facility.position.z);
            }
        }
        break;
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
            Message("Choose the Earthworks facility in the inspector, then assign it to the queued project.");break;
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
        history.Commit(current.plan); current.plan = Plan{}; current.mode = PickMode::Corners;
        Changed(); Message("Preview cleared. Click the first corner."); break;
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
        if(!durableState.projects.empty()) { Message("Stage 6 supports one queued project. Complete or remove it in a later stage before adding another."); break; }
        SavedProject project;
        project.id=1;
        project.status=ProjectStatus::Queued;
        project.plan=current.plan;
        durableState.projects.push_back(project);
        RefreshProjectStatus();
        current.plan=Plan{};
        current.mode=PickMode::Corners;
        history.Reset();
        Changed();
        host->log("earthworks  project confirmed and queued id=%llu; Stage 7 dry-run only, no terrain execution",project.id);
        Message("Project queued. Assign an Earthworks facility to review its dry-run capacity.");
        break;
    }
    case Action::AssignProject: {
        if(!current.projectQueued) { Message("Queue a project before assigning a facility."); break; }
        const auto workforce=ReadWorkforce();
        if(!workforce.selected || !workforce.buildingValid || !workforce.building.earthworksFacility) {
            Message("Select a recognized Earthworks facility in the inspector first.");break;
        }
        for(auto& project:durableState.projects)if(project.id==current.projectId && project.status==ProjectStatus::Queued) {
            project.facilityId=workforce.building.id;RefreshProjectStatus();
            SelectEarthworksFacility(project.facilityId);
            Message("Project assigned. Dry-run only: parked equipment is shown as candidates, not reserved.");
            break;
        }
        break;
    }
    case Action::ClearProjectAssignment:
        for(auto& project:durableState.projects)if(project.id==current.projectId && project.status==ProjectStatus::Queued)project.facilityId=0;
        RefreshProjectStatus();Message("Facility assignment cleared. The queued project remains saved.");break;
    case Action::CaptureTest: ProcessProbe(ProbeAction::Capture); break;
    case Action::RaiseTest: ProcessProbe(ProbeAction::Raise); break;
    case Action::LowerTest: ProcessProbe(ProbeAction::Lower); break;
    case Action::RecheckTest: ProcessProbe(ProbeAction::Recheck); break;
    case Action::InspectBuilding:
        current.mode=PickMode::InspectBuilding;Message("Click near the facility on the map, then choose its Earthworks facility result in the inspector.");break;
    case Action::ChooseBuilding:
        if(std::isfinite(command.value) && command.value>=0 && command.value<WorkforceCandidates)
            SelectWorkforceBuilding(static_cast<unsigned>(command.value));
        break;
    case Action::ClearInspector: ResetWorkforce();current.mode=PickMode::None;break;
    case Action::LogInspector: LogWorkforce();break;
    }
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
bool LeftTaken() { return panelOpen || mouseCapture || leftRelease; }
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
    heightQuery = reinterpret_cast<HeightFn>(GetProcAddress(static_cast<HMODULE>(host->engineModule),
        "?CollisionTerrainHeightOverPoint@C3D_TERRAIN@@QEAAMVC3DVECTOR3@@@Z"));
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
    return true;
}
}
