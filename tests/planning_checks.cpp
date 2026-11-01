#include "../src/core/planning.h"
#include "../src/core/terrain_grid.h"
#include <cstdlib>
#include <iostream>
void Near(double actual,double expected,const char* name)
{
    if (std::abs(actual-expected)>0.02) {
        std::cerr<<name<<": expected "<<expected<<", got "<<actual<<'\n'; std::exit(1);
    }
}
int main()
{
    ew::Plan p; p.first=true; p.complete=true; p.a={120,0,240}; p.b={0,0,0}; p.target=10;
    p.ground.fill(0);
    auto v=ew::Estimate(p); Near(v.fill,288000,"flat fill"); Near(v.cut,0,"flat cut");
    p.target=-2; v=ew::Estimate(p); Near(v.cut,57600,"lowering"); Near(v.fill,0,"lowering fill");
    p.target=0;
    for(int z=0;z<ew::GridSide;++z) for(int x=0;x<ew::GridSide;++x)
        p.ground[static_cast<size_t>(z*ew::GridSide+x)]=static_cast<float>(x)/ew::GridCells*20-10;
    v=ew::Estimate(p); Near(v.fill,72000,"sloping terrain fill"); Near(v.cut,72000,"sloping terrain cut");
    // Crossing at an off-grid position exercises mixed-sign triangle clipping.
    p.target=1.25f; v=ew::Estimate(p); Near(v.fill,91125,"mixed triangles fill"); Near(v.cut,55125,"mixed triangles cut");
    ew::History h; ew::Plan empty; h.Commit(empty);
    auto old=h.Undo(p); if (old.first || !h.CanRedo()) return 1;
    old=h.Redo(old); Near(old.target,1.25,"redo target");
    h.Commit(old); h.Undo(empty); h.Commit(p); if (h.CanRedo()) return 1;
    h.Reset(); if(h.CanUndo() || h.CanRedo()) return 1;
    ew::TerrainGrid grid{-100,-100,4,4,1000,64,64};
    ew::GridRect rect;
    p.a={-62,0,-62};p.b={-39,0,-39};
    if(!ew::SnapInside(p,grid,&rect) || rect.x0!=10 || rect.x1!=15 || rect.Count()!=36) return 1;
    if(rect.Contains(9,10) || !rect.Contains(10,10) || rect.Contains(16,15)) return 1;
    p.a={-99,0,-99};if(ew::SnapInside(p,grid,&rect)) return 1;
    p.a={-60,0,-60};p.b={-59,0,-59};if(ew::SnapInside(p,grid,&rect)) return 1;
    std::cout<<"Geometry: reversed corners, cut/fill, mixed-sign clipping; draft undo/redo/reset passed.\n";
}
