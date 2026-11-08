// SPDX-License-Identifier: GPL-3.0-only
#include "game_underlay.h"
#include "imgui.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace ew {
namespace {
// WRSR 1.1.1.9 interface routine. The prologue was read from the pinned game
// build; keep this checked so an update cannot splice a hook into new code.
constexpr size_t UiRoutineRva = 0x317DB0;
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
constexpr size_t GroupCount = 7;
constexpr float ShadeRowPixels = 5.0f;
constexpr size_t MaxNativeUiRects = 8192;

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
struct UiRect { float left=0,top=0,right=0,bottom=0; };
std::array<UiRect,MaxNativeUiRects> nativeUiRects{};
size_t nativeUiRectCount=0;
std::shared_mutex nativeUiMutex;
std::atomic<bool> nativeUiCaptureActive{false},nativeUiCaptureInstalled{false};
thread_local bool drawingEarthworksUnderlay=false;

struct SegmentGroup { float values[MaxSegments * 4]{}; int count = 0; ImU32 color = 0; float width = 1.5f; };
SegmentGroup groups[GroupCount];

void ResetGroups()
{
    groups[0].color = IM_COL32(55,220,200,225); groups[0].width = 1.5f;
    groups[1].color = IM_COL32(255,175,55,225); groups[1].width = 1.5f;
    groups[2].color = IM_COL32(175,180,190,210); groups[2].width = 1.5f;
    groups[3].color = IM_COL32(205,210,220,235); groups[3].width = 2.5f;
    groups[4].color = IM_COL32(245,250,255,255); groups[4].width = 2.5f;
    groups[5].color = IM_COL32(62,221,166,55); groups[5].width = ShadeRowPixels;
    groups[6].color = IM_COL32(255,159,67,55); groups[6].width = ShadeRowPixels;
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

void Collect(const Snapshot& s, float width, float height,const Plan* livePlan,uint64_t liveSiteId)
{
    ResetGroups();
    if (!s.ready || !s.cameraValid) return;
    for(size_t i=0;i<s.projectCount;++i) {
        const auto& project=s.projects[i];
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
        else if(project.status==ProjectStatus::Cancelled) DrawOutline(s,project.outline,1,width,height);
    }
    const auto& p=livePlan&&liveSiteId==0?*livePlan:s.plan;
    if (!p.first) return;
    if (!p.complete) {
        for(unsigned i=0;i<p.pointCount;++i) {
            if(i+1<p.pointCount)AddLine(s,p.points[i],p.points[i+1],0,width,height);
        }
        if(panelOpen&&s.cursorValid&&s.mode==PickMode::DrawSite&&p.pointCount) {
            if(s.shapeMode!=SiteShapeMode::FreeDraw&&p.pointCount==1) {
                Point corners[4]{};BuildTwoCornerShape(p.points[0],s.cursor,s.shapeMode,corners);
                for(unsigned i=0;i<4;++i)AddLine(s,corners[i],corners[(i+1)%4],0,width,height);
            } else AddLine(s,p.points[p.pointCount-1],s.cursor,0,width,height);
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

void HookedPanelDraw(void* panel,float u0,float v0,float u1,float v1,float rotation,bool flag)
{
    if(nativeUiCaptureActive.load(std::memory_order_relaxed)&&!drawingEarthworksUnderlay)
        RecordNativeUiRect(panel);
    if(originalPanelDraw)originalPanelDraw(panel,u0,v0,u1,v1,rotation,flag);
}

void BeginNativeUiCapture()
{
    std::unique_lock lock(nativeUiMutex);
    nativeUiRectCount=0;
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
    return true;
}

bool NativeUiOccludes(float x,float y)
{
    if(!std::isfinite(x)||!std::isfinite(y))return false;
    if(!nativeUiCaptureInstalled.load(std::memory_order_acquire)) {
        if(!boundHost||!boundHost->exeBase)return false;
        const float width=static_cast<float>(*reinterpret_cast<const int*>(boundHost->exeBase+ScreenWidthRva));
        const float height=static_cast<float>(*reinterpret_cast<const int*>(boundHost->exeBase+ScreenHeightRva));
        return x<=65.0f||y<=85.0f||x>=width-300.0f||y>=height-150.0f;
    }
    std::shared_lock lock(nativeUiMutex);
    for(size_t i=0;i<nativeUiRectCount;++i) {
        const auto& rect=nativeUiRects[i];
        if(x>=rect.left&&x<=rect.right&&y>=rect.top&&y<=rect.bottom)return true;
    }
    return false;
}

void SetInteractiveSitePreview(uint64_t siteId,const Plan* plan)
{
    std::lock_guard<std::mutex> lock(interactivePreviewMutex);
    if(!plan){interactivePreviewActive=false;interactivePreviewSiteId=0;return;}
    interactivePreviewPlan=*plan;interactivePreviewSiteId=siteId;interactivePreviewActive=true;
}
}
