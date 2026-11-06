# Stage 12 — Large facility user review

The Large Earthworks Facility is authored as an editable Blender scene and OBJ/MTL source. Its first in-game check still requires ModelViewer conversion and a user-run game session.

## Decisions implemented

- Twenty independently accessible vehicle stalls, ten in each side garage wing; all positions are covered and face inward.
- Two loading/unloading stations on the yard-facing main building.
- Two-storey raised central section on the rear service building, about 9.9 m high; the side wings remain single-storey.
- One road connector centered at the U opening / road-approach end.
- Proportions scaled toward the five-view concept: the structural U is approximately 75 m wide by 75 m deep; the rear hall is approximately 74 m long compared with 59.5 m side wings. The native asphalt court is approximately 75 m by 94 m including the forecourt/road approach. These are modeling dimensions, not a final gameplay footprint balance.
- `$TYPE_ROADDEPO`, no workers, no fuel storage, and the existing plugin-controlled excavator/bulldozer filter.
- Asphalt native surface metadata for the yard; ground and parking markings are not part of the mesh.
- Material coefficients 30% above Medium: asphalt 1.755, brick 0.819, wood 0.351, roof material 0.936.

## Prepare the ModelViewer source

1. Run `scripts/Prepare-EarthworksLargeModelViewer.ps1` from the mod root. It copies the generated OBJ, MTL and roof texture into a marked object folder under `media_soviet/workshop_wip/9100000014/EarthworksDevelopmentFacilityLarge`.
2. Open ModelViewer manually and import `earthworks_facility_large.obj` from that object folder. Keep the imported material groups and their order intact.
3. Save the model as `model.nmf` in that same object folder. ModelViewer's Save As appends `.nmf`; enter only `model` as the filename.
4. Copy the resulting `model.nmf` to `assets/building/large/native/earthworks_facility_large.nmf`.
5. Run `scripts/Stage-EarthworksBuilding-Large.ps1` to assemble the Workshop test item. The general diagnostics staging script will include it automatically once the native NMF exists.

## Fresh-world review checklist

Use a fresh disposable save and select **Earthworks Facility (Large)** from the construction-office category.

1. Check construction costs and phase progress. Confirm the listed resource quantities are the documented coefficients and the blueprint advances through construction.
2. Inspect the enlarged footprint and yard surface. Confirm the asphalt court fills the yard and meets the single road connector at the U opening / road-approach end.
3. Confirm the U layout reads clearly: main service block at the closed end, its two-storey center above the dispatch section, single-storey side wings, no stalls in front of other stalls, and containers outside vehicle paths.
4. Test the single road approach and confirm its side and outward-facing direction, then confirm vehicles can enter and turn through the central aisle.
5. Count twenty native parking positions: ten inside each garage wing. Confirm none are on the main-building roof, and every position faces inward toward the yard. Try representative long excavators and bulldozers in all positions; inspect clipping, spacing, and pathing.
6. Confirm both loading/unloading positions are outside the parking paths and do not overlap the building or road approach.
7. Confirm the native panel shows twenty vehicle positions, admits excavators and bulldozers, rejects other vehicle classes, and exposes no worker or fuel requirement.
8. Save, reload, and confirm the placed building retains its vehicle state. Demolish it and check that surface-area metadata is removed with the building.

Do not treat this prototype as balance-complete. Review the footprint, pathing, station placement and construction phases first; costs and detailed machine capacity remain tunable.
