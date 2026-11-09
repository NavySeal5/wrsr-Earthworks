# Stage 13 — polygon Sites and per-point heights

This build replaces rectangular flat Sites with editable polygon Sites. Use a fresh disposable save or a clear area in a copy of your test save. The agent has not performed in-game tests; these checks are for the user to run.

## Startup and draw/save a polygon

1. Start the project-local RML development profile with `launch-prototype.cmd`. First confirm the game reaches the main menu without closing. Then load a test world and open Earthworks.
2. Select a Project and choose **New Site**. Click at least three points around a clear area, keeping each point at least 5 m from every other point. Click within 5 m of the first point. The polygon should finish without adding a duplicate point, and the editor should show its footprint and cut/fill estimates; terrain must remain unchanged. Also try a click more than 5 m from the first point and confirm it adds another vertex. Discard the draft afterward.
3. Discard that draft, start another Site, and click six points to make an L-shaped/concave footprint. Finish it and confirm the wireframe follows the six-sided boundary instead of filling its bounding rectangle.
4. Select each vertex in the editor. Set at least three different heights using the numeric field, then use **Sample selected** and click a map location. The wireframe should interpolate the point heights into sloped triangular surfaces.
5. Select one or more shape points. Each selected point should show a clearly visible, colored line from its planned height down to the sampled current ground, with a marker at the ground endpoint. Unselected points should not show these height lines. Cut and fill points should use their corresponding preview colors.
6. Shift-click two or more point handles. Confirm they are visibly selected and show ground lines. Shift-click an already selected handle and confirm it becomes deselected and its ground line disappears. If no points remain selected, the height controls should be disabled. Enter a height or sample from the map and verify all selected points take that height. Click −/+ normally and verify each selected point changes by 1 m while preserving differences; hold Ctrl and verify 10 m steps. Click a point without Shift and confirm selection returns to that point only. Shift should not change height-step size.
7. Save the Site. Confirm it appears in the selected Project and that its name/status and polygon preview remain available.

## Shape modes

1. Choose **New Site** and confirm the editor now offers **Free draw**, **Square**, and **Rectangle** before any map point is placed.
2. In **Free draw**, click three or more vertices and confirm the outline follows the clicks. Click within 5 m of the first vertex and confirm the shape closes without adding another point. Also click directly on the first point handle and confirm it closes the shape rather than only selecting the point. Repeat once using **Finish shape** to confirm the explicit control still works.
3. While Free draw is active, click near (but more than 5 m from) an existing draft vertex, including where its enlarged handle would normally be clickable. Confirm the click appends a point at the live teal endpoint; with **Lock to Grid** enabled, it should land on the nearest grid vertex shown by that endpoint. Nearby draft handles must not intercept the click.
4. Continue placing vertices at a normal pace. The game cursor sprite was being mistaken for an occluding UI panel; build 0.16.23 ignores only its matching 32×32 rectangle. Click three map locations, including one that previously failed, then close or discard the draft. Confirm ordinary clicks add points and clicks on actual game UI remain blocked. If either behavior is wrong, share the matching `earthworks draw-click route` line, including `cursorRectIgnored`, `nativeUi`, `uiHit`, and `uiBounds`.
5. Discard the draft and create another Site. Choose **Square**, click one corner, and move the cursor. Confirm a closed square wireframe follows the cursor and stays aligned to the map axes. The longer cursor offset should define both equal side lengths. Click to commit it.
6. Confirm the square opens the ordinary point-height editor with four selectable corners. Set different heights on two corners and confirm the wireframe shows the slope. Save the Site.
7. Repeat with **Rectangle**. Its live wireframe should follow the first corner and cursor as two opposite, map-aligned corners. Click to commit, set different point heights, and save.
8. Select one vertex, then hold Shift while clicking either height adjustment button. Confirm all vertices change by the same relative amount and retain their height differences. Hold Ctrl at the same time to confirm the step is 10 m. Without Shift, confirm only the selected vertices change. Check the height info button for the updated modifier explanation.
8. Try committing a shape with either side shorter than 5 m; it should be rejected without losing the first corner. Move the cursor farther away and click again.
9. Before setting the second corner, switch between Square and Rectangle and confirm the live preview changes while keeping the original first corner. Once a Free draw has more than one point, or a shape is committed, the mode controls should be disabled.

## Cut/fill estimate — build 0.16.29

The estimate now uses four interior samples per planning-grid cell and counts only samples inside the Site footprint. This is intended to keep boundary and irregular polygons from losing entire cells when a corner falls outside the shape. Values display to one decimal place. Estimates remain coarse previews; actual execution uses terrain-grid measurements.

1. Draw and finish a rectangle on a clear area. Raise all points by several metres; Fill estimate should become positive. Lower them below the sampled ground; Cut estimate should become positive.
2. Draw a concave or L-shaped Site and set different point heights so part of the surface is above ground and part below. Confirm both estimates can be positive and respond when a point height changes.
3. Set all points to the sampled current ground height. Estimates should return close to zero. A displayed `0.0 m3` is expected only when the measured value is effectively zero at the preview's sampling resolution.

## Optional terrain-grid snapping

1. Start a new Site and confirm **Lock to Grid** is unchecked by default. With it unchecked, place Free draw points and confirm they follow the cursor without snapping.
2. Start another new Site, check **Lock to Grid** before placing any points, and click anywhere on the map, including between visible grid vertices. In Free draw, the preview segment should end at the nearest native terrain grid vertex; the click should place the point there without requiring a precise click on the vertex.
3. Repeat in Square and Rectangle modes. As the cursor moves, the live wireframe should follow the nearest grid vertex, and each click anywhere on the map should commit the corner at that snapped location. Check both axes on terrain where X and Z grid spacing differ, if available.
4. After the first point is placed, confirm the checkbox is disabled so the footprint cannot mix snapped and unsnapped points. Create another Site to switch the option back off or on.
5. Confirm the checkbox only changes horizontal point positions. Point target heights should still be sampled from the terrain at the snapped location, and planning must not modify terrain.

## Edit a saved Site

1. Reopen the saved Site. Click near (within about 20 screen pixels of) an unselected vertex; it should select the nearest vertex without needing to hit its center. If two point hit areas overlap, the closest visible vertex should be selected. Click an unselected vertex and release within 250 ms; it should only select the point and must not move it. Then press and hold an unselected vertex for at least 250 ms before dragging; it should move during that same press. Already-selected points should retain immediate drag behavior. The boundary should update, while the Site should not change the terrain.
2. During a drag, keep the mouse button held and move the point across a few metres. Confirm the wireframe boundary/grid and the point handle follow the cursor continuously, before release. Release to commit the edit; the preview should remain at the accepted location. Then click elsewhere on the map and release; the point must remain in place. If validation rejects the final shape, the point should return to its prior saved location and terrain should remain unchanged.
3. Select an edge and use **Add point on next edge**. Confirm the new point appears on that edge, can be selected, and can receive an independent height.
4. Hover over a different edge. A grey, plus-marked point preview should appear on the edge. Click the edge at the preview and confirm the new point is inserted at that position and selected. Try near both ends; positions less than 5 m from an endpoint should not offer insertion.
5. Remove a selected point. Confirm the polygon remains valid and has at least three points.
6. Try to move vertices into a self-crossing shape, within 5 m of another point, or into an overlap with a queued/running/paused Site. Invalid edits should be rejected and the prior valid shape should remain.
7. Confirm the editor reports a 32-point limit. Test exceeding it only if convenient; do not spend time placing 32 points if the counter and disabled control already make the limit clear.

## Save/reload and work rules

1. Save the republic and reload it. Confirm the Site polygon, all point heights, and the selected Project persist.
2. Assign a facility and start a small Site only when ready to change terrain. Watch the progress bar over multiple simulation-time intervals; it should advance in fractional steps rather than stay at 0% until completion. Save and reload a partly completed Site and confirm progress continues from its previous value.
3. While a Site is Running, edit a vertex or height. It should pause automatically and require terrain review before resuming. Make one safe edit to a Paused Site and confirm review is still required. A Queued Site should remain editable without a pause.
4. Confirm removing a point below three is refused. Confirm Finish shape and Save Site remain unavailable until the shape is valid and complete.

## Boundary blend for ramps

This build blends the target back to the Site's sampled starting terrain at its boundary, reaching the requested interpolated target one native terrain cell inward. Use a fresh, clear test area; do not reuse a Site where an earlier run may already have changed the terrain.

1. Draw a Rectangle with one long edge higher than the opposite edge, creating a ramp. Keep the Site away from roads, buildings, and other objects.
2. Before starting, inspect the colored preview. The cut/fill shading and wireframe should taper at all boundaries rather than show a full-height jump right at the outline.
3. Start the Site and watch the two long edges as work advances. They should meet the unchanged surrounding ground without forming the paired dents/berms seen on the earlier ramp. The requested slope should remain visible across the interior.
4. Let the Site complete, then inspect all four edges from a low camera angle. The terrain outside the Site should remain unchanged, and the blend band should be confined to approximately one terrain cell inside the boundary.
5. Save and reload a partly completed Site. Confirm that its original sampled boundary heights are retained and the transition continues consistently.

Report edge dents/berms, an unexpectedly wide blend, a visible flat lip, or any mismatch between the preview and the finished terrain. The blend is provisional pending this in-game check.

## Native UI click protection in draw modes

1. Open a fresh test save and arm each creation mode in turn: Free draw, Square, and Rectangle. Move the cursor over native UI areas such as the top date/speed bar, left tool strip, bottom build toolbar, and an open vanilla building panel.
2. Click native buttons and controls while each draw mode is armed. Confirm vanilla UI controls respond normally and no Site point is added, no shape is committed, and no existing vertex is selected, inserted, or moved behind the UI.
3. Return to unobstructed terrain and confirm each draw mode still accepts its intended map clicks and shows its preview.
4. Open the editor for a saved Site. Place a native panel over some of its wireframe/point handles if convenient. Click and drag over that vanilla panel; confirm the Site editor does not select, insert, or move points through it, while the vanilla panel still works.
5. Check the RML runtime log after startup for `native UI click occlusion installed`. If it reports `unavailable`, repeat the checks around the standard HUD margins; report the log line and any UI regions that still accept a Site click.

Report any crash, unexpected terrain change during planning/editing, boundary/height mismatch, incorrect save/reload, invalid edit acceptance, or failure to pause/review. Stage 13 remains pending until the in-game result is reviewed.

## Safe angles selector — build 0.16.28-road-safe-grade-16

The Site editor has one **Safe angles** selector and one adjacent (i) button. It defaults to **None**. The options are:

- **None:** no additional slope check.
- **Roads:** prevent saving when any triangulated planar facet exceeds 16% grade, about 9.09°.
- **Vanilla:** prevent saving when any facet exceeds 37°, about 75.36% grade; this is the selected terrain-safe reference, not a verified engine constant.

Grade is rise divided by horizontal run. Convert angle in degrees to percent grade with `100 × tan(angle)`, or percent grade to angle with `atan(percent / 100)`. The checks inspect every triangulated planar facet, including diagonal/cross slopes, not only polygon edges. Confirm that selecting each mode changes the validation behavior and that the mode survives save/reload.

## Live cut/fill estimate — build 0.16.32-live-site-cut-fill

1. Open the editor for a saved Site and note its Cut and Fill estimates.
2. Start the Site and leave the editor open while simulation runs. Confirm the estimates update at short intervals as terrain is changed. They represent the approximate remaining cut and fill needed to reach the saved target; completed work should reduce the relevant amount.
3. Pause the Site and confirm the values stop changing. Resume and confirm they continue updating. On completion, estimates should be near zero, subject to terrain-grid sampling tolerance.
4. For comparison, open an unstarted Site or create a new draft. Its estimate should still describe the planned difference from the sampled starting terrain.

The live estimate is sampled from current terrain every 500 ms while a saved Site is open in the editor. If it remains unchanged during visible terrain work, note the Site status and share the RML runtime log around the run.

## Progress recalculation after editing a running Site — build 0.16.33-review-progress-rebase

1. Start a Site and let it reach a visible partial progress value, then pause it and open its editor.
2. Change its target or shape so that more terrain work is required. Review and resume it. Progress should rebase downward to reflect the previously completed work plus the revised remaining work, then continue increasing.
3. Pause it again, edit the target in a way that reduces the remaining work, and review/resume. Progress should rebase upward while preserving the completed-work credit.
4. If the edited target already matches current terrain when reviewed, the Site should immediately show Completed rather than remain Running with no work to do.
5. Confirm progress keeps advancing and reaches completion against the revised target. Saving and reloading after review should preserve the recalculated percentage and continue normally.

The denominator is an estimate derived from terrain height differences on the native terrain grid. Edits can change estimated work nonlinearly, so percentages are expected to rebase rather than remain fixed when scope changes.

## Point-to-ground guides — build 0.16.34-site-point-ground-guides

1. Open a saved Site in the editor with multiple points at different target heights. Confirm each point that differs from its sampled terrain height has a visible line to the ground, including unselected points.
2. Create a draft and add several points. Confirm guides appear for its points before the shape is finished as well as after completion.
3. Select and deselect points, including Shift multi-selection. Guides should stay visible through selection changes.
4. Place native game panels over the guide lines. The guides should remain visible in uncovered map space and remain behind native UI elements.

## Live cut/fill and completion alignment — build 0.16.35-live-volume-completion-alignment

1. Start a Site with a clear cut-dominant target and keep its editor open. Watch cut and fill while work advances. They represent remaining actionable work sampled on native terrain vertices; either direction can change if the terrain crosses its target locally.
2. When the Site reaches Completed, both cut and fill estimates should be zero within the executor's completion tolerance.
3. Repeat with a fill-dominant Site if convenient. Compare the displayed residual with the Completed status and report any nonzero estimate after completion, including the Site status and runtime log.

## Assigned capacity and duration estimate — build 0.16.37-calendar-time-estimate-fix

1. Assign one Earthworks facility to the selected Project and open a Site editor. Confirm **Assigned capacity** reports an approximate m3/day value based on its eligible machines. With no assigned facility, it should report zero; if a reading fails, it should report unavailable rather than treating the failure as zero.
2. Add/remove a facility or buy/move an eligible machine. After the facility reading refresh, confirm the capacity changes accordingly.
3. For a queued, paused, or draft Site, confirm the remaining-time estimate assumes full assigned capacity. With a clear footprint, compare the estimate directionally against its live Cut + Fill values.
4. Start a Site and confirm its estimate uses its currently allocated machine count/rate. If several Sites share facilities, their current allocations can make the running Site's forecast differ from total Project capacity.
5. Pause simulation time and confirm the capacity estimate is stable; progress/time remaining should not count wall-clock time. Resume and observe the forecast as terrain and machine allocation change.
6. A calendar day is 60 simulation hours in this game. At the current provisional rate, one reference-rate machine therefore provides about 600 m3 per calendar day (10 m3/hour × 60 hours). Treat this as provisional, not calibrated balance. Report missing counts, a zero rate despite eligible machines, or a duration that does not update when capacity or remaining volume changes.

## Unresolved facility assignments after loading — build 0.16.41-facility-assignment-warning-ok-cleanup

1. Load a save with at least one facility assignment that no longer matches a facility in that republic. After the assignment scan completes, confirm a popup says: **Warning: Error loading Earthworks assignments, please re-check your assigned facilities to projects**.
2. Click the X. Confirm Projects, Sites, and unresolved assignment data remain unchanged. The popup should stay hidden for that session; loading the unresolved save again should show it again.
3. Choose **OK**. Confirm the affected facility links disappear, while their Projects and Sites remain. Valid assignments should remain.
4. Save the republic and reload it. Confirm the warning no longer appears for the removed links, and confirm valid facility assignments remain attached to their Projects.
5. Load a save whose facility assignments all resolve. Confirm no warning appears.
6. If the companion-file update fails, confirm the warning stays open and no saved assignments are removed. Share the runtime log if this occurs.

## Winter wireframe colors — build 0.16.42-winter-wireframe-mode

1. Open **Earthworks** and select **Projects and Sites**. Confirm **Winter Mode** is off by default.
2. Create or open a Site. Turn on **Winter Mode**. Confirm the wireframe, cut/fill colors, point markers, and queued Site outlines use darker colors that remain visible against snow.
3. Turn **Winter Mode** off. Confirm the normal colors return.
4. Repeat with a draft, a queued Site, and an active Site if available. Confirm the toggle changes display colors only. It must not change Site geometry, terrain, progress, or status.

## Persistent Site editor validation — build 0.16.43-site-editor-validation

1. Open a queued Site in the Site editor without assigning an Earthworks facility. Confirm a red explanation wraps inside the editor and tells you to assign a facility.
2. Assign a facility, then start the Site from its table row. If start validation fails, open that Site. Confirm the red failure explanation is visible in its editor, even though the table action did not open the editor.
3. If a running Site encounters a terrain or facility read error, open its editor. Confirm the affected Site's error appears there and remains visible while the editor stays open.
4. Correct the issue or retry successfully. Confirm the message clears after the successful action. Reopening an unchanged draft must not clear its error.
5. Trigger a draft validation error, such as an invalid or overlapping shape. Confirm its message wraps inside the draft editor and remains visible until the draft is corrected or discarded.
6. Start, pause, or cancel a Site from the list. Confirm those controls do not open the Site editor automatically. Open the affected Site to read any stored error.
