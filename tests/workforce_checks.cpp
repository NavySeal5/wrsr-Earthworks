// Synthetic memory only. This executable never opens or attaches to the game.
#include "../src/runtime/runtime.h"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
namespace ew { const TsmHost* host=nullptr; }
namespace {
using Address=uintptr_t;
template<class T> void Put(std::vector<unsigned char>& bytes,size_t offset,const T& value)
{std::memcpy(bytes.data()+offset,&value,sizeof(value));}
void Require(bool value,const char* message)
{if(!value){std::cerr<<message<<'\n';std::exit(1);}}
void Log(const char*,...) {}
void List(std::vector<unsigned char>& bytes,size_t offset,Address* data,unsigned count)
{Put(bytes,offset,reinterpret_cast<Address>(data));Put(bytes,offset+8,reinterpret_cast<Address>(data+count));}
}
int main()
{
    constexpr size_t World=0x9D4F10;
    std::vector<unsigned char> exe(World+0x12830),building(0xD00),type(0x368),vehicle(0x1710),vehicleType(0x300);
    TsmHost api{};api.exeBase=exe.data();api.log=Log;ew::host=&api;
    const Address b=reinterpret_cast<Address>(building.data()),v=reinterpret_cast<Address>(vehicle.data());
    Address buildings[]={b},vehicles[]={v},occupants[]={1,0,2},parked[]={v};
    List(exe,World+0x11B08,buildings,1);List(exe,World+0x12810,vehicles,1);
    Put(building,0,unsigned{123});Put(building,0x318,reinterpret_cast<Address>(type.data()));
    Put(building,0x350,ew::Point{100,10,200});Put(type,0x360,int{7});Put(type,0x364,int{9});
    List(building,0xBC0,occupants,3);List(building,0xC70,parked,1);
    Put(vehicle,4,unsigned{45});Put(vehicle,0x1708,reinterpret_cast<Address>(vehicleType.data()));
    Put(vehicle,0x4F0,b);Put(vehicle,0x4F8,b);Put(vehicle,0xDE0,ew::Point{103,10,204});
    Put(vehicleType,0x294,unsigned{4});std::memcpy(vehicleType.data()+0x200,"synthetic_excavator",19);
    ew::ResetWorkforce();ew::FindWorkforceBuildings({101,0,200});auto view=ew::ReadWorkforce();
    Require(view.candidateCount==1 && view.candidates[0].id==123,"candidate discovery");
    ew::SelectWorkforceBuilding(0);view=ew::ReadWorkforce();
    Require(view.selected && view.buildingValid && view.occupantsValid && view.occupantSlots==3 && view.occupants==2,"occupant slots versus non-null entries");
    Require(view.vehiclesValid && view.parkedValid && view.homeCount==1 && view.assignedCount==1 && view.parkedCount==1 && view.matchedCount==1,"overlapping links are not added as distinct machines");
    Require(view.rowCount==1 && view.vehicles[0].id==45 && view.vehicles[0].distance==5,"vehicle identity and distance");
    // A travelling vehicle remains linked to its home, without being called usable.
    List(building,0xC70,parked,0);Put(vehicle,0x4F8,Address{0});Put(vehicle,0xDE0,ew::Point{1000,0,200});
    ew::LogWorkforce();view=ew::ReadWorkforce();
    Require(view.homeCount==1 && view.assignedCount==0 && view.parkedCount==0 && view.vehicles[0].distance==900,"home/assignment/parked separation");
    Put(building,0xBC8,Address{1});ew::LogWorkforce();view=ew::ReadWorkforce();
    Require(!view.occupantsValid,"reversed vector rejected instead of zero workers");
    Put(vehicle,0x1708,Address{1});ew::LogWorkforce();view=ew::ReadWorkforce();
    Require(view.unreadableVehicles==1 && view.rowCount==0,"guarded unreadable vehicle reports incomplete scan");
    Put(building,0,unsigned{124});ew::LogWorkforce();
    Require(!ew::ReadWorkforce().selected,"same slot and address with replaced ID must invalidate selection");
    ew::TickWorkforce();Require(!ew::ReadWorkforce().selected,"replacement is not automatically attached");
    ew::ResetWorkforce();ew::FindWorkforceBuildings({10000,0,10000});
    Require(ew::ReadWorkforce().candidateCount==0,"diagnostic picking radius is bounded");
    ew::ResetWorkforce();Require(!ew::ReadWorkforce().selected && ew::ReadWorkforce().candidateCount==0,"world boundary reset");
    std::cout<<"Synthetic workforce checks passed: guarded lists, identity invalidation, independent vehicle links and reset. No game process used.\n";
}
