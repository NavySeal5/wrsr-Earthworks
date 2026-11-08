#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace ew {
struct Point { float x = 0, y = 0, z = 0; };
constexpr int GridCells = 24;
constexpr int GridSide = GridCells + 1;
constexpr size_t MaxSitePoints = 32;
constexpr float MinSitePointSpacing = 5.0f;
constexpr double MaxRoadGrade = 0.12;
constexpr double VanillaTerrainReferenceGrade = 20.0 / 33.0;
enum class SiteShapeMode : uint8_t { FreeDraw, Square, Rectangle };
void BuildTwoCornerShape(Point anchor,Point cursor,SiteShapeMode mode,Point (&corners)[4]);
struct SiteOutline {
    std::array<Point, MaxSitePoints> points{};
    uint8_t pointCount = 0;
    bool Contains(float x,float z) const;
};
struct Plan {
    bool first = false, complete = false;
    // a/b/target are retained for legacy-state decoding and diagnostics.
    Point a{}, b{};
    float target = 0;
    std::array<Point, MaxSitePoints> points{};
    uint8_t pointCount = 0;
    bool safeRoadAngle = false;
    bool safeAngle = false;
    std::array<std::array<unsigned,3>, MaxSitePoints-2> triangles{};
    uint8_t triangleCount = 0;
    std::array<float, GridSide * GridSide> ground{};
    float MinX() const;
    float MaxX() const;
    float MinZ() const;
    float MaxZ() const;
    float Width() const { return MaxX()-MinX(); }
    float Depth() const { return MaxZ()-MinZ(); }
    double FootprintArea() const;
    bool Contains(float x,float z) const;
    bool TargetAt(float x,float z,float* out) const;
    bool GroundAt(float x,float z,float* out) const;
    bool BlendedTargetAt(float x,float z,float blendWidth,float* out) const;
    void RefreshBounds();
    Point Vertex(int x, int z, bool proposed,float blendWidth=0.0f) const;
};
bool ValidFootprint(const Plan& plan);
double MaximumSurfaceGrade(const Plan& plan);
bool FootprintsOverlap(const Plan& a,const Plan& b);
bool ContainsPolygon(const Point* points,unsigned pointCount,float x,float z);
struct Volumes { double cut = 0, fill = 0; };
Volumes Estimate(const Plan& plan,float blendWidth=0.0f);
class History {
public:
    void Commit(const Plan& plan);
    Plan Undo(const Plan& current);
    Plan Redo(const Plan& current);
    void Reset();
    bool CanUndo() const { return !undo.empty(); }
    bool CanRedo() const { return !redo.empty(); }
private:
    std::vector<Plan> undo, redo;
};
}
