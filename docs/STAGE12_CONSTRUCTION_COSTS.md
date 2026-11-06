# Stage 12 — construction cost notes

## How the values work

Construction requirements are declared in the Workshop building's `building.ini`, in phases. A phase begins with `$COST_WORK <construction type> <coefficient>`. `$COST_WORK_BUILDING_NODE <mesh-name>` assigns an NMF mesh node to that phase. `$COST_RESOURCE_AUTO <resource-name> <coefficient>` assigns material consumption to the phase's construction geometry.

The resource coefficients are **not literal total units**. The game calculates the displayed total from the coefficient and the assigned construction nodes. A coefficient of `100.0` therefore made the test costs enormous; it did not mean “require 100 units.” Keep the nodes grouped with their intended phase and inspect the build-menu result in game after changing coefficients.

## Current Earthworks Facility baseline

The Small facility now uses 75% of the installed vanilla construction-office recipe. Medium uses 120% of Small. Large targets about 130% of Medium's displayed material totals. Its per-node coefficients are lower because the Large model assigns much more construction geometry to each phase. Values are rounded to three decimal places in generated configs.

| Phase | Work coefficient | Small resources | Medium resources | Large resources |
|---|---:|---|---|---|
| Groundworks | `0.0` | `ground_asphalt 1.125` | `ground_asphalt 1.350` | `ground_asphalt 0.336` |
| Brick Laying | `1.0` | `wall_brick 0.525`, `wall_wood 0.225` | `wall_brick 0.630`, `wall_wood 0.270` | `wall_brick 0.147`, `wall_wood 0.063` |
| Rooftop Building | `1.0` | `roof_woodsteel 0.600` | `roof_woodsteel 0.720` | `roof_woodsteel 0.289` |

These coefficient sets target roughly 130% of Medium's displayed phase totals after normalizing for the generated models' surface area in the construction-node meshes. The Large-to-Medium area ratios used were about 5.23 for Groundworks, 5.56 for Brick Laying and 3.23 for Rooftop Building. This is a geometry-based estimate; it is not confirmed until the game displays matching quantities in fresh build-menu previews. Groundworks work remains at `0.0`; only material coefficients changed.

Groundworks nodes use `mesh_foundation_concrete`; the Large variant's service plinth and stair concrete are grouped into that same mesh. Brick Laying uses `mesh_dispatch_concrete`, `mesh_window_frames`, `mesh_safety_red`, and `mesh_office_window_glass`. Rooftop Building for Small and Medium includes `mesh_canopy_structural_steel`, `mesh_decorative_tank_painted_steel`, `mesh_tank_cap_steel`, `mesh_corrugated_galvanized_roof`, and `mesh_office_roof_graphite`. Large's roof phase uses `mesh_office_roof_graphite`, `mesh_corrugated_galvanized_roof`, `mesh_canopy_structural_steel`, `mesh_window_frames`, `mesh_safety_red`, `mesh_office_window_glass`, `mesh_container_oxide_red`, and `mesh_container_blue`. The Groundworks vehicle work station is attached to `mesh_foundation_concrete`. Native `$CONNECTIONS_ROAD_DEAD_SQUARE` metadata supplies the asphalt court; it is not an NMF ground mesh.

## Changing costs

1. Edit `$COST_WORK` or `$COST_RESOURCE_AUTO` lines in the corresponding Small, Medium or Large staging script; each generates that variant's Workshop `building.ini`.
2. Keep resource lines inside the intended phase, and use resource IDs accepted by the game (for example `wall_brick`, `ground_asphalt`, `roof_woodsteel`).
3. Close the game and project-local RML, then run `launch-prototype.cmd` to stage the updated building.
4. Check the cost summary on a fresh build-menu preview in a disposable save. Existing placed buildings may retain their previous construction requirements.
5. Record the resulting totals and adjust coefficients in small steps. If construction again shows all zeros, check that the game-generated `building.bbox` and `building.fire` correspond to the current NMF; the staging script invalidates them when the NMF hash changes.

The vanilla comparison values were read from the installed `media_soviet/buildings_types/construction_office.ini`. Small and Medium retain their accepted 75%-of-vanilla and 120%-of-Small coefficients. Large now targets 130% of Medium's displayed phase totals, adjusted for mesh geometry; the user must confirm the actual displayed quantities in game. Final balancing remains open.
