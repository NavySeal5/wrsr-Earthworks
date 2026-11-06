# Stage 12 — Medium facility review

## Finish the ModelViewer export

The Medium OBJ and its material files are prepared inside the game media directory so ModelViewer will not report that the file is outside `GAME_DIR/media_soviet`.

1. Close the game and the project-local RML launcher.
2. Open ModelViewer and import:
   `media_soviet/workshop_wip/9100000013/EarthworksDevelopmentFacilityMedium/earthworks_facility_medium.obj`
3. Set **Object type** to **Building**.
4. Save the NMF into that same object folder with the exact filename `model.nmf`.
5. Copy `model.nmf` to `assets/building/medium/native/earthworks_facility_medium.nmf` in the project. The normal prototype launcher will then stage both Small and Medium items.

Do not save under the OBJ's long name or add a duplicate extension. The staging script expects the exact `model.nmf` basename in the Workshop object folder and the variant-specific native source filename above.

## User-run in-game checks

After the NMF is copied, run `launch-prototype.cmd` and use a fresh test save. Do not load a save containing a previous version of item `9100000013` for this first check.

1. Find **Earthworks Facility (Medium)** in the construction menu and confirm its preview and material costs appear. Place it, finish construction, and inspect its scale and visible surfaces.
2. Confirm there are four canopy bays with two vehicle parking positions per bay (eight total). Check that long excavators/bulldozers fit and can enter and leave without pillar or canopy collisions.
3. Confirm there is exactly one loading/unloading station in front of the office and that the Road Depot UI displays it outside the garages.
4. Confirm the road connection is at the far-right end of the canopy, points outward to the right, and joins the forecourt without a gap or overlap with the structure.
5. Confirm only excavators and bulldozers can be purchased into the facility; other vehicle purchases should be refused. Confirm the Earthworks UI recognizes the Medium facility and can assign it to a Project.
6. Save/reload once and confirm the building, parking layout, facility recognition and project assignment remain intact.

Report any slot count, pathing, road-facing, station placement, cost or recognition mismatch before accepting Medium. This stage has not been accepted in-game yet; no agent gameplay check was run.
