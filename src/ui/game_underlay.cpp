// SPDX-License-Identifier: GPL-3.0-only
#include "game_underlay.h"
#include "imgui.h"
#include "../core/terrain_grid.h"
#include <intrin.h>
#include <windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace ew {
namespace {
// WRSR 1.1.1.13 interface routine. The prologue was checked in the updated
// executable; keep this checked so a later update cannot splice into new code.
constexpr size_t UiRoutineRva = 0x317D30;
constexpr unsigned char UiRoutineBytes[] = {
    0x40,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
    0x48,0x8D,0xAC,0x24,0xD0,0xD4,0xFF,0xFF
};
constexpr size_t UiRoutineStolen = sizeof(UiRoutineBytes);
constexpr size_t PanelRva = 0x9BE060;
constexpr size_t PanelBeginIat = 0x86C918;
constexpr size_t PanelDrawIat = 0x86C860;
constexpr size_t PanelEndIat = 0x86C8F8;
constexpr size_t RotationIat = 0x86D000;
constexpr size_t CurrentShadersRva = 0x9EAD08;
constexpr size_t WhiteTextureRva = 0x9E0B38;
constexpr size_t ScreenWidthRva = 0x99528C;
constexpr size_t ScreenHeightRva = 0x995274;
constexpr size_t PanelSize = 0x288;
constexpr size_t PanelPos = 0x290;
constexpr size_t PanelRotation = 0x2A8;
constexpr size_t PanelColor = 0x2AC;
constexpr size_t PanelFieldsBytes = 0x34;
constexpr size_t PanelZoomCentre = 0x6730;
constexpr size_t PanelZoom = 0x673C;
constexpr size_t TextureSetSlot = 14;
constexpr size_t MaxSegments = 4096;
constexpr size_t GroupCount = 14;
constexpr float ShadeRowPixels = 5.0f;
constexpr size_t MaxNativeUiRects = 8192;

// WRSR 1.1.1.13 calls to C3D_PANEL2D::Draw for cursor graphics. Match the
// draw call because controls can use the same textures.
// Each entry gives the call RVA and the eight bytes that follow it.
struct CursorDrawCall { size_t rva; uint64_t followingBytes; };
constexpr CursorDrawCall CursorDrawCalls[] = {
    // Pointer draws in other native screens.
    {0x0B050D, 0x000001D024848B41ULL},
    {0x26244F, 0xFF0075BC040D8D48ULL},
    // Resize, arrow, selected tool, cursor extension and free move.
    {0x32712D, 0x5E38880D100F44F3ULL},
    {0x32723C, 0x000000ADE8BE8341ULL},
    {0x32751D, 0x480000B7709E8B49ULL},
    {0x327669, 0x000000ADE8BE8341ULL},
    {0x3277BE, 0x0F44F300000723E9ULL},
    // Radiation meter back, trim and front.
    {0x3278D2, 0x4800000F6C968D49ULL},
    {0x327990, 0xBF0D8D48CB280F41ULL},
    {0x327B53, 0x480000B7789E8B49ULL},
    // Arrow and free-move variants.
    {0x327C5B, 0x000000ADE8BE8341ULL},
    {0x327D81, 0x5E2C340D100F44F3ULL},
    {0x327EC0, 0x5E2AF50D100F44F3ULL},
    // Tool indicators drawn beside the pointer. The tool controls and text
    // boxes have separate draw calls and remain part of native UI capture.
    {0x32818B, 0x480000F640968B41ULL},
    {0x3283FE, 0x00031C908B068B48ULL},
    {0x3285E2, 0x8B0000F7A0868B49ULL},
    {0x3287DF, 0x8500011A14968B41ULL},
    {0x328AB6, 0x8B0000F800868B49ULL},
    {0x328D1C, 0x8B0000F858868B49ULL},
    {0x328FEC, 0x8B0000FAF0868B49ULL},
    {0x3292B9, 0x8B0000F8B0868B49ULL},
    {0x32951B, 0x8B0000FA98868B49ULL},
    {0x329816, 0x8B0000FA18868B49ULL},
    {0x329AB5, 0x8B0000FA20868B49ULL},
    {0x329EDB, 0x8B0000F908868B49ULL},
    {0x32A1A8, 0x8B0000F968868B49ULL},
    {0x32A3C3, 0x8B0000FA40868B49ULL},
    {0x32A62E, 0x8B0000FA30868B49ULL},
    {0x32A873, 0x8B0000F9C0868B49ULL},
    {0x32AA63, 0x8B0000F9C0868B49ULL},
};
constexpr size_t IndirectCallSize = 6;
std::array<uintptr_t, std::size(CursorDrawCalls)> cursorDrawReturns{};
size_t cursorDrawReturnCount = 0;

using BeginFn = void (*)(void*, const char*, bool);
using EndFn = void (*)(void*);
using DrawFn = void (*)(void*, float, float, float, float, float, bool);
using RotationFn = void (*)(float*, const float*);
using SetTextureFn = void (*)(void*, unsigned, void*);
using UiFn = __int64 (*)(void*);
UiFn originalUi = nullptr;
DrawFn originalPanelDraw = nullptr;
const TsmHost* boundHost = nullptr;
std::mutex interactivePreviewMutex;
Plan interactivePreviewPlan{};
uint64_t interactivePreviewSiteId=0;
bool interactivePreviewActive=false;
std::atomic<bool> winterWireframeMode{false};
struct UiRect { float left=0,top=0,right=0,bottom=0; };
std::array<UiRect,MaxNativeUiRects> nativeUiRects{};
size_t nativeUiRectCount=0;
unsigned cursorDrawsIgnored=0;
std::shared_mutex nativeUiMutex;
std::atomic<bool> nativeUiCaptureActive{false},nativeUiCaptureInstalled{false};
thread_local bool drawingEarthworksUnderlay=false;

struct SegmentGroup { float values[MaxSegments * 4]{}; int count = 0; ImU32 color = 0; float width = 1.5f; };
SegmentGroup groups[GroupCount];

void ResetGroups()
{
    const bool winter=winterWireframeMode.load(std::memory_order_relaxed);
    groups[0].color = winter?IM_COL32(255,220,40,245):IM_COL32(55,220,200,225); groups[0].width = 1.5f;
    groups[1].color = winter?IM_COL32(255,105,0,245):IM_COL32(255,175,55,225); groups[1].width = 1.5f;
    groups[2].color = winter?IM_COL32(255,174,0,230):IM_COL32(175,180,190,210); groups[2].width = 1.5f;
    groups[3].color = winter?IM_COL32(194,75,0,245):IM_COL32(205,210,220,235); groups[3].width = 2.5f;
    groups[4].color = winter?IM_COL32(255,245,120,255):IM_COL32(245,250,255,255); groups[4].width = 2.5f;
    groups[5].color = winter?IM_COL32(255,205,0,75):IM_COL32(62,221,166,55); groups[5].width = ShadeRowPixels;
    groups[6].color = winter?IM_COL32(255,95,0,75):IM_COL32(255,159,67,55); groups[6].width = ShadeRowPixels;
    groups[7].color = IM_COL32(20,25,30,240); groups[7].width = 5.0f;
    groups[8].color = winter?IM_COL32(255,211,0,255):IM_COL32(62,221,166,255); groups[8].width = 2.5f;
    groups[9].color = winter?IM_COL32(255,105,0,255):IM_COL32(255,159,67,255); groups[9].width = 2.5f;
    groups[10].color = IM_COL32(35,45,55,255); groups[10].width = 5.0f;
    groups[11].color = winter?IM_COL32(255,105,0,255):IM_COL32(255,245,130,255); groups[11].width = 2.5f;
    groups[12].color = winter?IM_COL32(255,174,0,255):IM_COL32(120,220,255,255); groups[12].width = 2.5f;
    groups[13].color = winter?IM_COL32(255,245,120,255):IM_COL32(245,250,255,245); groups[13].width = 2.5f;
    for (auto& group : groups) group.count = 0;
}

bool CopyInteractiveSitePreview(Plan* plan,uint64_t* siteId)
{
    if(!plan||!siteId)return false;
    std::lock_guard<std::mutex> lock(interactivePreviewMutex);
    if(!interactivePreviewActive)return false;
    *plan=interactivePreviewPlan;*siteId=interactivePreviewSiteId;
    return true;
}

bool Project(const Snapshot& s, Point p, float width, float height, float* x, float* y)
{
    float eye[3]{}, clip[4]{};
    for (int i=0; i<3; ++i) eye[i]=p.x*s.view[i]+p.y*s.view[4+i]+p.z*s.view[8+i]+s.view[12+i];
    for (int i=0; i<4; ++i) clip[i]=eye[0]*s.projection[i]+eye[1]*s.projection[4+i]+eye[2]*s.projection[8+i]+s.projection[12+i];
    if (!std::isfinite(clip[3]) || clip[3]<0.01f) return false;
    *x=(clip[0]/clip[3]+1.0f)*0.5f*width;
    *y=(1.0f-clip[1]/clip[3])*0.5f*height;
    return std::isfinite(*x)&&std::isfinite(*y)&&std::abs(*x)<1e6f&&std::abs(*y)<1e6f;
}

void AddLine(const Snapshot& s, Point a, Point b, int group, float width, float height)
{
    if (groups[group].count>=static_cast<int>(MaxSegments)) return;
    float x1,y1,x2,y2;
    if (!Project(s,a,width,height,&x1,&y1)||!Project(s,b,width,height,&x2,&y2)) return;
    auto* out=groups[group].values+groups[group].count++*4;
    out[0]=x1;out[1]=y1;out[2]=x2;out[3]=y2;
}

void AddScreenSpan(float x1,float y,float x2,int group)
{
    if(groups[group].count>=static_cast<int>(MaxSegments)||x2-x1<0.5f)return;
    auto* out=groups[group].values+groups[group].count++*4;
    out[0]=x1;out[1]=y;out[2]=x2;out[3]=y;
}

void AddScreenLine(float x1,float y1,float x2,float y2,int group)
{
    if(groups[group].count>=static_cast<int>(MaxSegments))return;
    auto* out=groups[group].values+groups[group].count++*4;
    out[0]=x1;out[1]=y1;out[2]=x2;out[3]=y2;
}

void AddScreenCross(float x,float y,float arm,int outlineGroup,int colorGroup)
{
    AddScreenLine(x-arm,y,x+arm,y,outlineGroup);
    AddScreenLine(x,y-arm,x,y+arm,outlineGroup);
    AddScreenLine(x-arm,y,x+arm,y,colorGroup);
    AddScreenLine(x,y-arm,x,y+arm,colorGroup);
}

void AddShadeTriangle(const Snapshot& s,const Point* triangle,const float* depth,float sign,int group,float width,float height)
{
    if(groups[group].count>=static_cast<int>(MaxSegments))return;
    Point clipped[6]{};int count=0;
    for(int i=0;i<3;++i) {
        const int j=(i+1)%3;
        const float a=depth[i]*sign,b=depth[j]*sign;
        if(a>=0)clipped[count++]=triangle[i];
        if((a>0&&b<0)||(a<0&&b>0)) {
            const float t=a/(a-b);
            clipped[count++]={triangle[i].x+t*(triangle[j].x-triangle[i].x),triangle[i].y,triangle[i].z+t*(triangle[j].z-triangle[i].z)};
        }
    }
    if(count<3)return;
    float px[6]{},py[6]{},minY=0,maxY=0;
    for(int i=0;i<count;++i) {
        if(!Project(s,clipped[i],width,height,&px[i],&py[i]))return;
        if(i==0)minY=maxY=py[i];else {minY=std::min(minY,py[i]);maxY=std::max(maxY,py[i]);}
    }
    minY=std::max(85.0f,minY);maxY=std::min(height-90.0f,maxY);
    if(maxY-minY<0.5f)return;
    const float firstY=std::floor(minY/ShadeRowPixels)*ShadeRowPixels+ShadeRowPixels*0.5f;
    for(float y=firstY;y<maxY;y+=ShadeRowPixels) {
        if(groups[group].count>=static_cast<int>(MaxSegments))return;
        float intersections[6]{};int n=0;
        for(int i=0;i<count;++i) {
            const int j=(i+1)%count;
            const float low=std::min(py[i],py[j]),high=std::max(py[i],py[j]);
            if(y<low||y>=high||std::abs(py[j]-py[i])<1e-5f)continue;
            const float t=(y-py[i])/(py[j]-py[i]);
            intersections[n++]=px[i]+t*(px[j]-px[i]);
        }
        std::sort(intersections,intersections+n);
        for(int i=0;i+1<n;i+=2)AddScreenSpan(intersections[i],y,intersections[i+1],group);
    }
}

template<class Shape>
void DrawOutline(const Snapshot& s, const Shape& plan, int group, float width, float height)
{
    for(unsigned i=0;i<plan.pointCount;++i) AddLine(s,plan.points[i],plan.points[(i+1)%plan.pointCount],group,width,height);
}

Point DrawingCursor(const Snapshot& s)
{
    if(!s.creatingSite||!s.lockToGrid||!s.terrainGridValid)return s.cursor;
    TerrainGrid grid{};
    grid.originX=s.terrainGridOriginX;grid.originZ=s.terrainGridOriginZ;
    grid.stepX=s.terrainGridStepX;grid.stepZ=s.terrainGridStepZ;
    return SnapToTerrainGrid(s.cursor,grid);
}

void Collect(const Snapshot& s, float width, float height,const Plan* livePlan,uint64_t liveSiteId)
{
    ResetGroups();
    if (!s.ready || !s.cameraValid) return;
    for(size_t i=0;i<s.projectCount;++i) {
        const auto& project=s.projects[i];
        if(!project.visible)continue;
        if(project.status==ProjectStatus::Queued||project.status==ProjectStatus::Running
            ||project.status==ProjectStatus::Waiting||project.status==ProjectStatus::Paused) {
            if(project.id==s.editingSiteId && s.queuedPlan.complete) {
                const auto& p=livePlan&&liveSiteId==project.id?*livePlan:s.queuedPlan;
                for(int z=0;z<GridSide;++z) for(int x=0;x<GridSide;++x) {
                    const auto a=p.Vertex(x,z,true,s.terrainCellSize);
                    if(x<GridCells) {const auto b=p.Vertex(x+1,z,true,s.terrainCellSize);if(p.Contains((a.x+b.x)*0.5f,(a.z+b.z)*0.5f))AddLine(s,a,b,2,width,height);}
                    if(z<GridCells) {const auto b=p.Vertex(x,z+1,true,s.terrainCellSize);if(p.Contains((a.x+b.x)*0.5f,(a.z+b.z)*0.5f))AddLine(s,a,b,2,width,height);}
                }
                DrawOutline(s,p,3,width,height);
            } else DrawOutline(s,project.outline,2,width,height);
        } else if(project.status==ProjectStatus::Completed) DrawOutline(s,project.outline,0,width,height);
    }
    bool editorVisible=s.creatingSite;
    if(s.siteEditorOpen&&!editorVisible)for(size_t i=0;i<s.projectCount;++i)
        if(s.projects[i].id==s.editingSiteId){editorVisible=s.projects[i].visible;break;}
    if(s.siteEditorOpen&&editorVisible) {
        const auto& editor=livePlan?*livePlan:s.editorPlan;
        const uint32_t selectedMask=s.selectedSitePointsMask;
        for(unsigned i=0;i<editor.pointCount;++i) {
            float x=0,y=0;
            if(!Project(s,editor.points[i],width,height,&x,&y))continue;
            const bool primary=i==s.selectedSitePoint;
            const bool selected=primary||(selectedMask&(1u<<i));
            const float radius=primary?8.0f:(selected?7.0f:6.0f);
            const int fillGroup=primary?11:(selected?12:13);
            AddScreenCross(x,y,radius,10,fillGroup);
            float ground=0;
            if(!editor.GroundAt(editor.points[i].x,editor.points[i].z,&ground)
                ||std::abs(editor.points[i].y-ground)<0.01f)continue;
            const Point terrain{editor.points[i].x,ground,editor.points[i].z};
            float gx=0,gy=0;
            if(!Project(s,terrain,width,height,&gx,&gy))continue;
            const int linkGroup=editor.points[i].y>ground?8:9;
            AddScreenLine(x,y,gx,gy,7);
            AddScreenLine(x,y,gx,gy,linkGroup);
            AddScreenCross(gx,gy,3.5f,7,linkGroup);
        }
    }
    const auto& p=livePlan&&liveSiteId==0?*livePlan:s.plan;
    if (!p.first) return;
    if (!p.complete) {
        for(unsigned i=0;i<p.pointCount;++i) {
            if(i+1<p.pointCount)AddLine(s,p.points[i],p.points[i+1],0,width,height);
        }
        if(panelOpen&&s.cursorValid&&s.mode==PickMode::DrawSite&&p.pointCount) {
            const Point cursor=DrawingCursor(s);
            if(s.shapeMode!=SiteShapeMode::FreeDraw&&p.pointCount==1) {
                Point corners[4]{};BuildTwoCornerShape(p.points[0],cursor,s.shapeMode,corners);
                for(unsigned i=0;i<4;++i)AddLine(s,corners[i],corners[(i+1)%4],0,width,height);
            } else AddLine(s,p.points[p.pointCount-1],cursor,0,width,height);
        }
        return;
    }
    for(int z=0;z<GridSide;++z) for(int x=0;x<GridSide;++x) {
        const auto a=p.Vertex(x,z,true,s.terrainCellSize);
        float target=0;const int group=p.BlendedTargetAt(a.x,a.z,s.terrainCellSize,&target)&&target>=p.Vertex(x,z,false).y?0:1;
        if(x<GridCells) {const auto b=p.Vertex(x+1,z,true,s.terrainCellSize);if(p.Contains((a.x+b.x)*0.5f,(a.z+b.z)*0.5f))AddLine(s,a,b,group,width,height);}
        if(z<GridCells) {const auto b=p.Vertex(x,z+1,true,s.terrainCellSize);if(p.Contains((a.x+b.x)*0.5f,(a.z+b.z)*0.5f))AddLine(s,a,b,group,width,height);}
    }
    for(int z=0;z<GridCells;++z) for(int x=0;x<GridCells;++x) {
        const Point ground[]={p.Vertex(x,z,false),p.Vertex(x+1,z,false),p.Vertex(x+1,z+1,false),p.Vertex(x,z+1,false)};
        if(!p.Contains((ground[0].x+ground[2].x)*0.5f,(ground[0].z+ground[2].z)*0.5f))continue;
        Point proposed[4];float depths[4];bool valid=true;
        for(unsigned i=0;i<4;++i) {valid=p.BlendedTargetAt(ground[i].x,ground[i].z,s.terrainCellSize,&proposed[i].y)&&valid;depths[i]=proposed[i].y-ground[i].y;proposed[i].x=ground[i].x;proposed[i].z=ground[i].z;}
        if(!valid)continue;
        for(int t=0;t<2;++t) {
            const Point triangle[]={proposed[0],proposed[t+1],proposed[t+2]};
            const float triangleDepth[]={depths[0],depths[t+1],depths[t+2]};
            AddShadeTriangle(s,triangle,triangleDepth,1,5,width,height);
            AddShadeTriangle(s,triangle,triangleDepth,-1,6,width,height);
        }
    }
    DrawOutline(s,p,4,width,height);
}

bool BindWhiteTexture(unsigned char* base)
{
    auto* texture=*reinterpret_cast<void**>(base+WhiteTextureRva);
    if(!texture) return false;
    auto** vtable=*reinterpret_cast<void***>(texture);
    reinterpret_cast<SetTextureFn>(vtable[TextureSetSlot])(texture,0,*reinterpret_cast<void**>(base+CurrentShadersRva));
    return true;
}

void DrawGroup(unsigned char* base, const SegmentGroup& group)
{
    if(group.count<=0||!BindWhiteTexture(base)) return;
    auto* panel=base+PanelRva;
    unsigned char saved[PanelFieldsBytes];
    std::memcpy(saved,panel+PanelSize,sizeof(saved));
    __try {
        float zoom=*reinterpret_cast<float*>(panel+PanelZoom); if(!(zoom>1.0f)) zoom=1.0f;
        const float zx=*reinterpret_cast<float*>(panel+PanelZoomCentre),zy=*reinterpret_cast<float*>(panel+PanelZoomCentre+4);
        const auto c=group.color;
        const float rgba[4]={((c>>IM_COL32_R_SHIFT)&255)/255.0f,((c>>IM_COL32_G_SHIFT)&255)/255.0f,((c>>IM_COL32_B_SHIFT)&255)/255.0f,((c>>IM_COL32_A_SHIFT)&255)/255.0f};
        std::memcpy(panel+PanelColor,rgba,sizeof(rgba));
        const auto draw=reinterpret_cast<DrawFn>(*reinterpret_cast<void**>(base+PanelDrawIat));
        const auto rotation=reinterpret_cast<RotationFn>(*reinterpret_cast<void**>(base+RotationIat));
        for(int i=0;i<group.count;++i) {
            float x1=group.values[i*4],y1=group.values[i*4+1],x2=group.values[i*4+2],y2=group.values[i*4+3];
            if(zoom>1.0f){x1=zx+(x1-zx)/zoom;y1=zy+(y1-zy)/zoom;x2=zx+(x2-zx)/zoom;y2=zy+(y2-zy)/zoom;}
            const float dx=x2-x1,dy=y2-y1,len=std::sqrt(dx*dx+dy*dy),w=group.width/zoom;
            if(len<0.5f) continue;
            auto* pos=reinterpret_cast<float*>(panel+PanelPos);pos[0]=(x1+x2)*0.5f;pos[1]=(y1+y2)*0.5f;pos[2]=0;
            auto* size=reinterpret_cast<float*>(panel+PanelSize);size[0]=w;size[1]=len+w;
            float v[3]={dx/len,0,-dy/len};rotation(v,v);*reinterpret_cast<float*>(panel+PanelRotation)=v[1];
            draw(panel,0,0,1,1,0,true);
        }
    } __finally { std::memcpy(panel+PanelSize,saved,sizeof(saved)); }
}

bool FindExportNameForAddress(HMODULE module,const void* address,std::string* name)
{
    if(!module||!address||!name)return false;
    const auto* base=reinterpret_cast<const unsigned char*>(module);
    const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return false;
    const auto directory=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if(!directory.VirtualAddress||directory.Size<sizeof(IMAGE_EXPORT_DIRECTORY))return false;
    const auto* exports=reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base+directory.VirtualAddress);
    const auto* functions=reinterpret_cast<const DWORD*>(base+exports->AddressOfFunctions);
    const auto* names=reinterpret_cast<const DWORD*>(base+exports->AddressOfNames);
    const auto* ordinals=reinterpret_cast<const WORD*>(base+exports->AddressOfNameOrdinals);
    for(DWORD i=0;i<exports->NumberOfNames;++i) {
        const WORD ordinal=ordinals[i];
        if(ordinal>=exports->NumberOfFunctions)continue;
        if(base+functions[ordinal]==address) {
            *name=reinterpret_cast<const char*>(base+names[i]);
            return !name->empty();
        }
    }
    return false;
}

bool ReadNativeUiRect(void* panel,UiRect* output)
{
    float x=0,y=0,width=0,height=0,angle=0,screenWidth=0,screenHeight=0;
    __try {
        auto* bytes=static_cast<unsigned char*>(panel);
        const auto* position=reinterpret_cast<const float*>(bytes+PanelPos);
        const auto* size=reinterpret_cast<const float*>(bytes+PanelSize);
        x=position[0];y=position[1];width=std::abs(size[0]);height=std::abs(size[1]);
        angle=*reinterpret_cast<const float*>(bytes+PanelRotation);
        if(boundHost&&boundHost->exeBase) {
            screenWidth=static_cast<float>(*reinterpret_cast<const int*>(boundHost->exeBase+ScreenWidthRva));
            screenHeight=static_cast<float>(*reinterpret_cast<const int*>(boundHost->exeBase+ScreenHeightRva));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(width)||!std::isfinite(height)
        ||!std::isfinite(angle)||width<2.0f||height<2.0f||screenWidth<=0||screenHeight<=0
        ||width>screenWidth*1.25f||height>screenHeight*1.25f)return false;
    const float cosine=std::abs(std::cos(angle)),sine=std::abs(std::sin(angle));
    const float halfWidth=(width*cosine+height*sine)*0.5f;
    const float halfHeight=(width*sine+height*cosine)*0.5f;
    *output={std::max(0.0f,x-halfWidth),std::max(0.0f,y-halfHeight),
        std::min(screenWidth,x+halfWidth),std::min(screenHeight,y+halfHeight)};
    return output->right>output->left&&output->bottom>output->top;
}

void RecordNativeUiRect(void* panel)
{
    UiRect rect{};
    if(!ReadNativeUiRect(panel,&rect))return;
    std::unique_lock lock(nativeUiMutex);
    if(nativeUiRectCount<MaxNativeUiRects)nativeUiRects[nativeUiRectCount++]=rect;
}

void VerifyCursorDrawCalls(const TsmHost& hookHost)
{
    cursorDrawReturnCount = 0;
    for (const auto& call : CursorDrawCalls) {
        const size_t returnRva = call.rva + IndirectCallSize;
        if (returnRva + sizeof(call.followingBytes) > hookHost.exeSize) continue;
        const auto* instruction = hookHost.exeBase + call.rva;
        const auto* returnAddress = hookHost.exeBase + returnRva;
        if (instruction[0] != 0xFF || instruction[1] != 0x15 ||
            std::memcmp(returnAddress, &call.followingBytes, sizeof(call.followingBytes)) != 0) continue;
        int32_t displacement = 0;
        std::memcpy(&displacement, instruction + 2, sizeof(displacement));
        if (displacement != static_cast<int32_t>(PanelDrawIat - returnRva)) continue;
        cursorDrawReturns[cursorDrawReturnCount++] = reinterpret_cast<uintptr_t>(returnAddress);
    }
}

void HookedPanelDraw(void* panel,float u0,float v0,float u1,float v1,float rotation,bool flag)
{
    if (nativeUiCaptureActive.load(std::memory_order_acquire) && !drawingEarthworksUnderlay) {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        if (std::binary_search(cursorDrawReturns.begin(), cursorDrawReturns.begin() + cursorDrawReturnCount, caller)) {
            std::unique_lock lock(nativeUiMutex);
            ++cursorDrawsIgnored;
        } else {
            RecordNativeUiRect(panel);
        }
    }
    if(originalPanelDraw)originalPanelDraw(panel,u0,v0,u1,v1,rotation,flag);
}

void BeginNativeUiCapture()
{
    std::unique_lock lock(nativeUiMutex);
    nativeUiRectCount=0;
    cursorDrawsIgnored=0;
    nativeUiCaptureActive.store(nativeUiCaptureInstalled.load(std::memory_order_acquire),std::memory_order_release);
}

void EndNativeUiCapture()
{
    nativeUiCaptureActive.store(false,std::memory_order_release);
}

void DrawUnderUi()
{
    if(!boundHost||!boundHost->exeBase) return;
    const auto& snapshot=ReadSnapshot();
    if(!snapshot.ready||!snapshot.cameraValid) return;
    Plan preview{};const Plan* previewPtr=nullptr;uint64_t previewId=0;
    if(CopyInteractiveSitePreview(&preview,&previewId))previewPtr=&preview;
    __try {
        auto* base=boundHost->exeBase;
        const float width=static_cast<float>(*reinterpret_cast<int*>(base+ScreenWidthRva));
        const float height=static_cast<float>(*reinterpret_cast<int*>(base+ScreenHeightRva));
        if(width<=0||height<=0) return;
        Collect(snapshot,width,height,previewPtr,previewId);
        auto* panel=base+PanelRva;
        reinterpret_cast<BeginFn>(*reinterpret_cast<void**>(base+PanelBeginIat))(panel,nullptr,false);
        __try {
            drawingEarthworksUnderlay=true;
            DrawGroup(base,groups[5]);DrawGroup(base,groups[6]);
            for(int i=0;i<5;++i)DrawGroup(base,groups[i]);
            for(int i=7;i<=9;++i)DrawGroup(base,groups[i]);
            DrawGroup(base,groups[10]);
            for(int i=11;i<GroupCount;++i)DrawGroup(base,groups[i]);
            drawingEarthworksUnderlay=false;
        } __finally {
            drawingEarthworksUnderlay=false;
            reinterpret_cast<EndFn>(*reinterpret_cast<void**>(base+PanelEndIat))(panel);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // The checked UI hook may run while the game is changing render state.
    }
}

__int64 HookedUiRoutine(void* world)
{
    DrawUnderUi();
    BeginNativeUiCapture();
    __try { return originalUi ? originalUi(world) : 0; }
    __finally { EndNativeUiCapture(); }
}
}

bool InstallGameUnderlay(const TsmHost* hookHost)
{
    if(!hookHost||!hookHost->exeBase||hookHost->exeSize<UiRoutineRva+UiRoutineStolen||!hookHost->installInlineHook) return false;
    boundHost=hookHost;
    VerifyCursorDrawCalls(*hookHost);
    auto* drawSlot=reinterpret_cast<void**>(hookHost->exeBase+PanelDrawIat);
    HMODULE graphics=GetModuleHandleW(L"C3DDLL64.dll");
    std::string drawSymbol;
    if(drawSlot&&*drawSlot&&FindExportNameForAddress(graphics,*drawSlot,&drawSymbol)
        &&hookHost->findIatSlot&&hookHost->patchIat
        &&hookHost->findIatSlot(hookHost->exeModule,"C3DDLL64.dll",drawSymbol.c_str())==drawSlot
        &&hookHost->patchIat(hookHost->exeModule,"C3DDLL64.dll",drawSymbol.c_str(),reinterpret_cast<void*>(HookedPanelDraw),
            reinterpret_cast<void**>(&originalPanelDraw),"Earthworks native UI click occlusion")) {
        nativeUiCaptureInstalled.store(originalPanelDraw!=nullptr,std::memory_order_release);
    }
    if(!hookHost->installInlineHook(hookHost->exeBase+UiRoutineRva,reinterpret_cast<void*>(HookedUiRoutine),
        reinterpret_cast<void**>(&originalUi),UiRoutineBytes,UiRoutineStolen,"Earthworks terrain wireframe under native UI")) {
        boundHost=nullptr;
        return false;
    }
    hookHost->log("earthworks  native UI click occlusion %s%s",nativeUiCaptureInstalled.load()?"installed":"unavailable",
        drawSymbol.empty()?"":" via C3D_PANEL draw capture");
    if (nativeUiCaptureInstalled.load()) {
        hookHost->log("earthworks  native cursor draw signatures matched %u/%u",
            static_cast<unsigned>(cursorDrawReturnCount), static_cast<unsigned>(std::size(CursorDrawCalls)));
    }
    return true;
}

bool NativeUiOccludes(float x,float y,NativeUiOcclusionInfo* info)
{
    if(info)*info={};
    if(!std::isfinite(x)||!std::isfinite(y))return false;
    if(!nativeUiCaptureInstalled.load(std::memory_order_acquire)) {
        if(!boundHost||!boundHost->exeBase)return false;
        const float width=static_cast<float>(*reinterpret_cast<const int*>(boundHost->exeBase+ScreenWidthRva));
        const float height=static_cast<float>(*reinterpret_cast<const int*>(boundHost->exeBase+ScreenHeightRva));
        return x<=65.0f||y<=85.0f||x>=width-300.0f||y>=height-150.0f;
    }
    std::shared_lock lock(nativeUiMutex);
    if (info) {
        info->rectCount = static_cast<unsigned>(nativeUiRectCount);
        info->cursorDrawsIgnored = cursorDrawsIgnored;
    }
    for(size_t i=0;i<nativeUiRectCount;++i) {
        const auto& rect=nativeUiRects[i];
        if(x>=rect.left&&x<=rect.right&&y>=rect.top&&y<=rect.bottom) {
            if(info) {
                info->hitIndex=static_cast<int>(i);info->left=rect.left;info->top=rect.top;
                info->right=rect.right;info->bottom=rect.bottom;
            }
            return true;
        }
    }
    return false;
}

void SetInteractiveSitePreview(uint64_t siteId,const Plan* plan)
{
    std::lock_guard<std::mutex> lock(interactivePreviewMutex);
    if(!plan){interactivePreviewActive=false;interactivePreviewSiteId=0;return;}
    interactivePreviewPlan=*plan;interactivePreviewSiteId=siteId;interactivePreviewActive=true;
}

void SetWinterWireframeMode(bool enabled)
{
    winterWireframeMode.store(enabled,std::memory_order_relaxed);
}
}
