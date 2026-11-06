# Stage 12 — Large facility user review

The Large Earthworks Facility is integrated as Workshop item `9100000014` (`EarthworksDevelopmentFacilityLarge`). The project RML launcher stages the item and enables it alongside Small and Medium in the isolated Earthworks Development profile. Its in-game review remains user-run.

## Decisions implemented

- Twenty independently accessible vehicle stalls: sixteen covered in the side garage wings (eight per wing) and four open-air at the U opening; all face inward.
- Two loading/unloading stations side by side near the center of the court, close to the yard-facing main-building frontage.
- Both garage wings remain open at their terminal ends; the end walls, safety strips, and decorative ribbed garage-door assemblies were removed so those openings are not blocked.
- Parking bay markings come from the native `$VEHICLE_PARKING` segment, not separate mesh paint. Large uses 10 m segments centered on the same 20 vehicle positions as before, matching the segment length used by Small and Medium.
- Two-storey raised central section on the rear service building, about 9.9 m high; the side wings remain single-storey.
- One road connector centered at the U opening / road-approach end.
- Proportions scaled toward the five-view concept: the structural U is approximately 75 m wide by 75 m deep; the rear hall is approximately 74 m long compared with 59.5 m side wings. The native asphalt court is approximately 75 m by 94 m including the forecourt/road approach. These are modeling dimensions, not a final gameplay footprint balance.
- `$TYPE_ROADDEPO`, no workers, no fuel storage, and the existing plugin-controlled excavator/bulldozer filter.
- Asphalt native surface metadata for the yard; ground and parking markings are not part of the mesh.
- Target displayed material total about 30% above Medium, adjusted for the Large model's greater construction-node surface area: asphalt coefficient 0.336, brick 0.147, wood 0.063, roof material 0.289. The build-menu quantities still need user confirmation.

## Prepare the ModelViewer source

1. Run `scripts/Prepare-EarthworksLargeModelViewer.ps1` from the mod root. It copies the generated OBJ, MTL and roof texture into a marked object folder under `media_soviet/workshop_wip/9100000014/EarthworksDevelopmentFacilityLarge`.
2. Open ModelViewer manually and import `earthworks_facility_large.obj` from that object folder. Keep the imported material groups and their order intact.
3. Save the model as `model.nmf` in that same object folder. ModelViewer's Save As appends `.nmf`; enter only `model` as the filename.
4. Copy the resulting `model.nmf` to `assets/building/large/native/earthworks_facility_large.nmf`.
5. Run `scripts/Stage-EarthworksBuilding-Large.ps1` to assemble the Workshop test item. The general diagnostics staging script includes and enables it automatically once the native NMF exists.

## Fresh-world review checklist

Run `launch-prototype.cmd` after closing the game and project-local RML. Its staging output should say Small, Medium, and Large are enabled. Start Earthworks Development, use a fresh disposable save, and select **Earthworks Facility (Large)** from the construction-office category.

Before reviewing the building, create/select a Project, choose **Choose facility on map**, and click the Large facility. It should appear under Available facilities; press **＋ Add** and confirm it moves to Assigned facilities. The assignment recognizer now matches the generated internal object name even if the Workshop id prefix differs.

1. Check construction costs and phase progress. Compare Large's total displayed material quantities with Medium and confirm Large is about 30% higher overall. Confirm the blueprint advances through construction.
2. Inspect the enlarged footprint and yard surface. Confirm the asphalt court fills the yard and meets the single road connector at the U opening / road-approach end.
3. Confirm the U layout reads clearly: main service block at the closed end, its two-storey center above the dispatch section, single-storey side wings with both terminal garage ends open, no stalls in front of other stalls, and containers outside vehicle paths.
4. Test the single road approach and confirm its snap point remains at the U opening while its direction is reversed from the previous build; confirm vehicles can enter and turn through the central aisle.
5. Count twenty native parking positions: eight covered inside each garage wing and four open-air at the U opening. Confirm none are on the main-building roof, and every position faces inward toward the yard. Try representative long excavators and bulldozers in all positions; inspect clipping, spacing, and pathing.
   Confirm the game renders the ground bay lines for all twenty positions. Their `$VEHICLE_PARKING` segments now span 10 m while retaining their previous centers and inward-facing direction.
6. Confirm both loading/unloading positions sit side by side near the court center, close to the main-building frontage, and do not overlap parking paths, stairs, buildings, or the road approach.
7. Confirm the native panel shows twenty vehicle positions, admits excavators and bulldozers, rejects other vehicle classes, and exposes no worker or fuel requirement.
8. Save, reload, and confirm the placed building retains its vehicle state. Demolish it and check that surface-area metadata is removed with the building.

Do not treat this prototype as balance-complete. Review the footprint, pathing, station placement and construction phases first; costs and detailed machine capacity remain tunable.
