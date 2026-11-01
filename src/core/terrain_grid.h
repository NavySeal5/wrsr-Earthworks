#pragma once
#include "planning.h"
#include <algorithm>

namespace ew {
struct TerrainGrid {
    float originX=0, originZ=0, stepX=0, stepZ=0, heightScale=0;
    int width=0, depth=0;
    Point At(int x,int z) const { return {originX+stepX*static_cast<float>(x),0,originZ+stepZ*static_cast<float>(z)}; }
};
struct GridRect {
    int x0=0,z0=0,x1=-1,z1=-1;
    int Count() const { return (x1-x0+1)*(z1-z0+1); }
    bool Contains(int x,int z) const { return x>=x0 && x<=x1 && z>=z0 && z<=z1; }
};
inline bool SnapInside(const Plan& p,const TerrainGrid& g,GridRect* r)
{
    if (!p.complete || !std::isfinite(g.stepX) || !std::isfinite(g.stepZ) || g.stepX<=0 || g.stepZ<=0) return false;
    const float x0=(std::min(p.a.x,p.b.x)-g.originX)/g.stepX;
    const float x1=(std::max(p.a.x,p.b.x)-g.originX)/g.stepX;
    const float z0=(std::min(p.a.z,p.b.z)-g.originZ)/g.stepZ;
    const float z1=(std::max(p.a.z,p.b.z)-g.originZ)/g.stepZ;
    if (!std::isfinite(x0) || !std::isfinite(x1) || !std::isfinite(z0) || !std::isfinite(z1)
        || x0<3 || z0<3 || x1>static_cast<float>(g.width-4) || z1>static_cast<float>(g.depth-4)) return false;
    *r={static_cast<int>(std::ceil(x0)),static_cast<int>(std::ceil(z0)),static_cast<int>(std::floor(x1)),static_cast<int>(std::floor(z1))};
    return r->x1>r->x0 && r->z1>r->z0;
}
}
