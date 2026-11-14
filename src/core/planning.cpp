#include "planning.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace ew {
void BuildTwoCornerShape(Point anchor,Point cursor,SiteShapeMode mode,Point (&corners)[4])
{
    float x=cursor.x,z=cursor.z;
    if(mode==SiteShapeMode::Square) {
        const float side=std::max(std::abs(x-anchor.x),std::abs(z-anchor.z));
        const float dx=x-anchor.x,dz=z-anchor.z;
        x=anchor.x+(dx<0.0f?-side:side);
        z=anchor.z+(dz<0.0f?-side:side);
    }
    corners[0]=anchor;
    corners[1]={x,anchor.y,anchor.z};
    corners[2]={x,anchor.y,z};
    corners[3]={anchor.x,anchor.y,z};
}

namespace {
using Triangle = std::array<unsigned,3>;
double Cross(const Point& a,const Point& b,const Point& c)
{
    return (static_cast<double>(b.x)-a.x)*(static_cast<double>(c.z)-a.z)
        -(static_cast<double>(b.z)-a.z)*(static_cast<double>(c.x)-a.x);
}
double PolygonArea(const Plan& p)
{
    double area=0;
    for(unsigned i=0;i<p.pointCount;++i) {
        const auto& a=p.points[i];const auto& b=p.points[(i+1)%p.pointCount];
        area+=static_cast<double>(a.x)*b.z-static_cast<double>(b.x)*a.z;
    }
    return area*0.5;
}
bool OnSegment(const Point& a,const Point& b,const Point& p)
{
    constexpr double eps=1e-5;
    return std::abs(Cross(a,b,p))<=eps && p.x>=std::min(a.x,b.x)-eps && p.x<=std::max(a.x,b.x)+eps
        && p.z>=std::min(a.z,b.z)-eps && p.z<=std::max(a.z,b.z)+eps;
}
bool SegmentsIntersect(const Point& a,const Point& b,const Point& c,const Point& d)
{
    const double abC=Cross(a,b,c),abD=Cross(a,b,d),cdA=Cross(c,d,a),cdB=Cross(c,d,b);
    if(((abC>0&&abD<0)||(abC<0&&abD>0))&&((cdA>0&&cdB<0)||(cdA<0&&cdB>0)))return true;
    return OnSegment(a,b,c)||OnSegment(a,b,d)||OnSegment(c,d,a)||OnSegment(c,d,b);
}
bool PointInTriangle(const Point& p,const Point& a,const Point& b,const Point& c,double orientation)
{
    constexpr double eps=1e-6;
    return orientation*Cross(a,b,p)>=-eps&&orientation*Cross(b,c,p)>=-eps&&orientation*Cross(c,a,p)>=-eps;
}
bool Triangulate(const Plan& p,std::vector<Triangle>* triangles)
{
    if(!triangles||p.pointCount<3||p.pointCount>MaxSitePoints)return false;
    triangles->clear();triangles->reserve(p.pointCount-2);
    const double orientation=PolygonArea(p)>=0?1.0:-1.0;
    std::vector<unsigned> remaining;remaining.reserve(p.pointCount);
    for(unsigned i=0;i<p.pointCount;++i)remaining.push_back(i);
    while(remaining.size()>3) {
        bool clipped=false;
        for(size_t i=0;i<remaining.size();++i) {
            const unsigned a=remaining[(i+remaining.size()-1)%remaining.size()],b=remaining[i],c=remaining[(i+1)%remaining.size()];
            const auto& pa=p.points[a];const auto& pb=p.points[b];const auto& pc=p.points[c];
            if(orientation*Cross(pa,pb,pc)<=1e-7)continue;
            bool contains=false;
            for(unsigned test:remaining)if(test!=a&&test!=b&&test!=c&&PointInTriangle(p.points[test],pa,pb,pc,orientation)){contains=true;break;}
            if(contains)continue;
            triangles->push_back({a,b,c});remaining.erase(remaining.begin()+static_cast<std::ptrdiff_t>(i));clipped=true;break;
        }
        if(!clipped)return false;
    }
    triangles->push_back({remaining[0],remaining[1],remaining[2]});
    return true;
}
bool TriangleHeight(const Point& p,const Point& a,const Point& b,const Point& c,float* height)
{
    const double area=Cross(a,b,c);if(std::abs(area)<1e-8)return false;
    const double wa=Cross(p,b,c)/area,wb=Cross(a,p,c)/area,wc=Cross(a,b,p)/area;
    constexpr double eps=1e-5;
    if(wa<-eps||wb<-eps||wc<-eps)return false;
    *height=static_cast<float>(wa*a.y+wb*b.y+wc*c.y);return std::isfinite(*height);
}
}
Point SiteCentroid(const Plan& plan) noexcept
{
    if(plan.pointCount>=3) {
        double twiceArea=0.0,weightedX=0.0,weightedZ=0.0;
        for(unsigned i=0;i<plan.pointCount;++i) {
            const auto& a=plan.points[i];
            const auto& b=plan.points[(i+1u)%plan.pointCount];
            const double cross=static_cast<double>(a.x)*b.z-static_cast<double>(b.x)*a.z;
            twiceArea+=cross;
            weightedX+=(static_cast<double>(a.x)+b.x)*cross;
            weightedZ+=(static_cast<double>(a.z)+b.z)*cross;
        }
        if(std::abs(twiceArea)>1.0e-6)
            return {static_cast<float>(weightedX/(3.0*twiceArea)),0,
                static_cast<float>(weightedZ/(3.0*twiceArea))};
    }
    return {(plan.a.x+plan.b.x)*0.5f,0,(plan.a.z+plan.b.z)*0.5f};
}
float Plan::MinX() const { if(pointCount){float v=points[0].x;for(unsigned i=1;i<pointCount;++i)v=std::min(v,points[i].x);return v;}return std::min(a.x,b.x); }
float Plan::MaxX() const { if(pointCount){float v=points[0].x;for(unsigned i=1;i<pointCount;++i)v=std::max(v,points[i].x);return v;}return std::max(a.x,b.x); }
float Plan::MinZ() const { if(pointCount){float v=points[0].z;for(unsigned i=1;i<pointCount;++i)v=std::min(v,points[i].z);return v;}return std::min(a.z,b.z); }
float Plan::MaxZ() const { if(pointCount){float v=points[0].z;for(unsigned i=1;i<pointCount;++i)v=std::max(v,points[i].z);return v;}return std::max(a.z,b.z); }
double Plan::FootprintArea() const { return pointCount<3?static_cast<double>(Width())*Depth():std::abs(PolygonArea(*this)); }
void Plan::RefreshBounds()
{
    triangleCount=0;
    if(!pointCount)return;
    a=points[0];b=points[0];
    for(unsigned i=1;i<pointCount;++i){a.x=std::min(a.x,points[i].x);a.z=std::min(a.z,points[i].z);b.x=std::max(b.x,points[i].x);b.z=std::max(b.z,points[i].z);}
    target=points[0].y;
    if(pointCount>=3) {
        std::vector<Triangle> generated;
        if(Triangulate(*this,&generated)) {
            triangleCount=static_cast<uint8_t>(std::min<size_t>(generated.size(),triangles.size()));
            for(unsigned i=0;i<triangleCount;++i)triangles[i]=generated[i];
        }
    }
}
bool ContainsPolygon(const Point* points,unsigned count,float x,float z)
{
    if(!points||count<3||count>MaxSitePoints)return false;
    const Point p{x,0,z};bool inside=false;
    for(unsigned i=0,j=count-1;i<count;j=i++) {
        const auto& previous=points[j];const auto& current=points[i];
        if(OnSegment(previous,current,p))return true;
        if(((previous.z>z)!=(current.z>z))&&(x<(static_cast<double>(current.x)-previous.x)*(z-previous.z)/(static_cast<double>(current.z)-previous.z)+previous.x))inside=!inside;
    }
    return inside;
}
bool SiteOutline::Contains(float x,float z) const { return ContainsPolygon(points.data(),pointCount,x,z); }
bool Plan::Contains(float x,float z) const
{
    if(pointCount<3)return x>=MinX()&&x<=MaxX()&&z>=MinZ()&&z<=MaxZ();
    return ContainsPolygon(points.data(),pointCount,x,z);
}
bool Plan::TargetAt(float x,float z,float* out) const
{
    if(!out)return false;
    if(pointCount<3){*out=target;return std::isfinite(target);}
    const Point query{x,0,z};
    for(unsigned i=0;i<triangleCount;++i) {
        const auto& t=triangles[i];
        if(TriangleHeight(query,points[t[0]],points[t[1]],points[t[2]],out))return true;
    }
    return false;
}
bool Plan::GroundAt(float x,float z,float* out) const
{
    if(!out||!complete||Width()<=0||Depth()<=0)return false;
    const float gx=std::clamp((x-MinX())/Width()*GridCells,0.0f,static_cast<float>(GridCells));
    const float gz=std::clamp((z-MinZ())/Depth()*GridCells,0.0f,static_cast<float>(GridCells));
    const int x0=static_cast<int>(gx),z0=static_cast<int>(gz);
    const int x1=std::min(x0+1,GridCells),z1=std::min(z0+1,GridCells);
    const float tx=gx-static_cast<float>(x0),tz=gz-static_cast<float>(z0);
    const auto at=[&](int ix,int iz){return ground[static_cast<size_t>(iz*GridSide+ix)];};
    const float top=at(x0,z0)+(at(x1,z0)-at(x0,z0))*tx;
    const float bottom=at(x0,z1)+(at(x1,z1)-at(x0,z1))*tx;
    *out=top+(bottom-top)*tz;
    return std::isfinite(*out);
}
bool Plan::BlendedTargetAt(float x,float z,float blendWidth,float* out) const
{
    if(!TargetAt(x,z,out))return false;
    if(pointCount<3||!std::isfinite(blendWidth)||blendWidth<=0||!Contains(x,z))return true;
    double nearestSquared=std::numeric_limits<double>::infinity();
    for(unsigned i=0;i<pointCount;++i) {
        const auto& edgeStart=points[i];const auto& edgeEnd=points[(i+1)%pointCount];
        const double dx=static_cast<double>(edgeEnd.x)-edgeStart.x,dz=static_cast<double>(edgeEnd.z)-edgeStart.z;
        const double lengthSquared=dx*dx+dz*dz;
        const double t=lengthSquared>0?std::clamp(((static_cast<double>(x)-edgeStart.x)*dx+(static_cast<double>(z)-edgeStart.z)*dz)/lengthSquared,0.0,1.0):0.0;
        const double ex=static_cast<double>(x)-(edgeStart.x+t*dx),ez=static_cast<double>(z)-(edgeStart.z+t*dz);
        nearestSquared=std::min(nearestSquared,ex*ex+ez*ez);
    }
    float original=0;
    if(!GroundAt(x,z,&original))return false;
    const float distance=static_cast<float>(std::sqrt(nearestSquared));
    const float t=std::clamp(distance/blendWidth,0.0f,1.0f);
    const float weight=t*t*(3.0f-2.0f*t);
    *out=original+(static_cast<float>(*out)-original)*weight;
    return std::isfinite(*out);
}
Point Plan::Vertex(int x,int z,bool proposed,float blendWidth) const
{
    const float px=MinX()+Width()*static_cast<float>(x)/GridCells;
    const float pz=MinZ()+Depth()*static_cast<float>(z)/GridCells;
    float y=ground[static_cast<size_t>(z*GridSide+x)];
    if(proposed)BlendedTargetAt(px,pz,blendWidth,&y);
    return {px,y,pz};
}
bool ValidFootprint(const Plan& p)
{
    if(p.pointCount<3||p.pointCount>MaxSitePoints||!std::isfinite(PolygonArea(p))||std::abs(PolygonArea(p))<1.0)return false;
    for(unsigned i=0;i<p.pointCount;++i) {
        const auto& a=p.points[i];const auto& b=p.points[(i+1)%p.pointCount];
        if(!std::isfinite(a.x)||!std::isfinite(a.y)||!std::isfinite(a.z))return false;
        const double dx=static_cast<double>(a.x)-b.x,dz=static_cast<double>(a.z)-b.z;
        if(dx*dx+dz*dz<static_cast<double>(MinSitePointSpacing)*MinSitePointSpacing)return false;
        for(unsigned j=i+1;j<p.pointCount;++j) {
            if(j==i||j==(i+1)%p.pointCount||i==(j+1)%p.pointCount)continue;
            const double px=static_cast<double>(a.x)-p.points[j].x,pz=static_cast<double>(a.z)-p.points[j].z;
            if(px*px+pz*pz<static_cast<double>(MinSitePointSpacing)*MinSitePointSpacing)return false;
        }
    }
    for(unsigned i=0;i<p.pointCount;++i)for(unsigned j=i+1;j<p.pointCount;++j) {
        if(j==i+1u||(i==0&&j==static_cast<unsigned>(p.pointCount-1)))continue;
        if(SegmentsIntersect(p.points[i],p.points[(i+1)%p.pointCount],p.points[j],p.points[(j+1)%p.pointCount]))return false;
    }
    std::vector<Triangle> triangles;return Triangulate(p,&triangles);
}
double MaximumSurfaceGrade(const Plan& p)
{
    if(p.pointCount<3||!ValidFootprint(p))return 0.0;
    double maximum=0.0;
    for(unsigned i=0;i<p.triangleCount;++i) {
        const auto& tri=p.triangles[i];
        const auto& a=p.points[tri[0]];const auto& b=p.points[tri[1]];const auto& c=p.points[tri[2]];
        const double ux=static_cast<double>(b.x)-a.x,uz=static_cast<double>(b.z)-a.z;
        const double vx=static_cast<double>(c.x)-a.x,vz=static_cast<double>(c.z)-a.z;
        const double uy=static_cast<double>(b.y)-a.y,vy=static_cast<double>(c.y)-a.y;
        const double determinant=ux*vz-uz*vx;
        if(std::abs(determinant)<1e-10)return std::numeric_limits<double>::infinity();
        const double gx=(uy*vz-vy*uz)/determinant;
        const double gz=(ux*vy-vx*uy)/determinant;
        maximum=std::max(maximum,std::sqrt(gx*gx+gz*gz));
    }
    return maximum;
}
bool FootprintsOverlap(const Plan& a,const Plan& b)
{
    if(!a.complete||!b.complete||!ValidFootprint(a)||!ValidFootprint(b))return false;
    if(a.MaxX()<b.MinX()||b.MaxX()<a.MinX()||a.MaxZ()<b.MinZ()||b.MaxZ()<a.MinZ())return false;
    for(unsigned i=0;i<a.pointCount;++i)for(unsigned j=0;j<b.pointCount;++j)
        if(SegmentsIntersect(a.points[i],a.points[(i+1)%a.pointCount],b.points[j],b.points[(j+1)%b.pointCount]))return true;
    return a.Contains(b.points[0].x,b.points[0].z)||b.Contains(a.points[0].x,a.points[0].z);
}
Volumes Estimate(const Plan& plan,float blendWidth)
{
    Volumes out;
    if (!plan.complete || !ValidFootprint(plan)) return out;
    const double cellWidth=static_cast<double>(plan.Width())/GridCells;
    const double cellDepth=static_cast<double>(plan.Depth())/GridCells;
    if(!std::isfinite(cellWidth)||!std::isfinite(cellDepth)||cellWidth<=0||cellDepth<=0)return out;

    // Sample sub-cell centers instead of requiring all four corners of a coarse
    // cell to fall inside the polygon. That corner requirement omitted boundary
    // cells wholesale and could erase estimates for narrow or irregular Sites.
    constexpr int samplesPerAxis=2;
    constexpr double sampleAreaFraction=1.0/(samplesPerAxis*samplesPerAxis);
    const double sampleArea=cellWidth*cellDepth*sampleAreaFraction;
    for(int z=0;z<GridCells;++z)for(int x=0;x<GridCells;++x) {
        const double x0=static_cast<double>(plan.MinX())+cellWidth*x;
        const double z0=static_cast<double>(plan.MinZ())+cellDepth*z;
        for(int sz=0;sz<samplesPerAxis;++sz)for(int sx=0;sx<samplesPerAxis;++sx) {
            const float px=static_cast<float>(x0+cellWidth*(sx+0.5)/samplesPerAxis);
            const float pz=static_cast<float>(z0+cellDepth*(sz+0.5)/samplesPerAxis);
            if(!plan.Contains(px,pz))continue;
            float proposed=0,ground=0;
            if(!plan.BlendedTargetAt(px,pz,blendWidth,&proposed)||!plan.GroundAt(px,pz,&ground))continue;
            const double delta=static_cast<double>(proposed)-ground;
            if(delta>0)out.fill+=delta*sampleArea;
            else out.cut-=delta*sampleArea;
        }
    }
    return out;
}
void History::Commit(const Plan& plan)
{
    if (undo.size() == 32) undo.erase(undo.begin());
    undo.push_back(plan); redo.clear();
}
Plan History::Undo(const Plan& current)
{
    if (undo.empty()) return current;
    redo.push_back(current); Plan out = undo.back(); undo.pop_back(); return out;
}
Plan History::Redo(const Plan& current)
{
    if (redo.empty()) return current;
    undo.push_back(current); Plan out = redo.back(); redo.pop_back(); return out;
}
void History::Reset() { undo.clear(); redo.clear(); }
}
