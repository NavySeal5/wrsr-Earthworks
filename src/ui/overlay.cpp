// SPDX-License-Identifier: GPL-3.0-only
// DXGI bootstrap, overlay-chain re-entry protection and Win32 input approach
// adapted from RepublicMP, commit 630775724abd5e3346eb59f9c66e2873128d74a4.
#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "../runtime/runtime.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "../core/productivity.h"
#include "imgui_impl_dx11.h"
#include <algorithm>
#include <cstdio>
#include <mutex>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace ew {
namespace {
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
PresentFn originalPresent = nullptr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* target = nullptr;
IDXGISwapChain* gameChain = nullptr;
ImGuiContext* gui = nullptr;
HWND gameWindow = nullptr;
WNDPROC originalWndProc = nullptr;
std::recursive_mutex guiMutex;
RECT buttonRect{}, panelRect{};
bool panelRectValid = false;
bool showProbe=true;
uint64_t frameGeneration = 0;
thread_local int presentDepth = 0;
const ImU32 cutColor = IM_COL32(255,159,67,255), fillColor = IM_COL32(62,221,166,255);
void Send(Action action, float value = 0) { Enqueue({action,value,frameGeneration}); }
bool Hit(const RECT& rect, LPARAM point)
{
    const POINT p = {GET_X_LPARAM(point),GET_Y_LPARAM(point)};
    return PtInRect(&rect,p) != FALSE;
}
bool WorldHit(LPARAM point)
{
    RECT client{}; GetClientRect(gameWindow,&client);
    const int x=GET_X_LPARAM(point),y=GET_Y_LPARAM(point);
    return x>65 && y>85 && y<client.bottom-90
        && !(x>client.right-300 && y>client.bottom-210);
}
void RememberRect(RECT* rect)
{
    const auto p = ImGui::GetWindowPos(), s = ImGui::GetWindowSize();
    *rect = {static_cast<LONG>(p.x),static_cast<LONG>(p.y),static_cast<LONG>(p.x+s.x),static_cast<LONG>(p.y+s.y)};
}
LRESULT CALLBACK WindowProc(HWND window,UINT message,WPARAM wParam,LPARAM lParam)
{
    std::lock_guard<std::recursive_mutex> lock(guiMutex);
    auto* previous = ImGui::GetCurrentContext();
    ImGui::SetCurrentContext(gui);
    ImGui_ImplWin32_WndProcHandler(window,message,wParam,lParam);
    const bool over = Hit(buttonRect,lParam) || (panelRectValid && Hit(panelRect,lParam));
    if (message == WM_KILLFOCUS) { mouseCapture=false; keyboardCapture=false; leftRelease=false; }
    if (message == WM_MOUSEMOVE) mouseCapture=over;
    const bool leftMessage = message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK;
    const bool leftOwned = LeftTaken() || over;
    if (message == WM_LBUTTONDOWN && leftOwned) {
        leftRelease = true;
        if (panelOpen && !over && WorldHit(lParam) && GetForegroundWindow() == gameWindow) Send(Action::Pick);
    }
    if (message == WM_LBUTTONUP) leftRelease = false;
    const bool otherMouse = message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK
        || message == WM_MBUTTONDOWN || message == WM_MBUTTONUP || message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
    const bool keyMessage = message == WM_KEYDOWN || message == WM_CHAR || message == WM_SYSKEYDOWN;
    const bool swallow = (leftMessage && leftOwned) || (otherMouse && mouseCapture) || (keyMessage && keyboardCapture);
    ImGui::SetCurrentContext(previous);
    if (swallow) return TRUE;
    return CallWindowProcW(originalWndProc,window,message,wParam,lParam);
}
bool Project(const Snapshot& s,Point p,ImVec2* out)
{
    float eye[3]{}; float clip[4]{};
    for (int i=0; i<3; ++i) eye[i] = p.x*s.view[i]+p.y*s.view[4+i]+p.z*s.view[8+i]+s.view[12+i];
    for (int i=0; i<4; ++i) clip[i] = eye[0]*s.projection[i]+eye[1]*s.projection[4+i]+eye[2]*s.projection[8+i]+s.projection[12+i];
    if (!std::isfinite(clip[3]) || clip[3]<0.01f) return false;
    const auto size = ImGui::GetIO().DisplaySize;
    *out = {(clip[0]/clip[3]+1)*0.5f*size.x,(1-clip[1]/clip[3])*0.5f*size.y};
    return std::isfinite(out->x) && std::isfinite(out->y) && std::abs(out->x)<1e6f && std::abs(out->y)<1e6f;
}
void Line(const Snapshot& s,Point a,Point b,ImU32 color,float thickness=1)
{
    ImVec2 p,q;
    if (Project(s,a,&p) && Project(s,b,&q)) ImGui::GetBackgroundDrawList()->AddLine(p,q,color,thickness);
}
void Shade(const Snapshot& s,const Point* triangle,const float* depth,float sign,ImU32 color)
{
    Point clipped[6]{}; int count=0;
    for (int i=0; i<3; ++i) {
        const int j=(i+1)%3;
        const float a=depth[i]*sign,b=depth[j]*sign;
        if (a>=0) clipped[count++]=triangle[i];
        if ((a>0 && b<0) || (a<0 && b>0)) {
            const float t=a/(a-b);
            clipped[count++]={triangle[i].x+t*(triangle[j].x-triangle[i].x),triangle[i].y,triangle[i].z+t*(triangle[j].z-triangle[i].z)};
        }
    }
    if (count<3) return;
    ImVec2 screen[6]{};
    for (int i=0; i<count; ++i) if (!Project(s,clipped[i],&screen[i])) return;
    ImGui::GetBackgroundDrawList()->AddConvexPolyFilled(screen,count,(color & 0x00FFFFFFu) | (55u<<24));
}
void DrawQueuedWireframe(const Snapshot& s,const Plan& p)
{
    if(!p.complete) return;
    const ImU32 gray=IM_COL32(175,180,190,210);
    for(int z=0;z<GridSide;++z) for(int x=0;x<GridSide;++x) {
        const auto a=p.Vertex(x,z,true);
        if(x<GridCells) Line(s,a,p.Vertex(x+1,z,true),gray,1.5f);
        if(z<GridCells) Line(s,a,p.Vertex(x,z+1,true),gray,1.5f);
    }
    const int corners[][2]={{0,0},{GridCells,0},{GridCells,GridCells},{0,GridCells}};
    for(int i=0;i<4;++i) {
        const auto a=p.Vertex(corners[i][0],corners[i][1],true);
        const auto b=p.Vertex(corners[(i+1)%4][0],corners[(i+1)%4][1],true);
        Line(s,a,b,IM_COL32(205,210,220,235),2.5f);
    }
}
void DrawPreview(const Snapshot& s)
{
    if (!s.ready || !s.cameraValid) return;
    if(s.projectQueued) DrawQueuedWireframe(s,s.queuedPlan);
    const auto& p=s.plan;
    if (!p.first) return;
    if (!p.complete) {
        ImVec2 first;
        if (Project(s,p.a,&first)) ImGui::GetBackgroundDrawList()->AddCircle(first,7,IM_COL32(255,235,135,255),16,2);
        if (panelOpen && s.cursorValid && s.mode==PickMode::Corners) {
            const Point a={p.a.x,p.target,p.a.z},b={s.cursor.x,p.target,p.a.z},c={s.cursor.x,p.target,s.cursor.z},d={p.a.x,p.target,s.cursor.z};
            Line(s,a,b,fillColor,2); Line(s,b,c,fillColor,2); Line(s,c,d,fillColor,2); Line(s,d,a,fillColor,2);
        }
        return;
    }
    for (int z=0; z<GridCells; ++z) for (int x=0; x<GridCells; ++x) {
        const Point proposed[]={p.Vertex(x,z,true),p.Vertex(x+1,z,true),p.Vertex(x+1,z+1,true),p.Vertex(x,z+1,true)};
        const float d[]={p.target-p.Vertex(x,z,false).y,p.target-p.Vertex(x+1,z,false).y,
            p.target-p.Vertex(x+1,z+1,false).y,p.target-p.Vertex(x,z+1,false).y};
        for (int t=0; t<2; ++t) {
            const Point tri[]={proposed[0],proposed[t+1],proposed[t+2]};
            const float depths[]={d[0],d[t+1],d[t+2]};
            Shade(s,tri,depths,1,fillColor); Shade(s,tri,depths,-1,cutColor);
        }
    }
    for (int z=0; z<GridSide; ++z) for (int x=0; x<GridSide; ++x) {
        const auto a=p.Vertex(x,z,true); const auto ground=p.Vertex(x,z,false);
        const ImU32 color=p.target>=ground.y ? fillColor:cutColor;
        if (x<GridCells) Line(s,a,p.Vertex(x+1,z,true),color);
        if (z<GridCells) Line(s,a,p.Vertex(x,z+1,true),color);
        if (x==0 || z==0 || x==GridCells || z==GridCells) Line(s,a,ground,color,1.5f);
    }
    const int corners[][2]={{0,0},{GridCells,0},{GridCells,GridCells},{0,GridCells}};
    for(int i=0;i<4;++i) {
        const auto a=p.Vertex(corners[i][0],corners[i][1],true);
        const auto b=p.Vertex(corners[(i+1)%4][0],corners[(i+1)%4][1],true);
        Line(s,a,b,IM_COL32(245,250,255,255),2.5f);
    }
}
void DrawProbe(const Snapshot& s)
{
    if(!showProbe || !s.ready || !s.cameraValid || !s.probe.available || !s.probe.captured) return;
    const auto& p=s.probe;
    const Point corners[]={{p.a.x,p.target,p.a.z},{p.b.x,p.target,p.a.z},{p.b.x,p.target,p.b.z},{p.a.x,p.target,p.b.z}};
    const Point outer[]={{p.outerA.x,p.target,p.outerA.z},{p.outerB.x,p.target,p.outerA.z},
        {p.outerB.x,p.target,p.outerB.z},{p.outerA.x,p.target,p.outerB.z}};
    for(int i=0;i<4;++i) {
        Line(s,corners[i],corners[(i+1)%4],IM_COL32(255,255,255,255),3);
        Line(s,outer[i],outer[(i+1)%4],IM_COL32(255,70,70,255),2);
    }
}
void DrawInterface(const Snapshot& s)
{
    frameGeneration=s.generation;
    if (!s.ready) { panelOpen=false; panelRectValid=false; buttonRect={}; return; }
    if(panelOpen && s.cameraValid && s.workforce.buildingValid) {
        const auto p=s.workforce.building.position;
        const auto colour=IM_COL32(255,100,255,255);
        Line(s,{p.x-5,p.y+1,p.z},{p.x+5,p.y+1,p.z},colour,3);
        Line(s,{p.x,p.y+1,p.z-5},{p.x,p.y+1,p.z+5},colour,3);
        Line(s,{p.x,p.y,p.z},{p.x,p.y+12,p.z},colour,3);
    }
    const auto display=ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos({display.x-190,86},ImGuiCond_Always);
    ImGui::SetNextWindowSize({170,62});
    ImGui::Begin("Earthworks launcher",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    if (ImGui::Button("Earthworks",{150,42})) {
        panelOpen=!panelOpen.load(); leftRelease=true;
        host->log("earthworks  planning panel %s",panelOpen ? "opened":"closed");
    }
    RememberRect(&buttonRect); ImGui::End();
    if (!panelOpen) { panelRectValid=false; return; }
    const float panelWidth=std::min(660.0f,std::max(480.0f,display.x-20.0f));
    const float panelHeight=std::min(850.0f,std::max(360.0f,display.y-180.0f));
    ImGui::SetNextWindowPos({std::max(10.0f,display.x-panelWidth-20.0f),155},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({panelWidth,panelHeight},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({480,300},{std::min(1000.0f,std::max(480.0f,display.x-20.0f)),std::max(300.0f,display.y-180.0f)});
    bool open=true;
    ImGui::Begin("Earthworks | Planning prototype",&open,ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextColored({0.4f,0.9f,0.75f,1},"DEDICATED FACILITY  |  STAGE 7 DRY-RUN");
    ImGui::TextWrapped("Plan a flat rectangle, save drafts, and queue one project. This stage does not execute terrain work.");
    ImGui::Separator();
    ImGui::BeginChild("Planning status",{0,ImGui::GetTextLineHeight()*3+4},ImGuiChildFlags_None,ImGuiWindowFlags_NoScrollbar);
    ImGui::TextWrapped("%s",s.message);
    ImGui::EndChild();
    if (ImGui::Button("Select new area")) Send(Action::Select);
    ImGui::SameLine();
    if (ImGui::Button("Sample height")) Send(Action::Sample);
    ImGui::TextDisabled("Click two opposite corners on the ground.");
    static float height=0;
    static uint64_t seenRevision=~uint64_t{0},seenWorld=~uint64_t{0};
    if (seenRevision!=s.revision || seenWorld!=s.generation) { height=s.plan.target; seenRevision=s.revision; seenWorld=s.generation; }
    ImGui::SetNextItemWidth(180);
    if (ImGui::InputFloat("Target height (m)",&height,0,0,"%.2f",ImGuiInputTextFlags_EnterReturnsTrue)) Send(Action::Target,height);
    ImGui::SameLine(); if (ImGui::Button("Apply")) Send(Action::Target,height);
    if (s.plan.complete) {
        ImGui::Text("Footprint: %.1f x %.1f m  |  %.0f m2",s.plan.Width(),s.plan.Depth(),static_cast<double>(s.plan.Width())*s.plan.Depth());
        ImGui::TextColored({1,0.62f,0.26f,1},"Cut estimate: %.0f m3",s.volumes.cut);
        ImGui::TextColored({0.24f,0.87f,0.65f,1},"Fill estimate: %.0f m3",s.volumes.fill);
        ImGui::TextWrapped("Sharp footprint edges. The overlay is visible through terrain. Volumes use a sampled grid and are estimates.");
        ImGui::BeginDisabled(s.projectQueued);
        if(ImGui::Button("Confirm and queue project"))Send(Action::ConfirmProject);
        ImGui::EndDisabled();
    }
    if(s.projectQueued)ImGui::TextColored({0.95f,0.82f,0.35f,1},"Queued project %llu is saved with the native slot. Stage 7 never edits terrain.",s.projectId);
    if(s.projectQueued)ImGui::TextWrapped("Grey wireframe: queued project. Coloured wireframe: editable draft.");
    {
        ImGui::SeparatorText("Stage 7 | Facility dry-run");
        if(!s.projectQueued) {
            ImGui::TextWrapped("First plan an area and press Confirm and queue project. Then choose an Earthworks facility here.");
        } else {
            if(ImGui::Button("Find Earthworks facility"))Send(Action::InspectBuilding);
            ImGui::SameLine();
            ImGui::BeginDisabled(!s.workforce.selected || !s.workforce.buildingValid || !s.workforce.building.earthworksFacility);
            if(ImGui::Button("Assign selected facility"))Send(Action::AssignProject);
            ImGui::EndDisabled();
            if(s.workforce.selected && s.workforce.buildingValid) {
                if(s.workforce.building.earthworksFacility)
                    ImGui::Text("Selected Earthworks facility ID %u. Assign it above.",s.workforce.building.id);
                else
                    ImGui::TextDisabled("Selected building is not an Earthworks facility.");
            } else ImGui::TextDisabled("Click Find, click near a facility on the map, then choose its Earthworks facility result below.");
        }
        if(s.projectQueued && s.projectFacilityId) {
            ImGui::SameLine();if(ImGui::Button("Clear assignment"))Send(Action::ClearProjectAssignment);
            if(s.assignedFacilityFound) {
                ImGui::Text("Assigned Earthworks facility ID %u | distance to plan centre %.1f m",s.projectFacilityId,s.projectFacilityDistance);
                ImGui::TextWrapped("Distance only. Operating range and footprint coverage are not established.");
                const auto& w=s.workforce;
                if(w.selected && w.buildingValid && w.building.id==s.projectFacilityId) {
                    if(w.occupantsValid && w.parkedValid && w.vehiclesValid) {
                        const double relativeRate=w.eligibleMachineCandidates
                            ? MachineRate(std::min(static_cast<int>(w.occupants),10),ReferenceGroundworksRating)
                            : WorkerOnlyRate(static_cast<int>(w.occupants));
                        ImGui::Text("Observed occupants: %u | parked bulldozer/excavator candidates: %u",w.occupants,w.eligibleMachineCandidates);
                        ImGui::Text("Provisional one-machine relative rate: %.2f manual-rate units",relativeRate);
                        ImGui::TextDisabled("Dry-run estimate only; candidates are not reserved or verified available. No completion time is estimated.");
                    } else ImGui::TextColored({1,0.65f,0.3f,1},"Staffing/equipment readings incomplete; no capacity summary.");
                } else ImGui::TextDisabled("Select the assigned facility in the inspector to view its latest read-only counts.");
            } else ImGui::TextColored({1,0.65f,0.3f,1},"Assigned facility ID %u is not present or could not be verified in this world.",s.projectFacilityId);
        } else if(s.projectQueued) ImGui::TextDisabled("No facility assigned yet.");
    }
    ImGui::BeginDisabled(!s.canUndo); if (ImGui::Button("Undo")) Send(Action::Undo); ImGui::EndDisabled();
    ImGui::SameLine(); ImGui::BeginDisabled(!s.canRedo); if (ImGui::Button("Redo")) Send(Action::Redo); ImGui::EndDisabled();
    ImGui::SameLine(); if (ImGui::Button("Clear preview")) Send(Action::Clear);
    if(ImGui::CollapsingHeader("Stage 4 | Read-only workforce inspector",ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& w=s.workforce;
        ImGui::TextWrapped("Facility identity is detected from the dedicated Workshop asset name. Ordinary construction offices remain unchanged.");
        if(ImGui::Button("Find buildings")) Send(Action::InspectBuilding);
        ImGui::SameLine();if(ImGui::Button("Clear inspector")) Send(Action::ClearInspector);
        ImGui::TextWrapped("%s",w.status);
        for(unsigned i=0;i<w.candidateCount;++i) {
            const auto& b=w.candidates[i];ImGui::PushID(static_cast<int>(i));
            char label[120];sprintf_s(label,"%s ID %u | %.1f m from click",b.earthworksFacility?"Earthworks facility":"Other building",b.id,b.distance);
            if(ImGui::Button(label))Send(Action::ChooseBuilding,static_cast<float>(i));
            ImGui::TextWrapped("X %.1f Z %.1f | %s | category %d / subtype %d",b.position.x,b.position.z,b.assetName,b.category,b.subtype);
            ImGui::PopID();
        }
        if(w.selected && !w.buildingValid)ImGui::TextColored({1,0.65f,0.3f,1},"Building fields: UNAVAILABLE");
        if(w.selected && w.buildingValid) {
            ImGui::Text("Building ID %u | slot %u | sample %u",w.building.id,w.building.slot,w.samples);
            ImGui::Text("Origin X %.1f Z %.1f | category %d / %d",w.building.position.x,w.building.position.z,w.building.category,w.building.subtype);
            if(w.occupantsValid)ImGui::Text("Occupant list: %u non-null / %u slots",w.occupants,w.occupantSlots);
            else ImGui::TextColored({1,0.65f,0.3f,1},"Occupant list: UNAVAILABLE");
            if(w.building.earthworksFacility)ImGui::TextColored({0.3f,0.9f,0.65f,1},"Automatically recognized Earthworks facility.");
            else ImGui::TextColored({1,0.65f,0.3f,1},"Not an Earthworks facility; Earthworks filters do not apply.");
            ImGui::TextWrapped("Occupant-list entries are observed counts; their precise work availability is not verified.");
            if(w.parkedValid)ImGui::Text("Parked list: %u native slots",w.parkedSlots);
            else ImGui::TextColored({1,0.65f,0.3f,1},"Parked list: UNAVAILABLE");
            if(w.vehiclesValid && w.unreadableVehicles==0) {
                ImGui::Text("Vehicle links: Home %u | Assignment %u",w.homeCount,w.assignedCount);
                if(w.parkedValid)ImGui::Text("Parked matches: %u",w.parkedCount);
            } else ImGui::TextColored({1,0.65f,0.3f,1},"Vehicle scan INCOMPLETE | unreadable: %u",w.unreadableVehicles);
            ImGui::TextWrapped("H=home link, A=assignment/owner link, P=parked-list membership. These overlap; P alone is not proof of idleness. Distance is diagnostic only.");
            ImGui::Text("Showing %u of %u readable linked vehicles",w.rowCount,w.matchedCount);
            for(unsigned i=0;i<w.rowCount;++i) {
                const auto& v=w.vehicles[i];
                ImGui::Text("ID %u | %s%s%s | %s | category %u",v.id,v.home?"H ":"",v.assigned?"A ":"",v.parked?"P":"",v.groundworksMachine&&v.earthworksAllowed?"machine candidate":"not a machine candidate",v.category);
                ImGui::TextWrapped("%s",v.type);
                if(v.positionValid)ImGui::TextDisabled("%.1f m from building origin",v.distance);
                else ImGui::TextDisabled("Position unavailable");
            }
            ImGui::TextWrapped("Machine skill rating, fuel, usable availability and reservations: UNVERIFIED. Parked matches are candidates only. Ordinary construction assignments are blocked automatically for the dedicated facility.");
            ImGui::Text("Blocked ordinary construction assignments this session: %u",w.blockedConstructionAssignments);
        if(w.building.earthworksFacility && s.projectQueued) {
            if(ImGui::Button("Assign this facility now"))Send(Action::AssignProject);
        }
        }
        if(ImGui::Button("Log inspector snapshot"))Send(Action::LogInspector);
        ImGui::TextWrapped("Readings refresh about twice per second, including while this panel is closed. Close Earthworks to use native building controls, then reopen to compare.");
    }
    if(s.probe.available && ImGui::CollapsingHeader("Stage 3 | Controlled terrain test")) {
        static bool emptyChecked=false;
        static uint64_t checkedWorld=~uint64_t{0};
        if(checkedWorld!=s.generation) {emptyChecked=false;checkedWorld=s.generation;}
        const auto& p=s.probe;
        ImGui::TextColored({1,0.6f,0.3f,1},"THE TEST BUTTONS CHANGE REAL GROUND");
        ImGui::TextWrapped("Disposable S03 save only. Select an empty, dry 20-50 m rectangle. Include paths, trees and underground infrastructure in your margin check.");
        ImGui::Checkbox("Patch and margin checked empty",&emptyChecked);
        ImGui::Checkbox("Show captured test outline",&showProbe);
        ImGui::BeginDisabled(!emptyChecked || !s.plan.complete);
        if(ImGui::Button("Capture selected test patch")) Send(Action::CaptureTest);
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s",p.status);
        if(p.captured) {
            ImGui::Text("Original: %.3f .. %.3f m",p.low,p.high);
            ImGui::Text("Target %.3f m | native unit %.5f m",p.target,p.precision);
            ImGui::Text("Target error: %.5f m",p.maxError);
            ImGui::Text("Guard delta: %.5f m",p.outsideDelta);
            ImGui::Text("Reload delta %.5f m | edits %u",p.reloadDelta,p.edits);
            ImGui::Text("Balances: RUB %.2f / USD %.2f",p.rubleChange,p.dollarChange);
            ImGui::BeginDisabled(!emptyChecked || p.faulted);
            if(ImGui::Button("Apply raise test (+1 m)")) Send(Action::RaiseTest);
            if(ImGui::Button("Apply lower test (-1 m)")) Send(Action::LowerTest);
            ImGui::EndDisabled();
            if(ImGui::Button("Recheck / compare after reload")) Send(Action::RecheckTest);
            ImGui::TextWrapped("White: edited vertex rectangle. Red: full interpolation envelope. Instant diagnostic; no staffing/cost model. Draft Undo never reverses ground edits.");
        }
    }
    ImGui::Separator();
    ImGui::TextWrapped("Right-drag and wheel: camera. Close this panel to restore normal left clicks. Preview remains visible.");
    ImGui::TextDisabled("Drafts and queued projects are stored with a matching native save. Preview controls never change terrain.");
    if (ImGui::CollapsingHeader("Terrain readings")) {
        ImGui::Text("Cursor: X %.1f  Z %.1f  Height %.2f m",s.cursor.x,s.cursor.z,s.cursor.y);
        ImGui::Text("Day %d, year %d, day progress %.3f",s.day,s.year,s.time);
        ImGui::Text("Grid: %d x %d samples",GridSide,GridSide);
    }
    if (ImGui::Button("Close planning",{ImGui::GetContentRegionAvail().x,32})) open=false;
    RememberRect(&panelRect); panelRectValid=open; ImGui::End();
    if (!open) { panelOpen=false; leftRelease=true; host->log("earthworks  planning panel closed; native left clicks released after button up"); }
}
void CleanupTarget() { if (target) { target->Release(); target=nullptr; } }
bool CreateTarget(IDXGISwapChain* swap)
{
    ID3D11Texture2D* buffer=nullptr;
    if (FAILED(swap->GetBuffer(0,__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&buffer)))) return false;
    const auto hr=device->CreateRenderTargetView(buffer,nullptr,&target); buffer->Release(); return SUCCEEDED(hr);
}
bool Initialize(IDXGISwapChain* swap)
{
    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(swap->GetDesc(&desc)) || !desc.OutputWindow || !IsWindowVisible(desc.OutputWindow)) return false;
    DWORD pid=0; GetWindowThreadProcessId(desc.OutputWindow,&pid);
    if (pid!=GetCurrentProcessId()) return false;
    if (FAILED(swap->GetDevice(__uuidof(ID3D11Device),reinterpret_cast<void**>(&device)))) return false;
    device->GetImmediateContext(&context); gameWindow=desc.OutputWindow;
    gui=ImGui::CreateContext(); ImGui::SetCurrentContext(gui);
    auto& io=ImGui::GetIO(); io.IniFilename=nullptr; io.LogFilename=nullptr;
    ImFontConfig font; font.SizePixels=19; io.Fonts->AddFontDefault(&font);
    ImGui::StyleColorsDark(); ImGui::GetStyle().WindowRounding=7; ImGui::GetStyle().FrameRounding=4;
    if (!ImGui_ImplWin32_Init(gameWindow) || !ImGui_ImplDX11_Init(device,context)) return false;
    SetLastError(0);
    originalWndProc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(gameWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(WindowProc)));
    if (!originalWndProc) return false;
    gameChain=swap;
    host->log("earthworks  overlay initialized hwnd=%p; on-screen Earthworks button only",gameWindow);
    return true;
}
void Render(IDXGISwapChain* swap)
{
    std::lock_guard<std::recursive_mutex> lock(guiMutex);
    auto* previous=ImGui::GetCurrentContext();
    if (!gameChain && !Initialize(swap)) { ImGui::SetCurrentContext(previous); return; }
    if (swap!=gameChain) { ImGui::SetCurrentContext(previous); return; }
    ImGui::SetCurrentContext(gui);
    if (!target && !CreateTarget(swap)) { ImGui::SetCurrentContext(previous); return; }
    ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
    const auto& io=ImGui::GetIO();
    const auto snapshot=ReadSnapshot();
    ImGui::GetBackgroundDrawList()->PushClipRect({65,85},{io.DisplaySize.x,io.DisplaySize.y-90},true);
    DrawPreview(snapshot);
    DrawProbe(snapshot);
    ImGui::GetBackgroundDrawList()->PopClipRect();
    DrawInterface(snapshot);
    const bool focused=GetForegroundWindow()==gameWindow;
    mouseCapture=focused && io.WantCaptureMouse && snapshot.ready;
    keyboardCapture=focused && io.WantTextInput && panelOpen;
    if (!(GetAsyncKeyState(VK_LBUTTON)&0x8000)) leftRelease=false;
    ImGui::GetIO().MouseDrawCursor=focused && (mouseCapture || panelOpen);
    ClearInputLatches();
    ImGui::Render();
    ID3D11RenderTargetView* oldTarget=nullptr; ID3D11DepthStencilView* oldDepth=nullptr;
    context->OMGetRenderTargets(1,&oldTarget,&oldDepth);
    context->OMSetRenderTargets(1,&target,nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    context->OMSetRenderTargets(1,&oldTarget,oldDepth);
    if (oldTarget) oldTarget->Release(); if (oldDepth) oldDepth->Release();
    // Do not retain a back-buffer reference between frames. Native resizes can
    // proceed without intercepting ResizeBuffers or chaining its Steam hook.
    CleanupTarget();
    ImGui::SetCurrentContext(previous);
}
HRESULT STDMETHODCALLTYPE Present(IDXGISwapChain* swap,UINT interval,UINT flags)
{
    if (presentDepth) {
        IDXGISwapChain1* next=nullptr;
        if (FAILED(swap->QueryInterface(__uuidof(IDXGISwapChain1),reinterpret_cast<void**>(&next)))) return S_OK;
        DXGI_PRESENT_PARAMETERS p{}; const auto hr=next->Present1(interval,flags,&p); next->Release(); return hr;
    }
    ++presentDepth;
    try { if (!(flags & DXGI_PRESENT_TEST)) Render(swap); }
    catch (...) { panelOpen=false; mouseCapture=false; keyboardCapture=false; host->log("earthworks  overlay exception; input released"); }
    const auto hr=originalPresent(swap,interval,flags);
    --presentDepth; return hr;
}
bool PatchSlot(void** slot,void* replacement,void** original)
{
    DWORD protect=0;
    if (!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protect)) return false;
    *original=*slot; *slot=replacement;
    DWORD ignored=0; VirtualProtect(slot,sizeof(void*),protect,&ignored); return true;
}
using CreateDeviceFn = decltype(&D3D11CreateDeviceAndSwapChain);
CreateDeviceFn originalCreate = nullptr;
HRESULT WINAPI CreateDevice(IDXGIAdapter* adapter,D3D_DRIVER_TYPE type,HMODULE software,UINT flags,
    const D3D_FEATURE_LEVEL* levels,UINT levelCount,UINT sdk,const DXGI_SWAP_CHAIN_DESC* desc,
    IDXGISwapChain** swap,ID3D11Device** dev,D3D_FEATURE_LEVEL* level,ID3D11DeviceContext** ctx)
{
    const auto hr=originalCreate(adapter,type,software,flags,levels,levelCount,sdk,desc,swap,dev,level,ctx);
    // Observe the game's actual creation after Steam has initialized its chain.
    // A disposable swap chain created early races Steam's own initialization.
    if (SUCCEEDED(hr) && swap && *swap && !originalPresent) {
        auto** table=*reinterpret_cast<void***>(*swap);
        const bool ok=PatchSlot(&table[8],reinterpret_cast<void*>(Present),reinterpret_cast<void**>(&originalPresent));
        host->log("earthworks  real swap-chain Present hook %s; original=%p",ok ? "installed":"failed",reinterpret_cast<void*>(originalPresent));
    }
    return hr;
}
}
void StartOverlay()
{
    if (!host->patchIat(host->engineModule,"d3d11.dll","D3D11CreateDeviceAndSwapChain",reinterpret_cast<void*>(CreateDevice),
        reinterpret_cast<void**>(&originalCreate),"Earthworks graphics creation")) host->log("earthworks  graphics creation import hook failed");
}
}
