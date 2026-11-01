// SPDX-License-Identifier: GPL-3.0-only
// Native ABI/layout reference: RepublicMP terrain_hook.cpp and world.cpp,
// pinned in dependencies.lock.json. Brush units and bounds additionally
// inspected in the installed C3DDLL64 1.1.1.9 disassembly.
#include <windows.h>
#include "runtime.h"
#include "terrain_probe.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace ew {
namespace {
using RaiseFn=void (*)(void*,const float*,float,float,float,float,bool);
using HeightFn=float (*)(void*,const float*);
using TextureFn=void (*)(void*);
RaiseFn raiseLower=nullptr;
HeightFn query=nullptr;
TextureFn openTexture=nullptr,closeTexture=nullptr;
TerrainGrid grid;
GridRect patch;
struct Sample { int x,z; float before,last; };
std::vector<Sample> samples;
ProbeView view;
bool bound=false,loadedAgain=false;
constexpr char Fixture[]="save/4505 - Earthworks_Test_S03";
void Status(const char* text) { strcpy_s(view.status,text); host->log("earthworks  STAGE3 %s",text); }
bool FixtureActive()
{
    __try {
        const auto* path=reinterpret_cast<const char*>(host->exeBase+0x9D4F10+0x9F8);
        return host->exeBase[0x993C65]!=0 && (std::strcmp(path,Fixture)==0 || std::strcmp(path,"save\\4505 - Earthworks_Test_S03")==0);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void* Terrain()
{
    __try {
        const auto* holder=*reinterpret_cast<unsigned char**>(host->exeBase+0x9941F0);
        return holder ? *reinterpret_cast<void* const*>(holder+0xED8):nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
bool ReadGrid(void* terrain,TerrainGrid* out)
{
    __try {
        const auto* p=static_cast<unsigned char*>(terrain);
        out->originX=*reinterpret_cast<const float*>(p+0x890);
        out->originZ=*reinterpret_cast<const float*>(p+0x898);
        out->heightScale=*reinterpret_cast<const float*>(p+0x89C);
        out->width=*reinterpret_cast<const int*>(p+0x220);
        out->depth=*reinterpret_cast<const int*>(p+0x224);
        if(out->width<64 || out->width>8192 || out->depth<64 || out->depth>8192) return false;
        out->stepX=*reinterpret_cast<const float*>(p+0x87C)/static_cast<float>(out->width);
        out->stepZ=*reinterpret_cast<const float*>(p+0x880)/static_cast<float>(out->depth);
        return std::isfinite(out->heightScale) && out->heightScale>10 && out->heightScale<100000
            && std::isfinite(out->originX) && std::isfinite(out->originZ)
            && std::isfinite(out->stepX) && out->stepX>0.1f && out->stepX<50
            && std::isfinite(out->stepZ) && out->stepZ>0.1f && out->stepZ<50;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Height(void* terrain,Point p,float* height)
{
    __try { float pos[]={p.x,p.y,p.z}; *height=query(terrain,pos); return std::isfinite(*height); }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Money(double* rubles,double* dollars)
{
    __try { auto* w=host->exeBase+0x9D4F10; *rubles=*reinterpret_cast<double*>(w+0x588); *dollars=*reinterpret_cast<double*>(w+0x580); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool BuildingsClear(Point a,Point b)
{
    // Fixture-only guard. The visual empty-patch check must also include paths,
    // trees and underground infrastructure; this is not production protection.
    __try {
        auto** begin=*reinterpret_cast<unsigned char***>(host->exeBase+0x9E6A18);
        auto** end=*reinterpret_cast<unsigned char***>(host->exeBase+0x9E6A20);
        if (!begin || !end || end<begin || end-begin>200000) return false;
        for(auto** it=begin;it!=end;++it) if (*it) {
            const auto* p=reinterpret_cast<const Point*>(*it+0x350);
            if(!std::isfinite(p->x) || !std::isfinite(p->z)) return false;
            const float dx=std::max({a.x-p->x,0.0f,p->x-b.x});
            const float dz=std::max({a.z-p->z,0.0f,p->z-b.z});
            if(dx*dx+dz*dz<300*300) return false;
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool ApplyOne(void* terrain,Point p,float delta)
{
    __try {
        float pos[]={p.x,0,p.z};
        // Radii are in heightmap texels. At an aligned vertex this affects
        // only that vertex; adjacent vertices have distance >= 1 texel.
        // -1 is the engine's no-height-limit sentinel. Amount is normalized.
        raiseLower(terrain,pos,0.20f,0.45f,delta/grid.heightScale,-1.0f,false);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool Open(void* terrain) { __try { openTexture(terrain); return true; } __except(EXCEPTION_EXECUTE_HANDLER) { return false; } }
bool Close(void* terrain) { __try { closeTexture(terrain); return true; } __except(EXCEPTION_EXECUTE_HANDLER) { return false; } }
void Evidence(const char* phase)
{
    const std::string path=std::string(host->pluginDir)+"\\earthworks-stage03-samples.tsv";
    FILE* file=nullptr;
    if(fopen_s(&file,path.c_str(),"a")!=0 || !file) { Status("Could not write diagnostic sample evidence; test stopped."); view.faulted=true; return; }
    std::fprintf(file,"# %s edits=%u target=%.9g error=%.9g outside=%.9g reload=%.9g RUB=%.9g USD=%.9g\n",
        phase,view.edits,view.target,view.maxError,view.outsideDelta,view.reloadDelta,view.rubleChange,view.dollarChange);
    for(const auto& s:samples) {
        const auto p=grid.At(s.x,s.z);
        std::fprintf(file,"%s\t%d\t%d\t%.9g\t%.9g\t%d\t%.9g\t%.9g\n",phase,s.x,s.z,p.x,p.z,
            patch.Contains(s.x,s.z)?1:0,s.before,s.last);
    }
    std::fclose(file);
}
bool Measure(void* terrain,bool compareReload)
{
    view.maxError=0; view.outsideDelta=0; view.reloadDelta=0;
    for(auto& s:samples) {
        float height=0;
        if(!Height(terrain,grid.At(s.x,s.z),&height)) return false;
        if(compareReload) view.reloadDelta=std::max(view.reloadDelta,std::abs(height-s.last));
        s.last=height;
        if(patch.Contains(s.x,s.z)) view.maxError=std::max(view.maxError,std::abs(height-view.target));
        else view.outsideDelta=std::max(view.outsideDelta,std::abs(height-s.before));
    }
    host->log("earthworks  STAGE3 measurement edits=%u nativeCalls=%u target=%.6f maxError=%.6f outsideDelta=%.6f reloadDelta=%.6f nativeUnit=%.6f RUB=%.6f USD=%.6f",
        view.edits,view.calls,view.target,view.maxError,view.outsideDelta,view.reloadDelta,view.precision,view.rubleChange,view.dollarChange);
    return true;
}
void Capture(const Plan& selection,void* terrain)
{
    if(!selection.complete || selection.Width()>64 || selection.Depth()>64 || !ReadGrid(terrain,&grid) || !SnapInside(selection,grid,&patch) || patch.Count()>256) {
        Status("Select a small rectangle, roughly 20-50 m per side. Test limit is 64 m / 256 vertices."); return;
    }
    const Point outerA=grid.At(patch.x0-2,patch.z0-2),outerB=grid.At(patch.x1+2,patch.z1+2);
    if(!BuildingsClear(outerA,outerB)) { Status("Test blocked: building origins within 300 m, or building list unavailable."); return; }
    samples.clear(); view=ProbeView{}; view.available=true;
    view.low=100000;view.high=-100000;
    for(int z=patch.z0-2;z<=patch.z1+2;++z) for(int x=patch.x0-2;x<=patch.x1+2;++x) {
        float h=0;
        if(!Height(terrain,grid.At(x,z),&h) || h<3) { samples.clear(); Status("Test blocked: terrain query failed or patch/margin is too close to water level."); return; }
        samples.push_back({x,z,h,h});
        if(patch.Contains(x,z)) {view.low=std::min(view.low,h);view.high=std::max(view.high,h);}
    }
    if(view.high-view.low>1) { samples.clear(); Status("Choose a flatter test patch: original height variation must be at most 1 m."); return; }
    view.a=grid.At(patch.x0,patch.z0);view.b=grid.At(patch.x1,patch.z1);
    // Full interpolated influence reaches the adjacent unedited vertex row.
    view.outerA=grid.At(patch.x0-1,patch.z0-1);view.outerB=grid.At(patch.x1+1,patch.z1+1);
    view.precision=grid.heightScale/60000.0f;view.target=view.high+1;view.captured=true;
    view.maxError=view.target-view.low;
    loadedAgain=false;Evidence("capture");
    host->log("earthworks  STAGE3 capture fixture=%s x=%d..%d z=%d..%d step=(%.6f,%.6f) scale=%.6f origin=(%.3f,%.3f) low=%.6f high=%.6f",
        Fixture,patch.x0,patch.x1,patch.z0,patch.z1,grid.stepX,grid.stepZ,grid.heightScale,grid.originX,grid.originZ,view.low,view.high);
    Status("Captured. White rectangle: edited vertices. Red border: full interpolation envelope. Raise test is ready.");
}
void Edit(void* terrain,bool raise)
{
    if(!view.captured || view.faulted || loadedAgain) { Status("Capture a fresh patch before another edit after loading."); return; }
    if(!BuildingsClear(view.outerA,view.outerB)) { Status("Test blocked: a nearby building or an unreadable building list."); return; }
    std::vector<float> deltas; deltas.reserve(static_cast<size_t>(patch.Count()));
    const float targetHeight=raise?view.high+1:view.low-1;
    for(const auto& s:samples) {
        float now=0;
        if(!Height(terrain,grid.At(s.x,s.z),&now) || std::abs(now-s.last)>0.001f) { Status("Ground changed since the last measurement. Recapture before editing."); return; }
        if(patch.Contains(s.x,s.z)) deltas.push_back(targetHeight-now);
    }
    double rubles=0,dollars=0,afterRubles=0,afterDollars=0;
    if(!Money(&rubles,&dollars)) { Status("Cannot record native balances; test blocked."); return; }
    if(!Open(terrain)) { view.faulted=true;Status("Native heightmap open failed; no further edits permitted this session."); return; }
    size_t index=0;
    bool ok=true;
    for(const auto& s:samples) if(patch.Contains(s.x,s.z)) {
        if(!ApplyOne(terrain,grid.At(s.x,s.z),deltas[index++])) { ok=false;break; }
        ++view.calls;
    }
    const bool closed=Close(terrain);
    ++view.edits;view.target=targetHeight;
    if(!ok || !closed || !Money(&afterRubles,&afterDollars)) {view.faulted=true;Status("Native call failed. Stop this experiment and reload the disposable fixture.");return;}
    view.rubleChange=afterRubles-rubles;view.dollarChange=afterDollars-dollars;
    if(!Measure(terrain,false)) {view.faulted=true;Status("Readback failed after the edit. Reload the disposable fixture.");return;}
    Evidence(raise?"raise":"lower");
    if(view.outsideDelta>0.001f) {view.faulted=true;Status("Unexpected change to guard vertices. Further edits disabled; inspect evidence.");return;}
    Status(raise?"Raise applied. Check the actual ground, then save and reload to verify it.":"Lower applied. Check the actual ground, then save and reload to verify it.");
}
}
bool BindTerrainProbe()
{
    const auto module=static_cast<HMODULE>(host->engineModule);
    raiseLower=reinterpret_cast<RaiseFn>(GetProcAddress(module,"?EditRaiseLower@C3D_TERRAIN@@QEAAXVC3DVECTOR3@@MMMM_N@Z"));
    query=reinterpret_cast<HeightFn>(GetProcAddress(module,"?CollisionTerrainHeightOverPoint@C3D_TERRAIN@@QEAAMVC3DVECTOR3@@@Z"));
    openTexture=reinterpret_cast<TextureFn>(GetProcAddress(module,"?HeightmapTextureOpen@C3D_TERRAIN@@QEAAXXZ"));
    closeTexture=reinterpret_cast<TextureFn>(GetProcAddress(module,"?HeightmapTextureClose@C3D_TERRAIN@@QEAAXXZ"));
    const auto base=reinterpret_cast<uintptr_t>(module);
    bound=raiseLower && query && openTexture && closeTexture
        && reinterpret_cast<uintptr_t>(raiseLower)-base==0xF8E50
        && reinterpret_cast<uintptr_t>(query)-base==0xF5ED0
        && reinterpret_cast<uintptr_t>(openTexture)-base==0xF5500
        && reinterpret_cast<uintptr_t>(closeTexture)-base==0xF5520
        && *reinterpret_cast<const float*>(base+0x188DB0)==-1
        && *reinterpret_cast<const float*>(base+0x188D38)==60000;
    host->log("earthworks  STAGE3 native brush bindings %s; writes restricted to %s",bound?"ready":"unavailable",Fixture);
    return bound;
}
void ProbeWorldBoundary() { if(view.captured) loadedAgain=true; }
ProbeView ReadProbeView() { auto out=view;out.available=bound && FixtureActive();return out; }
void RunProbe(ProbeAction action,const Plan& selection)
{
    if(!bound || !FixtureActive()) {Status("Terrain editing is restricted to Earthworks_Test_S03.");return;}
    void* terrain=Terrain();
    if(!terrain) {Status("No native terrain object available.");return;}
    if(action==ProbeAction::Capture) Capture(selection,terrain);
    else if(action==ProbeAction::Raise) Edit(terrain,true);
    else if(action==ProbeAction::Lower) Edit(terrain,false);
    else if(view.captured) {
        if(!Measure(terrain,loadedAgain)) {view.faulted=true;Status("Native recheck failed.");return;}
        Evidence(loadedAgain?"reload":"recheck");
        Status(loadedAgain?"Reload compared with the last measurement. Inspect reload delta below.":"Native heights rechecked. Inspect target error and guard delta below.");
    }
}
}
