// SPDX-License-Identifier: GPL-3.0-only
// Layout evidence: pinned RepublicMP game/world.h, determinism/state_hash.cpp,
// lockstep/call_home.cpp, lockstep/world_refs.cpp and lockstep/purchase.cpp.
// Observations only: no native calls, writes, reservations or availability rules.
#include <windows.h>
#include "runtime.h"
#include "workforce_probe.h"
#include "vehicle_filter.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace ew {
namespace {
using Address=uintptr_t;
struct List { Address begin=0; unsigned count=0; };
struct Identity { unsigned slot=0,id=0; Address address=0; };
WorkforceView view;
std::array<Identity,WorkforceCandidates> candidates;
Identity selected;
unsigned blockedAssignments=0;
ULONGLONG lastSample=0;
template<class T> bool Read(Address address,T* out)
{
    __try { *out=*reinterpret_cast<const T*>(address); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool PointerList(Address address,unsigned limit,List* out)
{
    Address begin=0,end=0;
    if(!Read(address,&begin) || !Read(address+8,&end) || end<begin ||
        (end-begin)%sizeof(Address)!=0 || (end-begin)/sizeof(Address)>limit ||
        (begin==0 && end!=0) || begin%alignof(Address)!=0) return false;
    out->begin=begin;out->count=static_cast<unsigned>((end-begin)/sizeof(Address));return true;
}
bool Element(const List& list,unsigned index,Address* out)
{ return index<list.count && Read(list.begin+sizeof(Address)*index,out); }
Address World() { return reinterpret_cast<Address>(host->exeBase)+0x9D4F10; }
bool Position(Address address,Point* out)
{ return Read(address,out) && std::isfinite(out->x) && std::isfinite(out->y) && std::isfinite(out->z)
    && std::abs(out->x)<100000 && std::abs(out->z)<100000; }
float Distance(Point a,Point b) { return std::hypot(a.x-b.x,a.z-b.z); }
bool DescribeBuilding(Address address,unsigned slot,BuildingReading* out)
{
    Address type=0;
    out->slot=slot;
    if(!address || !Read(address,&out->id) || !Position(address+0x350,&out->position)
        || !Read(address+0x318,&type) || !type || !Read(type+0x360,&out->category) || !Read(type+0x364,&out->subtype))return false;
    for(unsigned i=0;i+1<sizeof(out->assetName);++i) {
        char c=0;if(!Read(type+i,&c))return false;
        if(c==0)break;
        if(static_cast<unsigned char>(c)<32 || static_cast<unsigned char>(c)>126)return false;
        out->assetName[i]=c;
    }
    out->earthworksFacility=std::strcmp(out->assetName,"9100000012/EarthworksDevelopmentFacility")==0;
    return true;
}
bool Resolve(const Identity& identity,Address* address)
{
    List buildings;unsigned id=0;
    return PointerList(World()+0x11B08,262144,&buildings) && Element(buildings,identity.slot,address)
        && *address && *address==identity.address && Read(*address,&id) && id==identity.id;
}
bool Vehicle(Address address,Address building,const Address* parked,unsigned parkedCount,Point origin,VehicleReading* out)
{
    Address type=0,home=0,assigned=0;
    if(!Read(address+4,&out->id) || !Read(address+0x1708,&type) || !type ||
        !Read(type+0x294,&out->category) || !Read(address+0x4F0,&home) || !Read(address+0x4F8,&assigned)) return false;
    out->earthworksAllowed=IsEarthworksVehicleType(reinterpret_cast<void*>(type));
    out->groundworksMachine=IsGroundworksMachineType(reinterpret_cast<void*>(type));
    out->home=home==building;out->assigned=assigned==building;
    out->parked=std::binary_search(parked,parked+parkedCount,address);
    if(out->groundworksMachine) {
        // Pinned RepublicMP determinism/office_guard.cpp reads these fields
        // when filtering native mechanism candidates. They are observations,
        // not a reservation or a complete Earthworks availability predicate.
        out->nativeJobStateValid=Read(address+0x698,&out->nativeJobState);
        List route;
        out->routeValid=PointerList(address+0x680,4096,&route);
        if(out->routeValid) {
            out->routeStopCount=route.count;
            if(route.count) {
                Address first=0;
                out->routeValid=Element(route,0,&first);
                out->firstRouteStopIsFacility=out->routeValid && first==building;
            }
        }
    }
    for(unsigned i=0;i<64;++i) {
        char c=0;if(!Read(type+0x200+i,&c)) return false;
        if(c==0) break;out->type[i]=(c>=32 && c<=126)?c:'?';
    }
    Point position;
    out->positionValid=Position(address+0xDE0,&position);
    if(out->positionValid)out->distance=Distance(position,origin);
    return true;
}
void Sample()
{
    Address building=0;
    if(!Resolve(selected,&building)) {
        view=WorkforceView{};
        strcpy_s(view.status,"Building identity no longer resolves (removed, changed or unreadable). Select again; replacements are never followed automatically.");
        return;
    }
    WorkforceView next;
    next.selected=true;next.samples=view.samples+1;
    if(!DescribeBuilding(building,selected.slot,&next.building)) {
        view=next;strcpy_s(view.status,"Building fields unreadable. No staffing or equipment conclusions are available.");return;
    }
    next.buildingValid=true;
    List occupants,parked,vehicles;
    next.occupantsValid=PointerList(building+0xBC0,65536,&occupants);
    if(next.occupantsValid) {
        next.occupantSlots=occupants.count;
        for(unsigned i=0;i<occupants.count;++i) {
            Address item=0;if(!Element(occupants,i,&item)){next.occupantsValid=false;break;}
            if(item)++next.occupants;
        }
    }
    next.parkedValid=PointerList(building+0xC70,4096,&parked);
    std::array<Address,4096> parkedAddresses{};
    if(next.parkedValid) {
        next.parkedSlots=parked.count;
        for(unsigned i=0;i<parked.count;++i)if(!Element(parked,i,&parkedAddresses[i])){next.parkedValid=false;break;}
        if(next.parkedValid)std::sort(parkedAddresses.begin(),parkedAddresses.begin()+parked.count);
    }
    next.vehiclesValid=PointerList(World()+0x12810,65536,&vehicles);
    if(next.vehiclesValid)for(unsigned i=0;i<vehicles.count;++i) {
        Address address=0;
        if(!Element(vehicles,i,&address)){next.vehiclesValid=false;break;}
        if(!address)continue;
        VehicleReading row;
        if(!Vehicle(address,building,parkedAddresses.data(),next.parkedValid?parked.count:0,next.building.position,&row)) {++next.unreadableVehicles;continue;}
        if(!row.home && !row.assigned && !row.parked)continue;
        next.homeCount+=row.home?1u:0u;next.assignedCount+=row.assigned?1u:0u;next.parkedCount+=row.parked?1u:0u;
        if(row.parked && row.earthworksAllowed && row.groundworksMachine)++next.eligibleMachineCandidates;
        ++next.matchedCount;
        if(next.rowCount<WorkforceRows)next.vehicles[next.rowCount++]=row;
    }
    next.selectedIsEarthworksFacility=next.building.earthworksFacility;
    next.blockedConstructionAssignments=blockedAssignments;
    strcpy_s(next.status,"Read-only sample. Counts describe native lists/links; they do not establish usable workers or machines.");
    view=next;
}
}
void ResetWorkforce() {view=WorkforceView{};candidates={};selected={};blockedAssignments=0;lastSample=0;}
void FindWorkforceBuildings(Point point)
{
    ResetWorkforce();List buildings;
    if(!PointerList(World()+0x11B08,262144,&buildings)) {strcpy_s(view.status,"Building list unavailable.");return;}
    for(unsigned i=0;i<buildings.count;++i) {
        Address address=0;BuildingReading reading;
        if(!Element(buildings,i,&address)) {ResetWorkforce();strcpy_s(view.status,"Building scan failed; no candidates retained.");return;}
        if(!DescribeBuilding(address,i,&reading))continue;
        reading.distance=Distance(point,reading.position);
        if(reading.distance>150)continue; // Diagnostic picking radius only, never an operation's range.
        unsigned at=0;while(at<view.candidateCount && view.candidates[at].distance<=reading.distance)++at;
        if(at>=WorkforceCandidates)continue;
        const unsigned end=std::min(view.candidateCount,WorkforceCandidates-1);
        for(unsigned j=end;j>at;--j){view.candidates[j]=view.candidates[j-1];candidates[j]=candidates[j-1];}
        view.candidates[at]=reading;candidates[at]={i,reading.id,address};
        view.candidateCount=std::min(view.candidateCount+1,WorkforceCandidates);
    }
    strcpy_s(view.status,view.candidateCount?"Choose the intended building from the nearby origins below.":"No readable building origins within 150 m. Click closer to the building centre.");
}
void SelectWorkforceBuilding(unsigned candidate)
{
    if(candidate>=view.candidateCount)return;
    selected=candidates[candidate];view=WorkforceView{};view.selected=true;lastSample=0;Sample();
}
bool IsEarthworksBuilding(void* building)
{
    if(!IsEarthworksFacility(building))return false;
    ++blockedAssignments;
    view.blockedConstructionAssignments=blockedAssignments;
    host->log("earthworks  blocked normal construction assignment to dedicated Earthworks facility");
    return true;
}
bool IsEarthworksFacility(void* building)
{
    BuildingReading reading{};
    return DescribeBuilding(reinterpret_cast<Address>(building),0,&reading) && reading.earthworksFacility;
}
bool FindEarthworksFacility(unsigned id,BuildingReading* reading)
{
    if(!id || !reading)return false;
    List buildings;if(!PointerList(World()+0x11B08,262144,&buildings))return false;
    unsigned found=0;BuildingReading match{};
    for(unsigned i=0;i<buildings.count;++i) {
        Address address=0;BuildingReading candidate{};
        if(!Element(buildings,i,&address))return false;
        if(!DescribeBuilding(address,i,&candidate) || !candidate.earthworksFacility || candidate.id!=id)continue;
        match=candidate;++found;
    }
    if(found!=1)return false;
    *reading=match;return true;
}
bool SelectEarthworksFacility(unsigned id)
{
    BuildingReading reading{};
    if(!FindEarthworksFacility(id,&reading))return false;
    List buildings;if(!PointerList(World()+0x11B08,262144,&buildings))return false;
    Address address=0;
    if(!Element(buildings,reading.slot,&address) || !address)return false;
    unsigned liveId=0;
    if(!Read(address,&liveId) || liveId!=id)return false;
    selected={reading.slot,id,address};view=WorkforceView{};view.selected=true;lastSample=0;Sample();return true;
}
void TickWorkforce()
{
    if(!view.selected)return;
    const auto now=GetTickCount64();if(now-lastSample<500)return;lastSample=now;Sample();
}
WorkforceView ReadWorkforce() {return view;}
void LogWorkforce()
{
    if(view.selected)Sample();
    host->log("earthworks  stage07 DRY RUN selected=%d buildingValid=%d building=%u slot=%u asset=%s earthworksFacility=%d category=%d subtype=%d pos=(%.1f,%.1f) sample=%u occupantsValid=%d occupantSlots=%u nonnull=%u parkedValid=%d parkedSlots=%u vehiclesValid=%d unreadable=%u home=%u assigned=%u parked=%u matched=%u shown=%u blockedConstructionAssignments=%u status=%s",
        view.selected,view.buildingValid,view.building.id,view.building.slot,view.building.assetName,view.selectedIsEarthworksFacility,view.building.category,view.building.subtype,view.building.position.x,view.building.position.z,
        view.samples,view.occupantsValid,view.occupantSlots,view.occupants,view.parkedValid,view.parkedSlots,view.vehiclesValid,view.unreadableVehicles,
        view.homeCount,view.assignedCount,view.parkedCount,view.matchedCount,view.rowCount,view.blockedConstructionAssignments,view.status);
    for(unsigned i=0;i<view.rowCount;++i) {
        const auto& r=view.vehicles[i];
        host->log("earthworks  stage04 vehicle=%u type=%s category=%u H=%d A=%d P=%d jobStateValid=%d jobState=%d routeValid=%d routeStops=%u firstRouteIsFacility=%d positionValid=%d distance=%.1f",
            r.id,r.type,r.category,r.home,r.assigned,r.parked,r.nativeJobStateValid,r.nativeJobState,r.routeValid,r.routeStopCount,r.firstRouteStopIsFacility,r.positionValid,r.distance);
    }
}
}
