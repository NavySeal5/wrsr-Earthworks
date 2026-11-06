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
#include "game_underlay.h"
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
bool gameUnderlayInstalled = false;
bool previewLinesUnderlay = false;
ID3D11Device* device = nullptr;
ID3D11DeviceContext* context = nullptr;
ID3D11RenderTargetView* target = nullptr;
ID3D11ShaderResourceView* panelPaper = nullptr;
UINT panelPaperWidth = 0, panelPaperHeight = 0;
ID3D11ShaderResourceView* gameCursorArt = nullptr;
UINT gameCursorArtWidth = 0, gameCursorArtHeight = 0;
ID3D11ShaderResourceView* siteCancelIcon = nullptr;
ID3D11ShaderResourceView* siteRenameIcon = nullptr;
ID3D11ShaderResourceView* projectNewIcon = nullptr;
ID3D11ShaderResourceView* projectDeleteIcon = nullptr;
ID3D11ShaderResourceView* targetHeightInfoIcon = nullptr;
HCURSOR cursorToRestore = nullptr;
bool systemCursorHiddenForPanel = false;
IDXGISwapChain* gameChain = nullptr;
ImGuiContext* gui = nullptr;
HWND gameWindow = nullptr;
WNDPROC originalWndProc = nullptr;
std::recursive_mutex guiMutex;
RECT buttonRect{}, panelRect{}, popupRect{};
RECT editorRect{};
bool panelRectValid = false;
bool popupRectValid = false;
bool editorRectValid = false;
bool panelMinimized = false;
bool restorePanelSize = false;
float expandedPanelHeight = 640.0f;
std::atomic<bool> editorWasDragged{false};
bool escapeKeyDown = false;
bool escapeOwned = false;
bool pickCancelledByEscape = false;
bool targetHeightEditing = false;
bool nameTextEditing = false;
bool showAllFacilities = false;
std::atomic<bool> areaPickCapture{false};
uint64_t frameGeneration = 0;
thread_local int presentDepth = 0;
const ImU32 cutColor = IM_COL32(255,159,67,255), fillColor = IM_COL32(62,221,166,255);
void Send(Action action, float value = 0)
{
    if(action==Action::Select || action==Action::NewSite || action==Action::Sample || action==Action::SampleSiteTarget || action==Action::InspectBuilding) pickCancelledByEscape=false;
    Enqueue({action,value,frameGeneration});
}
void SendProject(Action action,uint64_t projectId) { Enqueue({action,0,frameGeneration,projectId}); }
void SendProjectValue(Action action,uint64_t projectId,float value)
{
    Command command{};command.action=action;command.value=value;command.projectId=projectId;command.generation=frameGeneration;Enqueue(command);
}
void SendProjectText(Action action,uint64_t projectId,const char* text)
{
    Command command{};command.action=action;command.generation=frameGeneration;command.projectId=projectId;
    if(text)strncpy_s(command.text,text,_TRUNCATE);
    Enqueue(command);
}
void SendProjectAction(Action action,uint64_t projectId)
{
    SendProject(action,projectId);
}
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
void NativeTooltip(const char* text)
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,{4.0f,2.0f});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,1.0f);
    ImGui::PushStyleColor(ImGuiCol_PopupBg,{0.96f,0.88f,0.77f,1.0f});
    ImGui::PushStyleColor(ImGuiCol_Border,{0.58f,0.43f,0.30f,0.85f});
    ImGui::PushStyleColor(ImGuiCol_Text,{0.25f,0.21f,0.17f,1.0f});
    ImGui::BeginTooltip();ImGui::TextUnformatted(text);ImGui::EndTooltip();
    ImGui::PopStyleColor(3);ImGui::PopStyleVar(3);
}
bool GameIconButton(const char* id,ID3D11ShaderResourceView* texture,const char* fallback,const char* tooltip,ImVec2 size={30,30})
{
    const auto pos=ImGui::GetCursorScreenPos();
    const bool hovered=ImGui::IsMouseHoveringRect(pos,{pos.x+size.x,pos.y+size.y},true);
    bool pressed=false;
    if(!texture)pressed=ImGui::Button(fallback,size);
    else {
        const ImTextureRef image(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture)));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,0});
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button,{0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0,0,0,0});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,{0,0,0,0});
        pressed=ImGui::ImageButton(id,image,size,{0,0},{1,1},hovered?ImVec4{1,1,1,1}:ImVec4{0.82f,0.82f,0.82f,1});
        ImGui::PopStyleColor(3);ImGui::PopStyleVar(2);
    }
    if(hovered&&tooltip) NativeTooltip(tooltip);
    return pressed;
}
bool Hit(const RECT& rect, LPARAM point)
{
    const POINT p = {GET_X_LPARAM(point),GET_Y_LPARAM(point)};
    return PtInRect(&rect,p) != FALSE;
}
bool CursorOnEditingSite(const Snapshot& s);
bool KeyboardShortcutPassesThrough(UINT message,WPARAM key)
{
    if(message==WM_SYSKEYDOWN||message==WM_SYSKEYUP||message==WM_SYSCHAR||message==WM_SYSDEADCHAR)return true;
    return key==VK_ESCAPE||(key>=VK_F1&&key<=VK_F24)||key==VK_PAUSE||key==VK_SNAPSHOT;
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
    // The render pass normally updates keyboardCapture after drawing the
    // widgets. Set it here too, before the game can poll C3D_INPUT for this
    // key, to cover the first key typed immediately after focusing a field.
    if(panelOpen && ImGui::GetIO().WantTextInput) keyboardCapture=true;
    const bool over = Hit(buttonRect,lParam) || (panelRectValid && Hit(panelRect,lParam)) || (popupRectValid && Hit(popupRect,lParam)) || (editorRectValid && Hit(editorRect,lParam));
    if (message == WM_KILLFOCUS) { mouseCapture=false; keyboardCapture=false; leftRelease=false; escapeKeyDown=false; escapeOwned=false; escapeCapture=false; if(systemCursorHiddenForPanel&&cursorToRestore)SetCursor(cursorToRestore);systemCursorHiddenForPanel=false;cursorToRestore=nullptr; }
    bool escapeMessage=false;
    if(message==WM_KEYDOWN && wParam==VK_ESCAPE) {
        if(!escapeKeyDown) {
            escapeKeyDown=true;
            if(panelOpen) {
                escapeOwned=true;
                escapeCapture=true;
                if(areaPickCapture) { mapPickCapture=false;areaPickCapture=false;pickCancelledByEscape=true; Send(Action::CancelPick); }
                else { panelOpen=false; panelRectValid=false; keyboardCapture=false; pickCancelledByEscape=false;Send(Action::CloseSiteEditor);host->log("earthworks  planning panel closed with Escape"); }
            }
        }
        escapeMessage=escapeOwned;
    } else if(message==WM_KEYUP && wParam==VK_ESCAPE) {
        escapeKeyDown=false;
        escapeMessage=escapeOwned||keyboardCapture;
        if(escapeOwned) escapeCapture=false;
        escapeOwned=false;
    }
    if (message == WM_MOUSEMOVE) mouseCapture=over;
    const bool leftMessage = message == WM_LBUTTONDOWN || message == WM_LBUTTONUP || message == WM_LBUTTONDBLCLK;
    if(message==WM_LBUTTONDOWN&&panelOpen&&editorRectValid&&!editorWasDragged.load()&&GetForegroundWindow()==gameWindow) {
        const auto snapshot=ReadSnapshot();
        if(snapshot.siteEditorOpen&&!snapshot.creatingSite&&snapshot.mode==PickMode::None) {
            const bool insidePanel=(panelRectValid&&Hit(panelRect,lParam))||(popupRectValid&&Hit(popupRect,lParam));
            const bool insideEditor=Hit(editorRect,lParam);
            if(!insidePanel&&!insideEditor&&!CursorOnEditingSite(snapshot))Send(Action::CloseSiteEditor);
        }
    }
    const bool mapPick = mapPickCapture && !over && WorldHit(lParam);
    const bool leftOwned = mouseCapture || leftRelease || over || mapPick;
    if (message == WM_LBUTTONDOWN && leftOwned) {
        leftRelease = true;
        if (mapPick && GetForegroundWindow() == gameWindow) Send(Action::Pick);
    }
    if (message == WM_LBUTTONUP) leftRelease = false;
    const bool otherMouse = message == WM_RBUTTONDOWN || message == WM_RBUTTONUP || message == WM_RBUTTONDBLCLK
        || message == WM_MBUTTONDOWN || message == WM_MBUTTONUP || message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL;
    const bool keyboardMessage=message==WM_KEYDOWN||message==WM_KEYUP||message==WM_CHAR||message==WM_SYSKEYDOWN
        ||message==WM_SYSKEYUP||message==WM_SYSCHAR||message==WM_DEADCHAR||message==WM_SYSDEADCHAR
        ||message==WM_UNICHAR||message==WM_IME_STARTCOMPOSITION||message==WM_IME_COMPOSITION||message==WM_IME_ENDCOMPOSITION;
    const bool suppressTypedShortcut=keyboardMessage&&keyboardCapture&&!KeyboardShortcutPassesThrough(message,wParam);
    const bool swallow = (leftMessage && leftOwned) || (otherMouse && mouseCapture) || suppressTypedShortcut || escapeMessage;
    ImGui::SetCurrentContext(previous);
    if (swallow) return TRUE;
    if(message==WM_MOUSEMOVE) {
        const LRESULT result=CallWindowProcW(originalWndProc,window,message,wParam,lParam);
        const bool overPanel=panelOpen&&((panelRectValid&&Hit(panelRect,lParam))||(popupRectValid&&Hit(popupRect,lParam))||(editorRectValid&&Hit(editorRect,lParam)));
        if(overPanel) {
            if(HCURSOR cursor=GetCursor())cursorToRestore=cursor;
            SetCursor(nullptr);systemCursorHiddenForPanel=true;
        } else if(systemCursorHiddenForPanel) {
            if(GetCursor()==nullptr&&cursorToRestore)SetCursor(cursorToRestore);
            systemCursorHiddenForPanel=false;cursorToRestore=nullptr;
        }
        return result;
    }
    if(message==WM_SETCURSOR) {
        const LRESULT result=CallWindowProcW(originalWndProc,window,message,wParam,lParam);
        POINT point{};
        if(GetCursorPos(&point)&&ScreenToClient(window,&point)&&panelOpen
            &&((panelRectValid&&Hit(panelRect,MAKELPARAM(point.x,point.y)))||(popupRectValid&&Hit(popupRect,MAKELPARAM(point.x,point.y)))||(editorRectValid&&Hit(editorRect,MAKELPARAM(point.x,point.y))))) {
            if(HCURSOR cursor=GetCursor())cursorToRestore=cursor;
            SetCursor(nullptr);systemCursorHiddenForPanel=true;return TRUE;
        }
        systemCursorHiddenForPanel=false;cursorToRestore=nullptr;
        return result;
    }
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
    if(previewLinesUnderlay) return;
    ImVec2 p,q;
    if (Project(s,a,&p) && Project(s,b,&q)) ImGui::GetBackgroundDrawList()->AddLine(p,q,color,thickness);
}
void Shade(const Snapshot& s,const Point* triangle,const float* depth,float sign,ImU32 color)
{
    if(gameUnderlayInstalled)return;
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
        if(project.status==ProjectStatus::Queued||project.status==ProjectStatus::Running
            ||project.status==ProjectStatus::Waiting||project.status==ProjectStatus::Paused) {
            if(project.id==s.editingSiteId)DrawQueuedWireframe(s,s.queuedPlan);
            else DrawQueuedOutline(s,project.a,project.b,project.target,IM_COL32(175,180,190,205));
        } else if(project.status==ProjectStatus::Completed)
            DrawQueuedOutline(s,project.a,project.b,project.target,IM_COL32(100,190,155,190));
        else if(project.status==ProjectStatus::Cancelled)
            DrawQueuedOutline(s,project.a,project.b,project.target,IM_COL32(170,135,135,170));
    }
    const auto& p=s.plan;
    if (!p.first) return;
    if (!p.complete) {
        ImVec2 first;
        if (!gameUnderlayInstalled && Project(s,p.a,&first)) ImGui::GetBackgroundDrawList()->AddCircle(first,7,IM_COL32(255,235,135,255),16,2);
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
bool CursorOnVisibleSite(const Snapshot& s)
{
    if(!s.cursorValid)return false;
    for(size_t i=0;i<s.projectCount;++i) {
        const auto& site=s.projects[i];
        if(s.cursor.x>=std::min(site.a.x,site.b.x)&&s.cursor.x<=std::max(site.a.x,site.b.x)
            &&s.cursor.z>=std::min(site.a.z,site.b.z)&&s.cursor.z<=std::max(site.a.z,site.b.z))return true;
    }
    return false;
}
bool CursorOnEditingSite(const Snapshot& s)
{
    if(!s.cursorValid||!s.editingSiteId)return false;
    for(size_t i=0;i<s.projectCount;++i) {
        const auto& site=s.projects[i];
        if(site.id==s.editingSiteId)
            return s.cursor.x>=std::min(site.a.x,site.b.x)&&s.cursor.x<=std::max(site.a.x,site.b.x)
                &&s.cursor.z>=std::min(site.a.z,site.b.z)&&s.cursor.z<=std::max(site.a.z,site.b.z);
    }
    return false;
}
void DrawSiteEditor(const Snapshot& s,ImVec2 display);
void DrawPanelPaper(bool headerOnly);
void DrawSiteEditorPaper(ImVec2 pos,ImVec2 size,bool headerOnly);
void DrawInterface(const Snapshot& s)
{
    targetHeightEditing = false;
    nameTextEditing = false;
    popupRectValid=false;
    frameGeneration=s.generation;
    if (!s.ready) { panelOpen=false; panelRectValid=false; popupRectValid=false; buttonRect={}; return; }
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
    if (!panelOpen) { panelRectValid=false;popupRectValid=false;editorRectValid=false; return; }
    const float panelWidth=std::min(580.0f,std::max(480.0f,display.x-20.0f));
    const float panelHeight=std::min(850.0f,std::max(360.0f,display.y-180.0f));
    constexpr float headerHeight=56.0f;
    ImGui::SetNextWindowPos({std::max(10.0f,display.x-panelWidth-20.0f),155},ImGuiCond_FirstUseEver);
    if(panelMinimized) ImGui::SetNextWindowSize({panelWidth,headerHeight+2.0f},ImGuiCond_Always);
    else if(restorePanelSize) { ImGui::SetNextWindowSize({panelWidth,expandedPanelHeight},ImGuiCond_Always); restorePanelSize=false; }
    else ImGui::SetNextWindowSize({panelWidth,panelHeight},ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints({std::min(560.0f,std::max(480.0f,display.x-20.0f)),44},{std::min(800.0f,std::max(480.0f,display.x-20.0f)),std::max(300.0f,display.y-180.0f)});
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
    panelDraw->AddText({panelPos.x+11.0f,panelPos.y+29.5f-ImGui::GetTextLineHeight()*0.5f},IM_COL32(255,255,255,255),"Earthworks | Projects");
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
        panelOpen=false; mapPickCapture=false;areaPickCapture=false;leftRelease=true; panelRectValid=false;editorRectValid=false;Send(Action::CloseSiteEditor);
        ImGui::End();
        host->log("earthworks  planning panel closed; native left clicks released after button up");
        return;
    }
    if(collapsedThisFrame || panelMinimized) {
        RememberRect(&panelRect); panelRectValid=true; ImGui::End();DrawSiteEditor(s,display);return;
    }
    ImGui::SetCursorScreenPos({panelPos.x+ImGui::GetStyle().WindowPadding.x,panelPos.y+headerHeight+ImGui::GetStyle().WindowPadding.y});
    static char nameBuffer[MaxSavedNameBytes+1]{};
    static Action renameAction=Action::RenameNamedProject;
    static uint64_t renameTarget=0;
    static uint64_t deleteTarget=0;
    if(ImGui::BeginTabBar("Earthworks tabs")) {
        if(ImGui::BeginTabItem("Projects and Sites")) {
            ImGui::AlignTextToFramePadding();ImGui::Text("Project");ImGui::SameLine();
            ImGui::SetNextItemWidth(225);
            const char* selectedProject=s.projectContainerName[0]?s.projectContainerName:"No Project selected";
            if(ImGui::BeginCombo("##Project selector",selectedProject)) {
                for(size_t i=0;i<s.namedProjectCount;++i) {
                    const auto& named=s.namedProjects[i];const bool selected=named.id==s.selectedNamedProjectId;
                    if(ImGui::Selectable(named.name,selected))SendProject(Action::SelectNamedProject,named.id);
                    if(selected)ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if(GameIconButton("##NewProject",projectNewIcon,"New","New Project")) {
                nameBuffer[0]=0;renameAction=Action::CreateNamedProject;renameTarget=0;ImGui::OpenPopup("Project name");
            }
            ImGui::SameLine();ImGui::BeginDisabled(!s.selectedNamedProjectId);
            if(GameIconButton("##RenameProject",siteRenameIcon,"Rename","Rename Project")) {
                strncpy_s(nameBuffer,s.projectContainerName,_TRUNCATE);renameAction=Action::RenameNamedProject;
                renameTarget=s.selectedNamedProjectId;ImGui::OpenPopup("Project name");
            }
            ImGui::SameLine();
            if(GameIconButton("##DeleteProject",projectDeleteIcon,"Delete","Delete Project")) {
                deleteTarget=s.selectedNamedProjectId;ImGui::OpenPopup("Delete Project");
            }
            ImGui::EndDisabled();
            ImGui::Spacing();

            if(ImGui::BeginPopupModal("Project name",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
                RememberRect(&popupRect);popupRectValid=true;
                ImGui::Text(renameAction==Action::CreateNamedProject?"Create Project":"Rename Project");
                ImGui::SetNextItemWidth(300);ImGui::InputText("Name",nameBuffer,sizeof(nameBuffer));nameTextEditing|=ImGui::IsItemActive();
                if(ImGui::Button("Save")) {SendProjectText(renameAction,renameTarget,nameBuffer);ImGui::CloseCurrentPopup();}
                ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            if(ImGui::BeginPopupModal("Delete Project",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
                RememberRect(&popupRect);popupRectValid=true;
                ImGui::TextWrapped("Delete this Project and its completed or cancelled Site records? Unfinished Sites must be finished or cancelled first.");
                if(ImGui::Button("Delete")) {SendProject(Action::DeleteNamedProject,deleteTarget);ImGui::CloseCurrentPopup();}
                ImGui::SameLine();if(ImGui::Button("Keep Project"))ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }

            {
                if(!s.selectedNamedProjectId)ImGui::TextDisabled("Create a Project to manage Sites.");
                else if(!s.projectCount)ImGui::TextDisabled("No Sites in this Project yet.");
                bool openSiteRenamePopup=false;
                ImGui::BeginChild("Sites list",{0,300},ImGuiChildFlags_Borders);
                ImGui::PushStyleVar(ImGuiStyleVar_CellPadding,{6.0f,3.0f});
                if(s.projectCount&&ImGui::BeginTable("Site rows",6,ImGuiTableFlags_BordersOuter|ImGuiTableFlags_BordersInnerV|ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_ScrollY)) {
                    ImGui::TableSetupColumn("Site",ImGuiTableColumnFlags_WidthStretch,2.5f);
                    ImGui::TableSetupColumn("Progress",ImGuiTableColumnFlags_WidthStretch,1.15f);
                    ImGui::TableSetupColumn("Priority",ImGuiTableColumnFlags_WidthStretch,1.05f);
                    ImGui::TableSetupColumn("Work",ImGuiTableColumnFlags_WidthStretch,1.05f);
                    ImGui::TableSetupColumn("##CancelHeader",ImGuiTableColumnFlags_WidthFixed,40.0f);
                    ImGui::TableSetupColumn("##RenameHeader",ImGuiTableColumnFlags_WidthFixed,40.0f);
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers,32.0f);
                    ImGui::TableNextColumn();ImGui::TextUnformatted("Site");
                    ImGui::TableNextColumn();ImGui::TextUnformatted("Progress");
                    ImGui::TableNextColumn();ImGui::TextUnformatted("Priority");
                    ImGui::TableNextColumn();ImGui::TextUnformatted("Work");
                    ImGui::TableNextColumn();
                    ImGui::Dummy({32,24});
                    if(ImGui::IsItemHovered()) NativeTooltip("Cancel");
                    ImGui::TableNextColumn();
                    ImGui::Dummy({32,24});
                    if(ImGui::IsItemHovered()) NativeTooltip("Rename");
                    auto drawIconButton=[](const char* id,ID3D11ShaderResourceView* texture,const char* tooltip) {
                        if(!texture)return false;
                        constexpr float iconSize=30.0f;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX()+std::max(0.0f,(ImGui::GetContentRegionAvail().x-iconSize)*0.5f));
                        const auto pos=ImGui::GetCursorScreenPos();
                        const bool hovered=ImGui::IsMouseHoveringRect(pos,{pos.x+iconSize,pos.y+iconSize},true);
                        const ImTextureRef image(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(texture)));
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,0});
                        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.0f);
                        ImGui::PushStyleColor(ImGuiCol_Button,{0,0,0,0});
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0,0,0,0});
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive,{0,0,0,0});
                        const bool pressed=ImGui::ImageButton(id,image,{iconSize,iconSize},{0,0},{1,1},hovered?ImVec4{1,1,1,1}:ImVec4{0.82f,0.82f,0.82f,1});
                        ImGui::PopStyleColor(3);ImGui::PopStyleVar(2);
                        if(hovered&&tooltip) NativeTooltip(tooltip);
                        return pressed;
                    };
                    auto centerCellItem=[](float height) {
                        constexpr float rowHeight=44.0f,cellPaddingY=3.0f;
                        const float innerHeight=rowHeight-2.0f*cellPaddingY;
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY()+std::max(0.0f,(innerHeight-height)*0.5f));
                    };
                    for(size_t i=0;i<s.projectCount;++i) {
                        const auto& site=s.projects[i];ImGui::PushID(static_cast<int>(site.id));
                        ImGui::TableNextRow(ImGuiTableRowFlags_None,44.0f);
                        ImGui::TableNextColumn();
                        char label[80];sprintf_s(label,"%s  |  %s",site.name,ProjectStatusName(site.status));
                        centerCellItem(38.0f);
                        if(ImGui::Selectable("##SelectSiteRow",s.projectId==site.id,ImGuiSelectableFlags_None,{0,38}))SendProject(Action::SelectProject,site.id);
                        const auto siteLabelMin=ImGui::GetItemRectMin();
                        const auto siteLabelMax=ImGui::GetItemRectMax();
                        const float siteLabelY=siteLabelMin.y+(siteLabelMax.y-siteLabelMin.y-ImGui::GetTextLineHeight())*0.5f;
                        ImGui::GetWindowDrawList()->AddText({siteLabelMin.x+4.0f,siteLabelY},ImGui::GetColorU32(ImGuiCol_Text),label);
                        ImGui::TableNextColumn();char progressLabel[24];sprintf_s(progressLabel,"%.0f%%",site.progress*100.0f);centerCellItem(28.0f);ImGui::ProgressBar(site.progress,{ImGui::GetContentRegionAvail().x,28},progressLabel);
                        ImGui::TableNextColumn();
                        centerCellItem(32.0f);
                        const bool movable=ProjectCanMove(site.status);const float moveWidth=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*0.5f;
                        ImGui::BeginDisabled(!movable);if(ImGui::Button("↑",{moveWidth,32}))SendProject(Action::MoveProjectUp,site.id);ImGui::SameLine();if(ImGui::Button("↓",{moveWidth,32}))SendProject(Action::MoveProjectDown,site.id);ImGui::EndDisabled();
                        ImGui::TableNextColumn();Action workAction=Action::StartProject;const char* workLabel="Done";bool workEnabled=false;
                        if(site.status==ProjectStatus::Queued){workAction=Action::StartProject;workLabel="Start";workEnabled=s.assignedFacilityCount!=0;}
                        else if(site.status==ProjectStatus::Running||site.status==ProjectStatus::Waiting){workAction=Action::PauseProject;workLabel="Pause";workEnabled=true;}
                        else if(site.status==ProjectStatus::Paused){workAction=site.needsReview?Action::ReviewResumeProject:Action::ResumeProject;workLabel=site.needsReview?"Review":"Resume";workEnabled=s.assignedFacilityCount!=0;}
                        else if(site.status==ProjectStatus::Completed){workAction=Action::RemoveProject;workLabel="Remove";workEnabled=true;}
                        centerCellItem(32.0f);ImGui::BeginDisabled(!workEnabled);if(ImGui::Button(workLabel,{ImGui::GetContentRegionAvail().x,32})) {
                            if(workAction==Action::RemoveProject)SendProject(workAction,site.id);else SendProjectAction(workAction,site.id);
                        }ImGui::EndDisabled();
                        ImGui::TableNextColumn();const bool cancellable=site.status==ProjectStatus::Queued||site.status==ProjectStatus::Running||site.status==ProjectStatus::Waiting||site.status==ProjectStatus::Paused;
                        const bool removable=site.status==ProjectStatus::Cancelled;
                        centerCellItem(30.0f);
                        ImGui::BeginDisabled(!cancellable&&!removable);
                        if(siteCancelIcon) {
                            if(drawIconButton("##CancelSite",siteCancelIcon,removable?"Delete Site":"Cancel Site"))
                                SendProjectAction(removable?Action::RemoveProject:Action::CancelProject,site.id);
                        } else if(ImGui::Button("×",{32,32}))
                            SendProjectAction(removable?Action::RemoveProject:Action::CancelProject,site.id);
                        ImGui::EndDisabled();
                        ImGui::TableNextColumn();
                        centerCellItem(30.0f);
                        if(siteRenameIcon) {
                            if(drawIconButton("##RenameSite",siteRenameIcon,"Rename Site")) {strncpy_s(nameBuffer,site.name,_TRUNCATE);renameAction=Action::RenameSite;renameTarget=site.id;openSiteRenamePopup=true;}
                        } else if(ImGui::Button("R",{32,32})) {strncpy_s(nameBuffer,site.name,_TRUNCATE);renameAction=Action::RenameSite;renameTarget=site.id;openSiteRenamePopup=true;}
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::PopStyleVar();
                ImGui::EndChild();
                if(openSiteRenamePopup)ImGui::OpenPopup("Site name");
                ImGui::BeginDisabled(!s.selectedNamedProjectId);
                const char* draftAction=(s.creatingSite||s.plan.first)?"Continue draft":"New Site";
                if(ImGui::Button(draftAction,{ImGui::GetContentRegionAvail().x,36}))Send(Action::NewSite);
                ImGui::EndDisabled();
                ImGui::SeparatorText("Facilities");
                if(ImGui::SmallButton(showAllFacilities?"Hide other Projects":"Show all facilities"))showAllFacilities=!showAllFacilities;
                ImGui::SameLine();ImGui::TextDisabled("%u found",s.facilityCandidateCount);
                if(ImGui::BeginTable("facilityTable",4,ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg|ImGuiTableFlags_SizingStretchProp|ImGuiTableFlags_ScrollY,{0,190})) {
                    ImGui::TableSetupColumn("Facility",ImGuiTableColumnFlags_WidthStretch,2.2f);
                    ImGui::TableSetupColumn("Project",ImGuiTableColumnFlags_WidthStretch,1.6f);
                    ImGui::TableSetupColumn("Machines",ImGuiTableColumnFlags_WidthFixed,84.0f);
                    ImGui::TableSetupColumn("",ImGuiTableColumnFlags_WidthFixed,72.0f);
                    ImGui::TableHeadersRow();
                    unsigned displayed=0;
                    for(unsigned i=0;i<s.facilityCandidateCount;++i) {
                        const auto& facility=s.facilityCandidates[i];
                        const bool assignedElsewhere=facility.assignedProjectId&&facility.assignedProjectId!=s.selectedNamedProjectId;
                        if(assignedElsewhere&&!showAllFacilities)continue;
                        ++displayed;ImGui::PushID(static_cast<int>(facility.id));ImGui::TableNextRow();
                        if(assignedElsewhere)ImGui::BeginDisabled(true);
                        const char* size=strstr(facility.assetName,"Large")?"Large":strstr(facility.assetName,"Medium")?"Medium":"Small";
                        ImGui::TableNextColumn();ImGui::Text("%s Earthworks Facility #%u",size,facility.id);
                        ImGui::TableNextColumn();
                        if(assignedElsewhere)ImGui::TextUnformatted(facility.assignedProjectName);
                        else if(facility.assignedProjectId)ImGui::TextUnformatted(s.projectContainerName);
                        else ImGui::TextDisabled("Unassigned");
                        ImGui::TableNextColumn();
                        const auto assigned=std::find_if(s.assignedFacilities.begin(),s.assignedFacilities.begin()+s.assignedFacilityCount,[&](const ProjectFacilityBrief& row){return row.id==facility.id;});
                        if(assigned!=s.assignedFacilities.begin()+s.assignedFacilityCount&&assigned->machineCountValid)
                            ImGui::Text("%u",assigned->eligibleMachines);
                        else ImGui::TextDisabled("—");
                        ImGui::TableNextColumn();
                        if(assignedElsewhere)ImGui::TextDisabled("Assigned");
                        else if(facility.assignedProjectId==s.selectedNamedProjectId) {
                            if(ImGui::SmallButton("Remove"))SendProject(Action::RemoveProjectFacility,facility.id);
                        } else {
                            ImGui::BeginDisabled(!s.selectedNamedProjectId);
                            if(ImGui::SmallButton("Assign"))SendProject(Action::AddProjectFacility,facility.id);
                            ImGui::EndDisabled();
                        }
                        if(assignedElsewhere)ImGui::EndDisabled();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                    if(!displayed)ImGui::TextDisabled("No unassigned or current-Project facilities found.");
                }
                if(s.facilityListTruncated)ImGui::TextDisabled("Showing the first %u facilities.",WorldEarthworksFacilities);
                else if(!s.facilityCandidateCount)ImGui::TextDisabled("No existing Earthworks facilities found in this republic.");
            }
            if(ImGui::BeginPopupModal("Site name",nullptr,ImGuiWindowFlags_AlwaysAutoResize)) {
                RememberRect(&popupRect);popupRectValid=true;
                ImGui::Text("Rename Site");ImGui::SetNextItemWidth(300);ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{8.0f,6.0f});ImGui::InputText("Name",nameBuffer,sizeof(nameBuffer));ImGui::PopStyleVar();nameTextEditing|=ImGui::IsItemActive();
                if(ImGui::Button("Save")) {SendProjectText(Action::RenameSite,renameTarget,nameBuffer);ImGui::CloseCurrentPopup();}
                ImGui::SameLine();if(ImGui::Button("Cancel"))ImGui::CloseCurrentPopup();ImGui::EndPopup();
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
            ImGui::EndTabItem();
        }
#endif
        ImGui::EndTabBar();
    }
    if (ImGui::Button("Close planning",{ImGui::GetContentRegionAvail().x,32})) {
        panelOpen=false;mapPickCapture=false;areaPickCapture=false;leftRelease=true;panelRectValid=false;editorRectValid=false;Send(Action::CloseSiteEditor);
    }
    RememberRect(&panelRect); panelRectValid=true; ImGui::End();DrawSiteEditor(s,display);
}
void DrawSiteEditor(const Snapshot& s,ImVec2 display)
{
    static char renameBuffer[MaxSavedNameBytes+1]{};
    static char draftNameBuffer[MaxSavedNameBytes+1]{};
    static bool draftNameWasActive=false;
    static bool renameEditing=false,editorMinimized=false;
    static uint64_t renameEditingSite=0;
    if(!panelOpen||!s.siteEditorOpen||!s.editorPlan.complete) {
        editorRectValid=false;
        if(!s.siteEditorOpen)editorWasDragged=false;
        if(!s.creatingSite)draftNameWasActive=false;
        return;
    }
    if(s.creatingSite&&!draftNameWasActive)strncpy_s(draftNameBuffer,s.siteDraftName,_TRUNCATE);
    draftNameWasActive=s.creatingSite;
    if(!s.creatingSite&&renameEditingSite!=s.editingSiteId) {renameEditingSite=s.editingSiteId;renameEditing=false;}
    const Plan& plan=s.editorPlan;
    const float width=400.0f,fullHeight=(s.creatingSite?400.0f:440.0f)+(s.creatingSite&&s.siteDraftError[0]?56.0f:0.0f);
    double groundTotal=0;for(float value:plan.ground)groundTotal+=value;
    const float groundMean=plan.ground.empty()?plan.target:static_cast<float>(groundTotal/static_cast<double>(plan.ground.size()));
    Point anchor{(plan.a.x+plan.b.x)*0.5f,groundMean,(plan.a.z+plan.b.z)*0.5f};
    ImVec2 screen{};
    if(!Project(s,anchor,&screen))screen={display.x*0.5f,display.y*0.5f};
    static uint64_t trackedSite=0;
    static ImVec2 editorPosition{};
    const uint64_t siteKey=s.creatingSite?~uint64_t{0}:s.editingSiteId;
    if(siteKey!=trackedSite) {
        trackedSite=siteKey;
        editorMinimized=false;
        float x=screen.x+26.0f,y=screen.y-95.0f;
        if(x+width>display.x-12.0f)x=screen.x-width-26.0f;
        x=std::clamp(x,12.0f,std::max(12.0f,display.x-width-12.0f));
        y=std::clamp(y,70.0f,std::max(70.0f,display.y-fullHeight-90.0f));
        RECT proposed{static_cast<LONG>(x),static_cast<LONG>(y),static_cast<LONG>(x+width),static_cast<LONG>(y+fullHeight)};
        RECT overlap{};
        if(panelRectValid&&IntersectRect(&overlap,&proposed,&panelRect)) {
            x=screen.x-width-26.0f;
            if(x<12.0f)x=screen.x+26.0f;
            x=std::clamp(x,12.0f,std::max(12.0f,display.x-width-12.0f));
        }
        editorPosition={x,y};
    }
    const float height=editorMinimized?52.0f:fullHeight;
    ImGui::SetNextWindowPos(editorPosition,ImGuiCond_Always);
    ImGui::SetNextWindowSize({width,height},ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,{0,0,0,0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f);
    ImGui::Begin("##EarthworksSiteEditor",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoSavedSettings);
    ImGui::PopStyleVar();ImGui::PopStyleColor();
    const auto pos=ImGui::GetWindowPos(),size=ImGui::GetWindowSize();
    DrawSiteEditorPaper(pos,size,editorMinimized);
    ImGui::SetCursorScreenPos({pos.x+2.0f,pos.y+2.0f});
    ImGui::InvisibleButton("##DragSiteEditorHeader",{size.x-80.0f,44.0f});
    if(ImGui::IsItemActive()&&ImGui::GetIO().MouseDown[0]) {
        const ImVec2 delta=ImGui::GetIO().MouseDelta;
        if(delta.x!=0.0f||delta.y!=0.0f)editorWasDragged=true;
        editorPosition.x+=delta.x;editorPosition.y+=delta.y;
        ImGui::SetWindowPos(editorPosition);
    }
    const auto movedPos=ImGui::GetWindowPos();
    ImGui::GetWindowDrawList()->AddText({movedPos.x+10.0f,movedPos.y+27.0f-ImGui::GetTextLineHeight()*0.5f},IM_COL32(255,255,255,255),
        s.creatingSite?"New Site":"Site editor");
    ImGui::PushStyleColor(ImGuiCol_Text,{0.20f,0.145f,0.105f,1.0f});
    ImGui::PushStyleColor(ImGuiCol_Button,{0.77f,0.64f,0.47f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0.86f,0.73f,0.55f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,{0.70f,0.56f,0.39f,1});
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,0});
    ImGui::PushStyleColor(ImGuiCol_Text,{1,1,1,1});
    ImGui::PushStyleColor(ImGuiCol_Button,{0.66f,0.29f,0.25f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0.76f,0.36f,0.32f,1});
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,{0.57f,0.22f,0.19f,1});
    ImGui::SetCursorScreenPos({movedPos.x+size.x-72.0f,movedPos.y+11.0f});
    if(ImGui::Button("-",{26,26}))editorMinimized=!editorMinimized;
    ImGui::SetCursorScreenPos({movedPos.x+size.x-38.0f,movedPos.y+11.0f});
    if(ImGui::Button("X",{26,26}))Send(Action::CloseSiteEditor);
    ImGui::PopStyleColor(4);
    if(editorMinimized) {
        ImGui::PopStyleVar();ImGui::PopStyleColor(4);
        editorPosition=ImGui::GetWindowPos();
        RememberRect(&editorRect);editorRectValid=true;ImGui::End();
        return;
    }
    ImGui::SetCursorScreenPos({movedPos.x+12.0f,movedPos.y+56.0f});
    if(s.creatingSite) {
        ImGui::TextUnformatted("New Site draft");
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x*(2.0f/3.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{8.0f,6.0f});
        if(ImGui::InputText("Site name",draftNameBuffer,sizeof(draftNameBuffer)))
            SendProjectText(Action::SetDraftSiteName,0,draftNameBuffer);
        ImGui::PopStyleVar();
        nameTextEditing|=ImGui::IsItemActive();
    } else if(renameEditing) {
        ImGui::SetNextItemWidth(155.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{8.0f,6.0f});
        const bool enterPressed=ImGui::InputText("##EditingSiteName",renameBuffer,sizeof(renameBuffer),ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopStyleVar();
        nameTextEditing|=ImGui::IsItemActive();
        ImGui::SameLine();
        const bool saveRename=ImGui::Button("Save",{48,26})||enterPressed;
        ImGui::SameLine();
        const bool cancelRename=ImGui::Button("Cancel##CancelSiteRename",{54,26});
        ImGui::SameLine();ImGui::Text("|  %s",ProjectStatusName(s.projectStatus));
        if(saveRename) {
            SendProjectText(Action::RenameSite,s.editingSiteId,renameBuffer);
            renameEditing=false;
        }
        if(cancelRename)renameEditing=false;
    } else {
        ImGui::SetWindowFontScale(1.35f);
        ImGui::TextUnformatted(s.selectedSiteName);
        ImGui::SameLine();
        bool renamePressed=false;
        if(siteRenameIcon) {
            const ImTextureRef icon(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(siteRenameIcon)));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{0,0});
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button,{0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,{0,0,0,0});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,{0,0,0,0});
            const auto renameIconPos=ImGui::GetCursorScreenPos();
            const bool hovered=ImGui::IsMouseHoveringRect(renameIconPos,{renameIconPos.x+24.0f,renameIconPos.y+24.0f},true);
            renamePressed=ImGui::ImageButton("##RenameSelectedSite",icon,{24,24},{0,0},{1,1},hovered?ImVec4{1,1,1,1}:ImVec4{0.82f,0.82f,0.82f,1});
            ImGui::PopStyleColor(3);ImGui::PopStyleVar(2);
            if(hovered) NativeTooltip("Rename Site");
        } else renamePressed=ImGui::Button("R",{24,24});
        ImGui::SameLine();ImGui::Text("|  %s",ProjectStatusName(s.projectStatus));
        if(renamePressed) {
            strncpy_s(renameBuffer,s.selectedSiteName,_TRUNCATE);
            renameEditing=true;
        }
        ImGui::SetWindowFontScale(1.0f);
    }
    ImGui::Separator();
    ImGui::Text("Footprint: %.1f x %.1f m  |  %.0f m2",plan.Width(),plan.Depth(),static_cast<double>(plan.Width())*plan.Depth());
    ImGui::Text("Sampled ground mean: %.2f m",groundMean);
    const auto volume=Estimate(plan);
    ImGui::TextColored({0.55f,0.25f,0.10f,1},"Cut estimate: %.0f m3",volume.cut);
    ImGui::TextColored({0.12f,0.43f,0.29f,1},"Fill estimate: %.0f m3",volume.fill);
    ImGui::Separator();
    static float editHeight=0;
    static uint64_t seenEditorRevision=~uint64_t{0},seenEditorSite=~uint64_t{0};
    if(seenEditorRevision!=s.revision||seenEditorSite!=s.editingSiteId) {
        editHeight=plan.target;seenEditorRevision=s.revision;seenEditorSite=s.editingSiteId;
    }
    const bool editable=(s.creatingSite&&plan.complete)||(!s.creatingSite&&(s.projectStatus==ProjectStatus::Queued||s.projectStatus==ProjectStatus::Running||s.projectStatus==ProjectStatus::Waiting||s.projectStatus==ProjectStatus::Paused));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,{4.0f,4.0f});
    ImGui::BeginDisabled(!editable);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,{6.0f,6.0f});
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Target height (m)");
    ImGui::SameLine();
    const auto& input=ImGui::GetIO();
    const float heightStep=input.KeyCtrl?100.0f:(input.KeyShift?10.0f:1.0f);
    auto applyHeight=[&](float height) {
        if(s.creatingSite)Send(Action::Target,height);
        else SendProjectValue(Action::SetSiteTarget,s.editingSiteId,height);
    };
    if(ImGui::Button("-##DecreaseTargetHeight",{24.0f,0.0f})) {editHeight-=heightStep;applyHeight(editHeight);}
    ImGui::SameLine();
    ImGui::SetNextItemWidth(80.0f);
    const bool changed=ImGui::InputFloat("##TargetHeight",&editHeight,0,0,"%.2f",ImGuiInputTextFlags_EnterReturnsTrue);
    targetHeightEditing=ImGui::IsItemActive();nameTextEditing|=targetHeightEditing;
    static uint64_t pauseRequestedForHeightEdit=0;
    const bool siteIsWorking=s.projectStatus==ProjectStatus::Running||s.projectStatus==ProjectStatus::Waiting;
    if(!s.creatingSite&&targetHeightEditing&&siteIsWorking&&pauseRequestedForHeightEdit!=s.editingSiteId) {
        SendProjectAction(Action::PauseProject,s.editingSiteId);
        pauseRequestedForHeightEdit=s.editingSiteId;
    } else if(s.creatingSite||!siteIsWorking)pauseRequestedForHeightEdit=0;
    ImGui::SameLine();
    if(ImGui::Button("+##IncreaseTargetHeight",{24.0f,0.0f})) {editHeight+=heightStep;applyHeight(editHeight);}
    ImGui::SameLine();
    GameIconButton("##TargetHeightHelp",targetHeightInfoIcon,"i","Click −/+ to adjust by 1 m.\nHold Shift for 10 m, or Ctrl for 100 m.",{22.0f,22.0f});
    ImGui::SameLine();
    if(ImGui::Button("Apply height")||changed) {
        applyHeight(editHeight);
    }
    ImGui::PopStyleVar();
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
    ImGui::BeginDisabled(!editable);
    const float sampleButtonWidth=ImGui::GetContentRegionAvail().x;
    const bool sampleTargetPressed=ImGui::Button("Sample target from map",{sampleButtonWidth,28});
    const ImVec2 sampleButtonMin=ImGui::GetItemRectMin(),sampleButtonMax=ImGui::GetItemRectMax();
    const ImVec2 sampleInfoMin{sampleButtonMax.x-25.0f,sampleButtonMin.y+3.0f};
    const ImVec2 sampleInfoMax{sampleButtonMax.x-3.0f,sampleButtonMin.y+25.0f};
    const bool sampleInfoHovered=targetHeightInfoIcon&&ImGui::IsMouseHoveringRect(sampleInfoMin,sampleInfoMax,true);
    if(targetHeightInfoIcon) {
        const ImTextureRef infoTexture(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(targetHeightInfoIcon)));
        ImGui::GetWindowDrawList()->AddImage(infoTexture,sampleInfoMin,sampleInfoMax,{0,0},{1,1},
            sampleInfoHovered?IM_COL32_WHITE:IM_COL32(209,209,209,255));
    }
    if(sampleTargetPressed)Send(Action::SampleSiteTarget);
    if(sampleInfoHovered)NativeTooltip("Click this button, then click a point on the map to set the target height.");
    ImGui::EndDisabled();
    if(s.creatingSite) {
        const bool draftInvalid=s.siteDraftError[0]!=0;
        if(draftInvalid)ImGui::TextColored({0.67f,0.15f,0.12f,1.0f},"%s",s.siteDraftError);
        ImGui::BeginDisabled(draftInvalid);
        if(ImGui::Button("Save Site",{ImGui::GetContentRegionAvail().x,32}))SendProjectText(Action::ConfirmProject,0,draftNameBuffer);
        ImGui::EndDisabled();
        if(ImGui::Button("Discard draft",{ImGui::GetContentRegionAvail().x,30}))Send(Action::CancelSiteDraft);
    } else {
        const bool finished=s.projectStatus==ProjectStatus::Completed||s.projectStatus==ProjectStatus::Cancelled;
        if(finished)ImGui::TextDisabled("Finished Sites cannot be edited.");
        else if(s.projectStatus==ProjectStatus::Running||s.projectStatus==ProjectStatus::Waiting)ImGui::TextDisabled("Editing the target will pause this Site; review terrain before resuming.");
        else if(s.projectStatus==ProjectStatus::Paused&&s.projectNeedsReview)ImGui::TextWrapped("This Site needs terrain review before work can resume.");
        if(!finished) {
            Action workAction=Action::StartProject;
            const char* workLabel="Start";
            bool workEnabled=s.assignedFacilityCount!=0;
            if(s.projectStatus==ProjectStatus::Running||s.projectStatus==ProjectStatus::Waiting) {
                workAction=Action::PauseProject;workLabel="Pause";workEnabled=true;
            } else if(s.projectStatus==ProjectStatus::Paused) {
                workAction=s.projectNeedsReview?Action::ReviewResumeProject:Action::ResumeProject;
                workLabel=s.projectNeedsReview?"Review and resume":"Resume";workEnabled=s.assignedFacilityCount!=0;
            }
            const float workWidth=(ImGui::GetContentRegionAvail().x-ImGui::GetStyle().ItemSpacing.x)*0.68f;
            ImGui::BeginDisabled(!workEnabled);
            if(ImGui::Button(workLabel,{workWidth,32.0f}))SendProjectAction(workAction,s.editingSiteId);
            ImGui::EndDisabled();
            ImGui::SameLine();
            const float cancelWidth=ImGui::GetContentRegionAvail().x;
            ImGui::BeginDisabled(!(s.projectStatus==ProjectStatus::Queued||s.projectStatus==ProjectStatus::Running
                ||s.projectStatus==ProjectStatus::Waiting||s.projectStatus==ProjectStatus::Paused));
            if(ImGui::Button("Cancel##CancelSiteWork",{cancelWidth,32.0f}))SendProjectAction(Action::CancelProject,s.editingSiteId);
            ImGui::EndDisabled();
        }
        if(ImGui::Button("Close editor",{ImGui::GetContentRegionAvail().x,32}))Send(Action::CloseSiteEditor);
    }
    ImGui::PopStyleVar();ImGui::PopStyleColor(4);
    editorPosition=ImGui::GetWindowPos();
    RememberRect(&editorRect);editorRectValid=true;ImGui::End();
}
void DrawSiteEditorPaper(ImVec2 pos,ImVec2 size,bool headerOnly)
{
    auto* draw=ImGui::GetWindowDrawList();
    if(!panelPaper||!panelPaperWidth||!panelPaperHeight) {
        if(!headerOnly)draw->AddRectFilled(pos,{pos.x+size.x,pos.y+size.y},IM_COL32(232,220,197,255),12.0f);
        draw->AddRectFilled(pos,{pos.x+size.x,pos.y+size.y},IM_COL32(150,60,55,255),12.0f,
            headerOnly?ImDrawFlags_RoundCornersAll:ImDrawFlags_RoundCornersTop);
        return;
    }
    const ImTextureRef texture(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(panelPaper)));
    const ImVec2 headerUv0{52.0f/panelPaperWidth,20.0f/panelPaperHeight};
    const ImVec2 headerUv1{605.0f/panelPaperWidth,63.0f/panelPaperHeight};
    if(headerOnly)draw->AddImageRounded(texture,pos,{pos.x+size.x,pos.y+size.y},headerUv0,headerUv1,IM_COL32_WHITE,12.0f,ImDrawFlags_RoundCornersAll);
    else {
        const ImVec2 bodyUv0{52.0f/panelPaperWidth,63.0f/panelPaperHeight};
        const ImVec2 bodyUv1{605.0f/panelPaperWidth,698.0f/panelPaperHeight};
        draw->AddImageRounded(texture,pos,{pos.x+size.x,pos.y+size.y},bodyUv0,bodyUv1,IM_COL32_WHITE,12.0f,ImDrawFlags_RoundCornersBottom);
        draw->AddImageRounded(texture,pos,{pos.x+size.x,pos.y+52.0f},headerUv0,headerUv1,IM_COL32_WHITE,12.0f,ImDrawFlags_RoundCornersTop);
    }
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
void LoadGameUiIcon(const wchar_t* filename,ID3D11ShaderResourceView** output)
{
    if(!device||!output||*output)return;
    wchar_t executable[MAX_PATH]{};
    const auto length=GetModuleFileNameW(nullptr,executable,MAX_PATH);
    if(!length||length>=MAX_PATH)return;
    std::wstring path(executable,length);
    const auto slash=path.find_last_of(L"\\/");
    if(slash==std::wstring::npos)return;
    path.resize(slash+1);path+=L"media_soviet\\editor\\";path+=filename;
    const auto init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(init)&&init!=RPC_E_CHANGED_MODE)return;
    const bool uninitialize=SUCCEEDED(init);
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr))hr=factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder);
    if(SUCCEEDED(hr))hr=decoder->GetFrame(0,&frame);
    if(SUCCEEDED(hr))hr=factory->CreateFormatConverter(&converter);
    if(SUCCEEDED(hr))hr=converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
    UINT width=0,height=0;
    if(SUCCEEDED(hr))hr=converter->GetSize(&width,&height);
    std::vector<BYTE> pixels;
    if(SUCCEEDED(hr)&&width&&height&&static_cast<uint64_t>(width)*height<=1'000'000) {
        pixels.resize(static_cast<size_t>(width)*height*4);
        hr=converter->CopyPixels(nullptr,width*4,static_cast<UINT>(pixels.size()),pixels.data());
    } else if(SUCCEEDED(hr))hr=E_INVALIDARG;
    if(SUCCEEDED(hr)) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=width;desc.Height=height;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=pixels.data();data.SysMemPitch=width*4;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        hr=device->CreateTexture2D(&desc,&data,&texture);
        if(SUCCEEDED(hr))hr=device->CreateShaderResourceView(texture.Get(),nullptr,output);
    }
    if(uninitialize)CoUninitialize();
    if(FAILED(hr))host->log("earthworks  native Site row icon unavailable: %ls",filename);
}
void LoadSiteRowIcons()
{
    LoadGameUiIcon(L"button_cancel.png",&siteCancelIcon);
    LoadGameUiIcon(L"button_rename.png",&siteRenameIcon);
    LoadGameUiIcon(L"button_plus.png",&projectNewIcon);
    LoadGameUiIcon(L"trash.png",&projectDeleteIcon);
    LoadGameUiIcon(L"infoicon.png",&targetHeightInfoIcon);
}
void LoadGameCursorArt()
{
    if(gameCursorArt || !device) return;
    wchar_t executable[MAX_PATH]{};
    const auto length=GetModuleFileNameW(nullptr,executable,MAX_PATH);
    if(!length || length>=MAX_PATH) return;
    std::wstring path(executable,length);
    const auto slash=path.find_last_of(L"\\/");
    if(slash==std::wstring::npos) return;
    path.resize(slash+1);path+=L"media_soviet\\editor\\cursor.png";
    const auto init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(FAILED(init)&&init!=RPC_E_CHANGED_MODE) return;
    const bool uninitialize=SUCCEEDED(init);
    Microsoft::WRL::ComPtr<IWICImagingFactory> factory;
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> decoder;
    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    HRESULT hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
    if(SUCCEEDED(hr))hr=factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder);
    if(SUCCEEDED(hr))hr=decoder->GetFrame(0,&frame);
    if(SUCCEEDED(hr))hr=factory->CreateFormatConverter(&converter);
    if(SUCCEEDED(hr))hr=converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom);
    UINT width=0,height=0;
    if(SUCCEEDED(hr))hr=converter->GetSize(&width,&height);
    std::vector<BYTE> pixels;
    if(SUCCEEDED(hr)&&width&&height&&static_cast<uint64_t>(width)*height<=1'000'000) {
        pixels.resize(static_cast<size_t>(width)*height*4);
        hr=converter->CopyPixels(nullptr,width*4,static_cast<UINT>(pixels.size()),pixels.data());
    } else if(SUCCEEDED(hr))hr=E_INVALIDARG;
    if(SUCCEEDED(hr)) {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width=width;desc.Height=height;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{};data.pSysMem=pixels.data();data.SysMemPitch=width*4;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        hr=device->CreateTexture2D(&desc,&data,&texture);
        if(SUCCEEDED(hr))hr=device->CreateShaderResourceView(texture.Get(),nullptr,&gameCursorArt);
        if(SUCCEEDED(hr)){gameCursorArtWidth=width;gameCursorArtHeight=height;}
    }
    if(uninitialize)CoUninitialize();
    if(FAILED(hr))host->log("earthworks  game cursor art unavailable; native cursor remains unchanged");
}
void DrawGameCursorArt()
{
    const auto pos=ImGui::GetIO().MousePos;
    const bool finite=std::isfinite(pos.x)&&std::isfinite(pos.y);
    const auto point=finite?MAKELPARAM(static_cast<int>(pos.x),static_cast<int>(pos.y)):LPARAM{};
    const bool overPanel=finite
        &&((panelOpen&&panelRectValid&&Hit(panelRect,point))||(panelOpen&&popupRectValid&&Hit(popupRect,point))||(editorRectValid&&Hit(editorRect,point)));
    if(!overPanel||GetForegroundWindow()!=gameWindow) {
        if(systemCursorHiddenForPanel&&cursorToRestore)SetCursor(cursorToRestore);
        systemCursorHiddenForPanel=false;cursorToRestore=nullptr;return;
    }
    if(!gameCursorArt)return;
    if(!systemCursorHiddenForPanel) {
        if(HCURSOR cursor=GetCursor())cursorToRestore=cursor;
        SetCursor(nullptr);systemCursorHiddenForPanel=true;
    }
    const float width=32.0f,height=width*static_cast<float>(gameCursorArtHeight)/std::max(1u,gameCursorArtWidth);
    const ImTextureRef texture(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(gameCursorArt)));
    ImGui::GetForegroundDrawList()->AddImage(texture,{pos.x-2.0f,pos.y-2.0f},{pos.x+width-2.0f,pos.y+height-2.0f},
        ImVec2(0,0),ImVec2(1,1),IM_COL32_WHITE);
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
    io.ConfigFlags|=ImGuiConfigFlags_NoMouseCursorChange;
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
    LoadSiteRowIcons();
    LoadGameCursorArt();
    if (!ImGui_ImplWin32_Init(gameWindow) || !ImGui_ImplDX11_Init(device,context)) return false;
    SetLastError(0);
    originalWndProc=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(gameWindow,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(WindowProc)));
    if (!originalWndProc) return false;
    gameChain=swap;
    gameUnderlayInstalled=InstallGameUnderlay(host);
    host->log("earthworks  native UI terrain underlay %s",gameUnderlayInstalled?"installed":"unavailable; using foreground fallback");
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
    previewLinesUnderlay=gameUnderlayInstalled;
    DrawPreview(snapshot);
    previewLinesUnderlay=false;
    ImGui::GetBackgroundDrawList()->PopClipRect();
    DrawInterface(snapshot);
    DrawGameCursorArt();
    if(snapshot.mode==PickMode::None) pickCancelledByEscape=false;
    const bool sitePick=!snapshot.siteEditorOpen&&CursorOnVisibleSite(snapshot);
    areaPickCapture=panelOpen&&!panelMinimized&&snapshot.mode!=PickMode::None&&!pickCancelledByEscape;
    mapPickCapture=areaPickCapture||(!pickCancelledByEscape&&sitePick);
    const bool focused=GetForegroundWindow()==gameWindow;
    mouseCapture=focused && io.WantCaptureMouse && snapshot.ready;
    keyboardCapture=focused && (targetHeightEditing||nameTextEditing) && panelOpen;
    if (!(GetAsyncKeyState(VK_LBUTTON)&0x8000)) leftRelease=false;
    // Leave cursor shape and visibility entirely to the game window procedure.
    ImGui::GetIO().MouseDrawCursor=false;
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
