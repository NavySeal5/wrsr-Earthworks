#pragma once
#include "../core/terrain_grid.h"

namespace ew {
enum class ProbeAction { Capture, Raise, Lower, Recheck };
struct ProbeView {
    bool available=false, captured=false, faulted=false;
    unsigned calls=0, edits=0;
    float low=0,high=0,target=0,maxError=0,outsideDelta=0,reloadDelta=0,precision=0;
    double rubleChange=0,dollarChange=0;
    Point a{},b{},outerA{},outerB{};
    char status[240]="Only enabled in Earthworks_Test_S03.";
};
bool BindTerrainProbe();
void ProbeWorldBoundary();
ProbeView ReadProbeView();
void RunProbe(ProbeAction action,const Plan& selection);
}
