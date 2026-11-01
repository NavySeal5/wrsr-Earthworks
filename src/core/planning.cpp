#include "planning.h"
#include <algorithm>

namespace ew {
Point Plan::Vertex(int x, int z, bool proposed) const
{
    return {std::min(a.x, b.x) + Width() * static_cast<float>(x) / GridCells,
        proposed ? target : ground[static_cast<size_t>(z * GridSide + x)],
        std::min(a.z, b.z) + Depth() * static_cast<float>(z) / GridCells};
}
namespace {
struct Vertex { double x, z, d; };
// Clip a triangle's linearly interpolated depth at zero, then integrate the
// resulting polygon. Mixed cut/fill cells therefore do not cancel each other.
double PositiveVolume(const std::array<Vertex, 3>& triangle, double sign)
{
    std::array<Vertex, 6> polygon{};
    int count = 0;
    for (int i = 0; i < 3; ++i) {
        Vertex a = triangle[static_cast<size_t>(i)];
        Vertex b = triangle[static_cast<size_t>((i + 1) % 3)];
        a.d *= sign; b.d *= sign;
        if (a.d >= 0) polygon[static_cast<size_t>(count++)] = a;
        if ((a.d > 0 && b.d < 0) || (a.d < 0 && b.d > 0)) {
            const double t = a.d / (a.d - b.d);
            polygon[static_cast<size_t>(count++)] = {a.x + t * (b.x - a.x), a.z + t * (b.z - a.z), 0};
        }
    }
    double result = 0;
    for (int i = 1; i + 1 < count; ++i) {
        const auto& a = polygon[0];
        const auto& b = polygon[static_cast<size_t>(i)];
        const auto& c = polygon[static_cast<size_t>(i + 1)];
        const double area = std::abs((b.x-a.x)*(c.z-a.z)-(c.x-a.x)*(b.z-a.z)) * 0.5;
        result += area * (a.d + b.d + c.d) / 3;
    }
    return result;
}
}
Volumes Estimate(const Plan& plan)
{
    Volumes out;
    if (!plan.complete) return out;
    for (int z = 0; z < GridCells; ++z) for (int x = 0; x < GridCells; ++x) {
        const Point p[] = {plan.Vertex(x,z,false), plan.Vertex(x+1,z,false),
            plan.Vertex(x+1,z+1,false), plan.Vertex(x,z+1,false)};
        for (int t = 0; t < 2; ++t) {
            const Point q[] = {p[0], p[t+1], p[t+2]};
            std::array<Vertex,3> triangle{};
            for (size_t i=0; i<3; ++i) triangle[i] = {q[i].x,q[i].z,static_cast<double>(plan.target)-q[i].y};
            out.fill += PositiveVolume(triangle,1);
            out.cut += PositiveVolume(triangle,-1);
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
