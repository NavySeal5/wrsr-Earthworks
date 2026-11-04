# Stage 12.1 — First model draft

The approved target is the initial two-bay canopy, a single road-facing entrance edge, a dispatch office at the left, and a visible decorative tank. The user approved this concept for modeling on 4 November 2026. The superseded through-access option is not used.

## Draft geometry

- Approximate footprint: 43 m wide by 25 m deep including the short front apron and tank pad.
- Highest point: approximately 7.4 m above the foundation.
- Two open vehicle bays under a pitched metal canopy, divided by a central support line.
- Dispatch office with front windows, door, plinth, shallow roof, and restrained red trim.
- A vertical decorative tank is visible beside the office. It has no pump or hose and makes no fuel-service promise.
- The model is authored from named low-poly objects using Blender, with separate material groups. Generated OBJ/MTL is intended for ModelViewer import; the `.blend` file and generator remain editable project source.
- The ModelViewer export is reduced to 10 material-group meshes, 3,590 vertices total and 6,596 triangles; the exported OBJ contains no quads/ngons. The official modelling notes recommend triangles and cite an approximate 19,000-vertex per-object limit: [Modelviewer](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Modelviewer), [Modelling](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Modelling).

Files:

- `assets/building/generate_earthworks_facility.py` — deterministic Blender authoring script.
- `assets/building/generated/earthworks_facility.blend` — editable scene.
- `assets/building/generated/earthworks_facility.obj` and `.mtl` — export for ModelViewer.
- `docs/mockups/STAGE12_FACILITY_MODEL_PREVIEW.png` — offline render for design review.

Regenerate with the project Blender runtime:

```powershell
tools\runtime\blender-5.2.3\blender-5.2.3-windows-x64\blender.exe --background --factory-startup --python assets/building/generate_earthworks_facility.py
```

## Remaining 12.1 work and limits

The ModelViewer conversion to the game's native model and material formats has not been completed, so there is not yet a placeable item. After the user authorized desktop use, the computer-use tool tried to launch `ModelViewer.exe` twice; both launches exited without exposing a targetable window. The native item script still uses the old construction-office donor and must not be treated as this model's item.

Parking routes and road connections also remain unimplemented. They must be authored together with two real native parking slots after the model is converted. No in-game test was run. Stage 12.1 remains open until the user places the converted item and checks scale, orientation, access, entry/exit routing, save/reload and demolition in a disposable save.

### Native routing evidence

- The `construction_office3.ini` donor has 16 `$VEHICLE_PARKING` entries, per-slot advanced waypoint records for some parking IDs, a road connector and dead-road connector records. It is not suitable unchanged for this two-bay footprint.
- The native `muddy_construction.ini` is a `$TYPE_CONSTRUCTION_OFFICE` with four `$VEHICLE_PARKING` lines, no advanced waypoints, and one straight `$CONNECTION_ROAD` across its frontage. This confirms that the game's native format supports a small, direct-route office layout.
- The generated footprint places the two bays side by side with their open edges facing the same single frontage. The likely first route experiment is one parking position per bay and a single straight road connector across that frontage. Exact coordinates and connector orientation are intentionally not committed yet: ModelViewer has no targetable window in this session, and the game must resolve route direction and parked orientation in the disposable-save check.
- The existing development item still uses the old construction-office model, 20-worker test value and inherited 16 parking positions. Do not use it to judge the custom model or assume it meets the Stage 12 requirements.
