// SPDX-License-Identifier: GPL-3.0-only
// DXGI bootstrap, overlay-chain re-entry protection and Win32 input approach
// adapted from RepublicMP, commit 630775724abd5e3346eb59f9c66e2873128d74a4.
#include <windows.h>
#include <windowsx.h>
#include <wincodec.h>
#include <wrl/client.h>
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
#include <string>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND,UINT,WPARAM,LPARAM);
namespace ew {
namespace {
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*,UINT,UINT);
PresentFn originalPresent = nullptr;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* target = nullptr;
ID3D11ShaderResourceView* panelPaper = nullptr;
UINT panelPaperWidth = 0, panelPaperHeight = 0;
IDXGISwapChain* gameChain = nullptr;
ImGuiContext* gui = nullptr;
HWND gameWindow = nullptr;
WNDPROC originalWndProc = nullptr;
std::recursive_mutex guiMutex;
RECT buttonRect{}, panelRect{};
bool panelRectValid = false;
bool panelMinimized = false;
bool restorePanelSize = false;
float expandedPanelHeight = 640.0f;
bool escapeKeyDown = false;
bool escapeOwned = false;
bool pickCancelledByEscape = false;
#ifdef EARTHWORKS_DEVELOPER_UI
bool showProbe=true;
#endif
uint64_t frameGeneration = 0;
thread_local int presentDepth = 0;
const ImU32 cutColor = IM_COL32(255,159,67,255), fillColor = IM_COL32(62,221,166,255);
void Send(Action action, float value = 0)
{
    if(action==Action::Select || action==Action::Sample || action==Action::InspectBuilding) pickCancelledByEscape=false;
    Enqueue({action,value,frameGeneration});
}
void SendProject(Action action,uint64_t projectId) { Enqueue({action,0,frameGeneration,projectId}); }
const char* ProjectStatusName(ProjectStatus status)
{
    switch(status) {
    case ProjectStatus::Queued:return "Queued";
    case ProjectStatus::Running:return "Running";
    case ProjectStatus::Paused:return "Paused";
    case ProjectStatus::Waiting:return "Waiting";
    case ProjectStatus::Completed:return "Completed";
    case ProjectStatus::Cancelled:return "Cancelled";
    default:return "Draft";
    }
}
bool ProjectCanMove(ProjectStatus status)
{
    return status==ProjectStatus::Queued||status==ProjectStatus::Running||status==ProjectStatus::Waiting||status==ProjectStatus::Paused;
}
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
    if (message == WM_KILLFOCUS) { mouseCapture=false; keyboardCapture=false; leftRelease=false; escapeKeyDown=false; escapeOwned=false; escapeCapture=false; }
    bool escapeMessage=false;
    if(message==WM_KEYDOWN && wParam==VK_ESCAPE) {
        if(!escapeKeyDown) {
            escapeKeyDown=true;
            if(panelOpen) {
                escapeOwned=true;
                escapeCapture=true;
                if(mapPickCapture) { mapPickCapture=false; pickCancelledByEscape=true; Send(Action::CancelPick); }
                else { panelOpen=false; panelRectValid=false; keyboardCapture=false; pickCancelledByEscape=false; host->log("earthworks  planning panel closed with Escape"); }
            }
        }
        escapeMessage=escapeOwned;
    } else if(message==WM_KEYUP && wParam==VK_ESCAPE) {
        escapeKeyDown=false;
        escapeMessage=escapeOwned;
        if(escapeOwned) escapeCapture=false;
        escapeOwned=false;
    }
    if (message == WM_MOUSEMOVE) mouseCapture=over;
    const bool leftMessage = message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK;
    const bool mapPick = mapPickCapture && !over && WorldHit(lParam);
    const bool leftOwned = mouseCapture || leftRelease || over || mapPick;
    if (message == WM_LBUTTONDOWN && leftOwned) {
        leftRelease = true;
        if (mapPick && GetForegroundWindow() == gameWindow) Send(Action::Pick);
    }
    if (message == WM_LBUTTONUP) leftRelease = false;
    const bool otherMouse = message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK
        || message == WM_MBUTTONDOWN || message == WM_MBUTTONUP || message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
    const bool keyMessage = message == WM_KEYDOWN || message == WM_CHAR || message == WM_SYSKEYDOWN;
    const bool swallow = (leftMessage && leftOwned) || (otherMouse && mouseCapture) || (keyMessage && keyboardCapture) || escapeMessage;
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
void DrawQueuedOutline(const Snapshot& s,Point a,Point b,float targetHeight,ImU32 color)
{
    const Point corners[]={{a.x,targetHeight,a.z},{b.x,targetHeight,a.z},{b.x,targetHeight,b.z},{a.x,targetHeight,b.z}};
    for(int i=0;i<4;++i)Line(s,corners[i],corners[(i+1)%4],color,2.5f);
}
void DrawPreview(const Snapshot& s)
{
    if (!s.ready || !s.cameraValid) return;
    for(size_t i=0;i<s.projectCount;++i) {
        const auto& project=s.projects[i];
        if(project.status!=ProjectStatus::Queued&&project.status!=ProjectStatus::Running
            &&project.status!=ProjectStatus::Waiting&&project.status!=ProjectStatus::Paused)continue;
        if(project.id==s.projectId)DrawQueuedWireframe(s,s.queuedPlan);
        else DrawQueuedOutline(s,project.a,project.b,project.target,IM_COL32(175,180,190,205));
    }
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
#ifdef EARTHWORKS_DEVELOPER_UI
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
#endif
void DrawPanelPaper(bool headerOnly);
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
    ImGui::SetNextWindowPos({display.x-170,86},ImGuiCond_Always);
    ImGui::SetNextWindowSize({150,42});
    ImGui::PushStyleColor(ImGuiCol_WindowBg,{0,0,0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{0,0});
    ImGui::Begin("Earthworks launcher",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    if (ImGui::Button("Earthworks",{150,42})) {
        panelOpen=!panelOpen.load(); leftRelease=true;
        host->log("earthworks  planning panel %s",panelOpen ? "opened":"closed");
    }
    RememberRect(&buttonRect); ImGui::End();
    if (!panelOpen) { panelRectValid=false; return; }
    const float panelWidth=std::min(660.0f,std::max(480.0f,display.x-20.0f));
    const float panelHeight=std::min(850.0f,std::max(360.0f,display.y-180.0f));
    constexpr float headerHeight=56.0f;
    ImGui::SetNextWindowPos({std::max(10.0f,display.x-panelWidth-20.0f),155},ImGuiCond_FirstUseEver);
    if(panelMinimized) ImGui::SetNextWindowSize({panelWidth,headerHeight+2.0f},ImGuiCond_Always);
    else if(restorePanelSize) { ImGui::SetNextWindowSize({panelWidth,expandedPanelHeight},ImGuiCond_Always); restorePanelSize=false; }
    else ImGui::SetNextWindowSize({panelWidth,panelHeight},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({480,44},{std::min(1000.0f,std::max(480.0f,display.x-20.0f)),std::max(300.0f,display.y-180.0f)});
    bool open=true;
    ImGui::PushStyleColor(ImGuiCol_WindowBg,{0,0,0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f);
    ImGui::Begin("##EarthworksPanel",&open,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    const bool collapsedThisFrame=panelMinimized;
    auto* panelDraw=ImGui::GetWindowDrawList();
    const auto panelPos=ImGui::GetWindowPos(),panelSize=ImGui::GetWindowSize();
    DrawPanelPaper(collapsedThisFrame);
    panelDraw->AddText({panelPos.x+11.0f,panelPos.y+29.5f-ImGui::GetTextLineHeight()*0.5f},IM_COL32(255,255,255,255),"Earthworks | Terrain projects");
    ImGui::PushStyleColor(ImGuiCol_Text,{1,1,1,1});
    ImGui::PushStyleColor(ImGuiCol_Button,{0.59f,0.235f,0.216f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0.70f,0.31f,0.29f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,{0.42f,0.14f,0.13f,1});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);
    ImGui::SetCursorScreenPos({panelPos.x+panelSize.x-70.0f,panelPos.y+14.5f});
    if(ImGui::Button("-",{30.0f,30.0f})) { panelMinimized=!panelMinimized; restorePanelSize=!panelMinimized; }
    ImGui::SameLine(0,4.0f);
    if(ImGui::Button("X",{30.0f,30.0f})) open=false;
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    if(!panelMinimized && !collapsedThisFrame) expandedPanelHeight=panelSize.y;
    if(!open) {
        panelOpen=false; mapPickCapture=false; leftRelease=true; panelRectValid=false;
        ImGui::End();
        host->log("earthworks  planning panel closed; native left clicks released after button up");
        return;
    }
    if(collapsedThisFrame || panelMinimized) {
        RememberRect(&panelRect); panelRectValid=true; ImGui::End(); return;
    }
    ImGui::SetCursorScreenPos({panelPos.x+ImGui::GetStyle().WindowPadding.x,panelPos.y+headerHeight+ImGui::GetStyle().WindowPadding.y});
    ImGui::TextWrapped("Plan terrain work and manage projects from one place.");
    ImGui::Separator();
    ImGui::TextWrapped("%s",s.message);
    static float height=0;
    static uint64_t seenRevision=~uint64_t{0},seenWorld=~uint64_t{0};
    if (seenRevision!=s.revision || seenWorld!=s.generation) { height=s.plan.target; seenRevision=s.revision; seenWorld=s.generation; }
    if(ImGui::BeginTabBar("Earthworks tabs")) {
        if(ImGui::BeginTabItem("Plan")) {
            if (ImGui::Button("Select area")) Send(Action::Select);
            ImGui::SameLine();
            if (ImGui::Button("Sample height")) Send(Action::Sample);
            ImGui::TextDisabled("Click two opposite corners on the ground; keep this panel open while selecting.");
            ImGui::SetNextItemWidth(180);
            if (ImGui::InputFloat("Target height (m)",&height,0,0,"%.2f",ImGuiInputTextFlags_EnterReturnsTrue)) Send(Action::Target,height);
            ImGui::SameLine(); if (ImGui::Button("Apply height")) Send(Action::Target,height);
            if (s.plan.complete) {
                ImGui::SeparatorText("Planned area");
                ImGui::Text("%.1f x %.1f m  |  %.0f m2",s.plan.Width(),s.plan.Depth(),static_cast<double>(s.plan.Width())*s.plan.Depth());
                ImGui::TextColored({0.55f,0.25f,0.10f,1},"Cut estimate: %.0f m3",s.volumes.cut);
                ImGui::TextColored({0.12f,0.43f,0.29f,1},"Fill estimate: %.0f m3",s.volumes.fill);
                ImGui::TextWrapped("Sharp edges. The preview is visible through terrain. Volume figures are sampled estimates.");
                if(ImGui::Button("Add project to queue"))Send(Action::ConfirmProject);
            } else ImGui::TextDisabled("Select an area to preview the planned terrain.");
            ImGui::Separator();
            ImGui::BeginDisabled(!s.canUndo); if (ImGui::Button("Undo")) Send(Action::Undo); ImGui::EndDisabled();
            ImGui::SameLine(); ImGui::BeginDisabled(!s.canRedo); if (ImGui::Button("Redo")) Send(Action::Redo); ImGui::EndDisabled();
            ImGui::SameLine(); if (ImGui::Button("Clear plan")) Send(Action::Clear);
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Projects")) {
            if(!s.projectTotal)ImGui::TextDisabled("No projects yet. Add a project from the Plan tab.");
            ImGui::BeginChild("Project queue",{0,155},ImGuiChildFlags_Borders);
            for(size_t i=0;i<s.projectCount;++i) {
                const auto& project=s.projects[i];
                char label[160];
                if(project.facilityId)sprintf_s(label,"Project %llu  |  %s  |  %.0f%%  |  Facility %u",static_cast<unsigned long long>(project.id),ProjectStatusName(project.status),project.progress*100.0f,project.facilityId);
                else sprintf_s(label,"Project %llu  |  %s  |  %.0f%%  |  Unassigned",static_cast<unsigned long long>(project.id),ProjectStatusName(project.status),project.progress*100.0f);
                if(ImGui::Selectable(label,s.projectId==project.id))SendProject(Action::SelectProject,project.id);
            }
            ImGui::EndChild();
            if(s.projectTotal>s.projectCount)ImGui::TextColored({0.55f,0.25f,0.08f,1},"Showing %zu of %zu projects.",s.projectCount,s.projectTotal);
            if(s.projectQueued) {
                ImGui::SeparatorText("Selected project");
                ImGui::Text("Project %llu  |  %s",static_cast<unsigned long long>(s.projectId),ProjectStatusName(s.projectStatus));
                const bool movable=ProjectCanMove(s.projectStatus);
                ImGui::BeginDisabled(!movable);if(ImGui::Button("Move up"))SendProject(Action::MoveProjectUp,s.projectId);ImGui::SameLine();if(ImGui::Button("Move down"))SendProject(Action::MoveProjectDown,s.projectId);ImGui::EndDisabled();
                if(s.projectStatus==ProjectStatus::Running||s.projectStatus==ProjectStatus::Waiting||s.projectStatus==ProjectStatus::Paused) {
                    char progressLabel[32];sprintf_s(progressLabel,"%.0f%%",s.projectProgress*100.0f);
                    ImGui::ProgressBar(s.projectProgress,{0,0},progressLabel);
                    if(s.projectStatus==ProjectStatus::Waiting)ImGui::TextDisabled("Started and waiting for facility capacity.");
                }
                if(s.projectStatus==ProjectStatus::Queued) {ImGui::BeginDisabled(!s.assignedFacilityFound);if(ImGui::Button("Start project"))Send(Action::StartProject);ImGui::EndDisabled();}
                if(s.projectStatus==ProjectStatus::Running||s.projectStatus==ProjectStatus::Waiting) {if(ImGui::Button("Pause project"))Send(Action::PauseProject);}
                else if(s.projectStatus==ProjectStatus::Paused&&s.projectNeedsReview) {
                    ImGui::TextColored({0.55f,0.25f,0.08f,1},"Terrain needs review before work can resume.");
                    if(!s.assignedFacilityFound)ImGui::TextDisabled("Assign this project to an Earthworks facility first.");
                    else if(ImGui::Button("Review terrain and resume"))Send(Action::ReviewResumeProject);
                } else if(s.projectStatus==ProjectStatus::Paused&&ImGui::Button("Resume project"))Send(Action::ResumeProject);
                if(s.projectStatus==ProjectStatus::Queued||s.projectStatus==ProjectStatus::Running||s.projectStatus==ProjectStatus::Waiting||s.projectStatus==ProjectStatus::Paused) {
                    ImGui::SameLine();if(ImGui::Button("Cancel project"))Send(Action::CancelProject);
                }
                if(s.projectStatus==ProjectStatus::Completed||s.projectStatus==ProjectStatus::Cancelled)ImGui::TextDisabled("This project is finished; its area can be planned again.");
                ImGui::SeparatorText("Facility");
                if(ImGui::Button("Choose facility on map"))Send(Action::InspectBuilding);
                if(s.workforce.candidateCount) {
                    bool foundEarthworks=false;
                    ImGui::TextDisabled("Choose a nearby Earthworks facility:");
                    for(unsigned i=0;i<s.workforce.candidateCount;++i) {
                        const auto& candidate=s.workforce.candidates[i];
                        if(!candidate.earthworksFacility)continue;
                        foundEarthworks=true;ImGui::PushID(static_cast<int>(i));
                        char facilityLabel[80];sprintf_s(facilityLabel,"Select facility (%.0f m away)",candidate.distance);
                        if(ImGui::Button(facilityLabel))Send(Action::ChooseBuilding,static_cast<float>(i));
                        ImGui::PopID();
                    }
                    if(!foundEarthworks)ImGui::TextColored({0.55f,0.25f,0.08f,1},"No Earthworks facility found nearby. Click closer to its centre.");
                } else if(s.workforce.selected&&s.workforce.buildingValid) {
                    if(s.workforce.building.earthworksFacility)ImGui::Text("Selected Earthworks facility %u.",s.workforce.building.id);
                    else ImGui::TextColored({0.55f,0.25f,0.08f,1},"Selected building is not an Earthworks facility.");
                } else ImGui::TextDisabled("Click near your facility after choosing this action.");
                const bool canAssign=s.projectStatus==ProjectStatus::Queued||s.projectStatus==ProjectStatus::Paused;
                ImGui::BeginDisabled(!canAssign||!s.workforce.selected||!s.workforce.buildingValid||!s.workforce.building.earthworksFacility);
                if(ImGui::Button(s.projectFacilityId?"Assign / reassign":"Assign selected facility"))Send(Action::AssignProject);
                ImGui::EndDisabled();
                if(s.projectFacilityId) {
                    ImGui::SameLine();ImGui::BeginDisabled(s.projectStatus!=ProjectStatus::Queued);if(ImGui::Button("Clear"))Send(Action::ClearProjectAssignment);ImGui::EndDisabled();
                    if(s.assignedFacilityFound)ImGui::Text("Assigned facility %u  |  %.1f m from project centre",s.projectFacilityId,s.projectFacilityDistance);
                    else ImGui::TextColored({0.55f,0.25f,0.08f,1},"Assigned facility %u is missing or unreadable.",s.projectFacilityId);
                } else ImGui::TextDisabled("No facility assigned.");
            }
            ImGui::EndTabItem();
        }
#ifdef EARTHWORKS_DEVELOPER_UI
        if(ImGui::BeginTabItem("Developer")) {
            if(ImGui::CollapsingHeader("Workforce inspector",ImGuiTreeNodeFlags_DefaultOpen)) {
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
                if(w.selected && !w.buildingValid)ImGui::TextColored({0.55f,0.25f,0.08f,1},"Building fields: UNAVAILABLE");
                if(w.selected && w.buildingValid) {
                    ImGui::Text("Building ID %u | slot %u | sample %u",w.building.id,w.building.slot,w.samples);
                    ImGui::Text("Origin X %.1f Z %.1f | category %d / %d",w.building.position.x,w.building.position.z,w.building.category,w.building.subtype);
                    if(w.occupantsValid)ImGui::Text("Occupant list: %u non-null / %u slots",w.occupants,w.occupantSlots);
                    else ImGui::TextColored({0.55f,0.25f,0.08f,1},"Occupant list: UNAVAILABLE");
                    if(w.parkedValid)ImGui::Text("Parked list: %u native slots",w.parkedSlots);
                    else ImGui::TextColored({0.55f,0.25f,0.08f,1},"Parked list: UNAVAILABLE");
                    ImGui::Text("Vehicle links: Home %u | Assignment %u | Parked %u",w.homeCount,w.assignedCount,w.parkedCount);
                    ImGui::Text("Showing %u of %u readable linked vehicles",w.rowCount,w.matchedCount);
                    for(unsigned i=0;i<w.rowCount;++i) {
                        const auto& v=w.vehicles[i];
                        ImGui::Text("ID %u | %s%s%s | %s | category %u",v.id,v.home?"H ":"",v.assigned?"A ":"",v.parked?"P":"",v.groundworksMachine&&v.earthworksAllowed?"machine candidate":"not a machine candidate",v.category);
                        ImGui::TextWrapped("%s",v.type);
                        if(v.groundworksMachine) {
                            if(v.nativeJobStateValid)ImGui::Text("Native job state: %d (raw)",v.nativeJobState);
                            else ImGui::TextColored({0.55f,0.25f,0.08f,1},"Native job state: UNAVAILABLE");
                            if(v.routeValid)ImGui::Text("Route stops: %u | first stop is this facility: %s",v.routeStopCount,v.firstRouteStopIsFacility?"yes":"no");
                            else ImGui::TextColored({0.55f,0.25f,0.08f,1},"Native route list: UNAVAILABLE");
                        }
                    }
                    ImGui::TextWrapped("Raw job state and route stops are diagnostics, not proof of availability or reservation.");
                }
                if(ImGui::Button("Log inspector snapshot"))Send(Action::LogInspector);
            }
            if(s.probe.available && ImGui::CollapsingHeader("Controlled terrain test")) {
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
                }
            }
            if(ImGui::CollapsingHeader("Native terrain-call probe")) {
                const auto& t=s.nativeTerrainCall;
                if(!t.bound) ImGui::TextColored({0.55f,0.25f,0.08f,1},"Native angle-limited terrain-call probe unavailable for this game build.");
                else {
                    ImGui::Text("Observed EditRaiseLowerAngleLimit calls: %llu",static_cast<unsigned long long>(t.calls));
                    if(t.calls) {
                        if(t.callerInGame) ImGui::Text("Last caller: SOVIET64.exe + 0x%llX",static_cast<unsigned long long>(t.caller));
                        else ImGui::Text("Last caller address: 0x%llX",static_cast<unsigned long long>(t.caller));
                        ImGui::Text("Last position X %.2f Z %.2f",t.x,t.z);
                        ImGui::Text("Brush parameters: %.4g, %.4g, %.4g, %.4g | flag %s | angle %.4g",t.brush[0],t.brush[1],t.brush[2],t.brush[3],t.flag?"true":"false",t.angleLimit);
                    } else ImGui::TextDisabled("No angle-limited terrain calls observed yet.");
                }
            }
            if(ImGui::CollapsingHeader("Terrain readings")) {
                ImGui::Text("Cursor: X %.1f  Z %.1f  Height %.2f m",s.cursor.x,s.cursor.z,s.cursor.y);
                ImGui::Text("Day %d, year %d, day progress %.3f",s.day,s.year,s.time);
                ImGui::Text("Grid: %d x %d samples",GridSide,GridSide);
            }
            ImGui::EndTabItem();
        }
#endif
        ImGui::EndTabBar();
    }
    ImGui::Separator();
    ImGui::TextWrapped("Map clicks pass through normally. Select area and Sample height capture only their required terrain clicks. Esc cancels map picking; Esc again closes this panel. Planning does not change terrain.");
    if (ImGui::Button("Close planning",{ImGui::GetContentRegionAvail().x,32})) open=false;
    RememberRect(&panelRect); panelRectValid=true; ImGui::End();
}
void CleanupTarget() { if (target) { target->Release(); target=nullptr; } }
void LoadPanelPaper()
{
    if(panelPaper || !device) return;
    wchar_t executable[MAX_PATH]{};
    const auto length=GetModuleFileNameW(nullptr,executable,MAX_PATH);
    if(!length || length>=MAX_PATH) return;
    std::wstring path(executable,length);
    const auto slash=path.find_last_of(L"\\/");
    if(slash==std::wstring::npos) return;
    path.resize(slash+1);
    path+=L"media_soviet\\menu\\strip.png";
    const auto init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(init) && init!=RPC_E_CHANGED_MODE) return;
    const bool uninitialize=SUCCEEDED(init);
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr)) hr=factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder);
    if(SUCCEEDED(hr)) hr=decoder->GetFrame(0,&frame);
    if(SUCCEEDED(hr)) hr=factory->CreateFormatConverter(&converter);
    if(SUCCEEDED(hr)) hr=converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
    UINT width=0,height=0;
    if(SUCCEEDED(hr)) hr=converter->GetSize(&width,&height);
    std::vector<BYTE> pixels;
    if(SUCCEEDED(hr) && width && height && static_cast<uint64_t>(width)*height<=16'000'000) {
        pixels.resize(static_cast<size_t>(width)*height*4);
        hr=converter->CopyPixels(nullptr,width*4,static_cast<UINT>(pixels.size()),pixels.data());
    } else if(SUCCEEDED(hr)) hr=E_INVALIDARG;
    if(SUCCEEDED(hr)) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=width; desc.Height=height; desc.MipLevels=1; desc.ArraySize=1;
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count=1;
        desc.Usage=D3D11_USAGE_IMMUTABLE; desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{}; data.pSysMem=pixels.data(); data.SysMemPitch=width*4;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        hr=device->CreateTexture2D(&desc,&data,&texture);
        if(SUCCEEDED(hr)) hr=device->CreateShaderResourceView(texture.Get(),nullptr,&panelPaper);
        if(SUCCEEDED(hr)) { panelPaperWidth=width; panelPaperHeight=height; }
    }
    if(uninitialize) CoUninitialize();
    if(FAILED(hr)) host->log("earthworks  native parchment texture unavailable; using color fallback");
}
void DrawPanelPaper(bool headerOnly)
{
    const auto pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
    auto* draw=ImGui::GetWindowDrawList();
    if(!panelPaper || !panelPaperWidth || !panelPaperHeight) {
        draw->AddRectFilled(pos,{pos.x+size.x,pos.y+size.y},IM_COL32(232,220,197,255),12.0f);
        draw->AddRectFilled(pos,{pos.x+size.x,pos.y+52.0f},IM_COL32(150,60,55,255),12.0f,ImDrawFlags_RoundCornersTop);
        return;
    }
    const ImTextureRef texture(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(panelPaper)));
    const ImVec2 uv0{52.0f/panelPaperWidth,20.0f/panelPaperHeight};
    const ImVec2 uv1{605.0f/panelPaperWidth,(headerOnly?63.0f:698.0f)/panelPaperHeight};
    draw->AddImageRounded(texture,pos,{pos.x+size.x,pos.y+size.y},uv0,uv1,IM_COL32_WHITE,12.0f,ImDrawFlags_RoundCornersAll);
}
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
    ImFontConfig fontConfig{};
    fontConfig.PixelSnapH=true;
    wchar_t windowsDirectory[MAX_PATH]{};
    const UINT windowsLength=GetWindowsDirectoryW(windowsDirectory,MAX_PATH);
    ImFont* gameFont=nullptr;
    if(windowsLength && windowsLength<MAX_PATH) {
        std::wstring fontPath(windowsDirectory,windowsLength);
        fontPath+=L"\\Fonts\\arial.ttf";
        const int pathLength=WideCharToMultiByte(CP_ACP,0,fontPath.c_str(),-1,nullptr,0,nullptr,nullptr);
        if(pathLength>0) {
            std::string ansiPath(static_cast<size_t>(pathLength),'\0');
            if(WideCharToMultiByte(CP_ACP,0,fontPath.c_str(),-1,ansiPath.data(),pathLength,nullptr,nullptr)>0)
                gameFont=io.Fonts->AddFontFromFileTTF(ansiPath.c_str(),16.0f,&fontConfig);
        }
    }
    if(!gameFont) { fontConfig.SizePixels=16.0f; gameFont=io.Fonts->AddFontDefault(&fontConfig); }
    io.FontDefault=gameFont;
    ImGui::StyleColorsLight();
    auto& style=ImGui::GetStyle();
    const ImVec4 ink{0.22f,0.16f,0.12f,1.0f},paper{0.91f,0.85f,0.74f,1.0f};
    const ImVec4 paperDark{0.76f,0.65f,0.49f,1.0f},oxblood{0.40f,0.10f,0.09f,1.0f};
    // Keep control text on the fixed dark-ink color; selected/pressed surfaces
    // stay light enough for it to remain readable.
    const ImVec4 red{0.82f,0.57f,0.48f,1.0f},redLight{0.89f,0.70f,0.60f,1.0f};
    style.Colors[ImGuiCol_Text]=ink;
    style.Colors[ImGuiCol_TextDisabled]={0.43f,0.36f,0.28f,1.0f};
    style.Colors[ImGuiCol_WindowBg]=paper;
    style.Colors[ImGuiCol_ChildBg]={0.88f,0.81f,0.69f,0.55f};
    style.Colors[ImGuiCol_Border]={0.36f,0.25f,0.16f,0.85f};
    style.Colors[ImGuiCol_FrameBg]={0.96f,0.92f,0.83f,1.0f};
    style.Colors[ImGuiCol_FrameBgHovered]={1.0f,0.97f,0.89f,1.0f};
    style.Colors[ImGuiCol_FrameBgActive]={0.99f,0.91f,0.76f,1.0f};
    style.Colors[ImGuiCol_TitleBg]=oxblood;
    style.Colors[ImGuiCol_TitleBgActive]=red;
    style.Colors[ImGuiCol_TitleBgCollapsed]=oxblood;
    style.Colors[ImGuiCol_Button]=paperDark;
    style.Colors[ImGuiCol_ButtonHovered]=redLight;
    style.Colors[ImGuiCol_ButtonActive]=red;
    style.Colors[ImGuiCol_Header]=paperDark;
    style.Colors[ImGuiCol_HeaderHovered]=redLight;
    style.Colors[ImGuiCol_HeaderActive]=red;
    style.Colors[ImGuiCol_Tab]=paperDark;
    style.Colors[ImGuiCol_TabHovered]=redLight;
    style.Colors[ImGuiCol_TabSelected]=red;
    style.Colors[ImGuiCol_TabSelectedOverline]=oxblood;
    style.Colors[ImGuiCol_TabDimmed]=paperDark;
    style.Colors[ImGuiCol_TabDimmedSelected]=paper;
    style.Colors[ImGuiCol_CheckMark]=oxblood;
    style.Colors[ImGuiCol_SliderGrab]=red;
    style.Colors[ImGuiCol_SliderGrabActive]=oxblood;
    style.Colors[ImGuiCol_Separator]={0.45f,0.32f,0.21f,0.55f};
    style.Colors[ImGuiCol_ScrollbarBg]={0.67f,0.56f,0.42f,0.30f};
    style.Colors[ImGuiCol_ScrollbarGrab]=paperDark;
    style.Colors[ImGuiCol_ScrollbarGrabHovered]=redLight;
    style.Colors[ImGuiCol_ScrollbarGrabActive]=red;
    style.WindowPadding={14,12}; style.FramePadding={10,5}; style.ItemSpacing={8,7};
    style.WindowRounding=12; style.ChildRounding=8; style.FrameRounding=6; style.TabRounding=6; style.GrabRounding=5;
    style.WindowBorderSize=1; style.FrameBorderSize=1;
    LoadPanelPaper();
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
#ifdef EARTHWORKS_DEVELOPER_UI
    DrawProbe(snapshot);
#endif
    ImGui::GetBackgroundDrawList()->PopClipRect();
    DrawInterface(snapshot);
    if(snapshot.mode==PickMode::None) pickCancelledByEscape=false;
    mapPickCapture=panelOpen && !panelMinimized && snapshot.mode!=PickMode::None && !pickCancelledByEscape;
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
    catch (...) { panelOpen=false; mouseCapture=false; keyboardCapture=false; leftRelease=false; mapPickCapture=false; escapeCapture=false; host->log("earthworks  overlay exception; input released"); }
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
