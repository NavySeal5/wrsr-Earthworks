#pragma once
#include "../core/planning.h"
#include <array>
#include <cstdint>

namespace ew {
constexpr unsigned WorkforceCandidates=5, WorkforceRows=48;
constexpr unsigned WorldEarthworksFacilities=128;
struct BuildingReading {
    unsigned slot=0,id=0;
    int category=-1,subtype=-1;
    Point position{};
    float distance=0;
    bool earthworksFacility=false;
    char assetName[64]{};
    char displayName[257]{};
};
struct VehicleReading {
    unsigned id=0,category=0,routeStopCount=0;
    int nativeJobState=-1;
    bool parked=false,home=false,assigned=false,positionValid=false,earthworksAllowed=false,groundworksMachine=false;
    bool nativeJobStateValid=false,routeValid=false,firstRouteStopIsFacility=false;
    float distance=0;
    char type[65]{};
};
struct WorkforceView {
    bool selected=false,buildingValid=false,occupantsValid=false,parkedValid=false,vehiclesValid=false;
    unsigned candidateCount=0,samples=0,occupantSlots=0,occupants=0,parkedSlots=0;
    unsigned homeCount=0,assignedCount=0,parkedCount=0,matchedCount=0,rowCount=0,unreadableVehicles=0;
    bool selectedIsEarthworksFacility=false;
    unsigned eligibleMachineCandidates=0,blockedConstructionAssignments=0;
    BuildingReading building{};
    std::array<BuildingReading,WorkforceCandidates> candidates{};
    std::array<VehicleReading,WorkforceRows> vehicles{};
    char status[200]="Read-only diagnostic. Click Find buildings, then click near a building.";
};
void ResetWorkforce();
void FindWorkforceBuildings(Point point);
bool ReadWorldEarthworksFacilities(std::array<BuildingReading,WorldEarthworksFacilities>* out,unsigned* count,bool* truncated);
void SelectWorkforceBuilding(unsigned candidate);
bool IsEarthworksBuilding(void* building);
bool IsEarthworksFacility(void* building);
bool FindEarthworksFacility(unsigned id, BuildingReading* reading);
bool SelectEarthworksFacility(unsigned id);
bool ReadEarthworksFacilityWorkforce(unsigned id, WorkforceView* out);
void TickWorkforce();
void LogWorkforce();
WorkforceView ReadWorkforce();
}
