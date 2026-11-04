# Stage 12.1 — First model and placeable item

The approved target is the initial two-bay canopy, a single road-facing entrance edge, a dispatch office at the left, and a visible decorative tank. The user approved this concept for modeling on 4 November 2026. The superseded through-access option is not used.

## Draft geometry

- Approximate footprint: 43 m wide by 25 m deep including the short front apron and tank pad.
- Highest point: approximately 7.4 m above the foundation.
- Two open vehicle bays under a pitched metal canopy, divided by a central support line.
- Dispatch office with front windows, door, plinth, shallow roof, and restrained red trim.
- A vertical decorative tank is visible beside the office. It has no pump or hose and makes no fuel-service promise.
- The model is authored from named low-poly objects using Blender, with separate material groups. The `.blend` file and generator remain editable project source.
- The OBJ export has 10 material-group meshes, 3,590 vertices total and 6,596 triangles. It contains no quads/ngons. The official modelling notes recommend triangles and cite an approximate 19,000-vertex per-object limit: [Modelviewer](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Modelviewer), [Modelling](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Modelling).

## Project assets

- `assets/building/generate_earthworks_facility.py` — deterministic Blender authoring script.
- `assets/building/generated/earthworks_facility.blend` — editable scene.
- `assets/building/generated/earthworks_facility.obj` and `.mtl` — ModelViewer import source.
- `assets/building/native/earthworks_facility.nmf` — native mesh exported by ModelViewer.
- `assets/building/native/earthworks_facility.mtl` — native submaterials using the authored Blender diffuse/ambient colors.
- `assets/building/native/previewimage.png` — square 512 px Workshop preview generated from the render.
- `docs/mockups/STAGE12_FACILITY_MODEL_PREVIEW.png` — offline render for design review.

Regenerate the editable source and OBJ with the project Blender runtime:

```powershell
tools\runtime\blender-5.2.3\blender-5.2.3-windows-x64\blender.exe --background --factory-startup --python assets/building/generate_earthworks_facility.py
```

## Native conversion and item

The user opened ModelViewer for the 4 November 2026 authoring session. It imported the OBJ, saved the native NMF, and saved a native material template. The NMF was copied into this repository and given the canonical `.nmf` extension; the native material file was populated from the OBJ's authored diffuse and ambient colors. ModelViewer accepted the material when loaded from `media_soviet`. Its preview showed color separation, though its close camera and bright lighting are not a game-appearance check.

The first game screenshot showed black materials and donor-like texture fragments. The game error log directly identified ten failed lookups for `media_soviet/buildings/blankdiffuse.dds`; that file exists at `media_soviet/blankdiffuse.dds`. The native MTL now references `blankdiffuse.dds` without the incorrect `buildings/` prefix, and the Workshop item was restaged. A fresh game load is still needed to confirm that the material and atlas artifacts are resolved.

`scripts/Stage-EarthworksBuilding.ps1` stages the custom mesh/material and preview as Workshop item `9100000012/EarthworksDevelopmentFacility`. It retains the native construction-office class and support files, provides exactly two `$VEHICLE_PARKING` entries and a straight single-front `$CONNECTION_ROAD`, and keeps five worker places as a temporary test setting. The bay centers and connector are provisional coordinates in model space. This draft does not establish that the game will interpret approach direction or parking headings as intended.

The inherited bounding-box/fire support files and construction-office type remain prototype scaffolding. The building-specific UI and dynamic worker capacity are not implemented in 12.1; those are later 12.2/12.3 work. The tank is decorative. No game test was run. Follow [the Stage 12.1 test instructions](STAGE12_1_TESTING.md); 12.1 remains open until placement, access, routing, save/reload and demolition are checked by the user.

### Native routing evidence

- The `construction_office3.ini` donor has 16 `$VEHICLE_PARKING` entries, per-slot advanced waypoint records for some parking IDs, a road connector and dead-road connector records. It is not suitable unchanged for this two-bay footprint.
- The native `muddy_construction.ini` is a `$TYPE_CONSTRUCTION_OFFICE` with four `$VEHICLE_PARKING` lines, no advanced waypoints, and one straight `$CONNECTION_ROAD` across its frontage. This confirms that the game's native format supports a small, direct-route office layout.
- The custom item places the two bays side by side with their open edges facing one frontage. Its initial route experiment uses positions `(0.7, 0, 0)` and `(13.3, 0, 0)`, each oriented toward negative Z, with a straight connector spanning the fronts at Z `-14.5`. Verify the frontage, road snapping, entry, parking orientation, exit, and interaction between bays in game.
- The previous large-office model, 20-worker test value and inherited 16 parking points are no longer used by the Stage 12 item. It uses a temporary five-worker value and two native parking entries; dynamic worker limits and final values remain unimplemented.
