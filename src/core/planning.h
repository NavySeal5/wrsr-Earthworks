#pragma once
#include <array>
#include <cmath>
#include <vector>

namespace ew {
struct Point { float x = 0, y = 0, z = 0; };
constexpr int GridCells = 24;
constexpr int GridSide = GridCells + 1;
struct Plan {
    bool first = false, complete = false;
    Point a{}, b{};
    float target = 0;
    std::array<float, GridSide * GridSide> ground{};
    float Width() const { return std::abs(b.x - a.x); }
    float Depth() const { return std::abs(b.z - a.z); }
    Point Vertex(int x, int z, bool proposed) const;
};
struct Volumes { double cut = 0, fill = 0; };
Volumes Estimate(const Plan& plan);
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
