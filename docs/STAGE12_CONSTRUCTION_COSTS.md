# Stage 12 — construction cost notes

## How the values work

Construction requirements are declared in the Workshop building's `building.ini`, in phases. A phase begins with `$COST_WORK <construction type> <coefficient>`. `$COST_WORK_BUILDING_NODE <mesh-name>` assigns an NMF mesh node to that phase. `$COST_RESOURCE_AUTO <resource-name> <coefficient>` assigns material consumption to the phase's construction geometry.

The resource coefficients are **not literal total units**. The game calculates the displayed total from the coefficient and the assigned construction nodes. A coefficient of `100.0` therefore made the test costs enormous; it did not mean “require 100 units.” Keep the nodes grouped with their intended phase and inspect the build-menu result in game after changing coefficients.

## Current Earthworks Facility baseline

The Small facility now uses 75% of the installed vanilla construction-office recipe. The Medium facility uses 120% of the resulting Small coefficients. Values are rounded to three decimal places in the generated config.

| Phase | Work coefficient | Small resources | Medium resources |
|---|---:|---|---|
| Groundworks | `0.0` | `ground_asphalt 1.125` | `ground_asphalt 1.350` |
| Brick Laying | `1.0` | `wall_brick 0.525`, `wall_wood 0.225` | `wall_brick 0.630`, `wall_wood 0.270` |
| Rooftop Building | `1.0` | `roof_woodsteel 0.600` | `roof_woodsteel 0.720` |

These are coefficient ratios, not guaranteed displayed totals. Groundworks work remains at `0.0`; only material coefficients were changed. The game calculates displayed quantities using its construction geometry, so compare fresh build-menu previews before further balancing.

Groundworks nodes are `mesh_foundation_concrete`. Brick Laying nodes are `mesh_dispatch_concrete`, `mesh_window_frames`, `mesh_safety_red`, and `mesh_office_window_glass`. Rooftop Building nodes are `mesh_canopy_structural_steel`, `mesh_decorative_tank_painted_steel`, `mesh_tank_cap_steel`, `mesh_corrugated_galvanized_roof`, and `mesh_office_roof_graphite`. The Groundworks vehicle work station is attached to `mesh_foundation_concrete`. The asphalt forecourt is defined by native `$CONNECTIONS_ROAD_DEAD_SQUARE` metadata, not an NMF mesh node.

## Changing costs

1. Edit `$COST_WORK` or `$COST_RESOURCE_AUTO` lines in `scripts/Stage-EarthworksBuilding.ps1` for Small and `scripts/Stage-EarthworksBuilding-Medium.ps1` for Medium; each script generates its Workshop `building.ini`.
2. Keep resource lines inside the intended phase, and use resource IDs accepted by the game (for example `wall_brick`, `ground_asphalt`, `roof_woodsteel`).
3. Close the game and project-local RML, then run `launch-prototype.cmd` to stage the updated building.
4. Check the cost summary on a fresh build-menu preview in a disposable save. Existing placed buildings may retain their previous construction requirements.
5. Record the resulting totals and adjust coefficients in small steps. If construction again shows all zeros, check that the game-generated `building.bbox` and `building.fire` correspond to the current NMF; the staging script invalidates them when the NMF hash changes.

The vanilla comparison values were read from the installed `media_soviet/buildings_types/construction_office.ini`. The current 75% Small / 120%-of-Small Medium ratios are staged for user in-game confirmation; final balancing remains open.
