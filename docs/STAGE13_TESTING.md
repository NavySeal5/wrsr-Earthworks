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
3. Discard the draft and create another Site. Choose **Square**, click one corner, and move the cursor. Confirm a closed square wireframe follows the cursor and stays aligned to the map axes. The longer cursor offset should define both equal side lengths. Click to commit it.
4. Confirm the square opens the ordinary point-height editor with four selectable corners. Set different heights on two corners and confirm the wireframe shows the slope. Save the Site.
5. Repeat with **Rectangle**. Its live wireframe should follow the first corner and cursor as two opposite, map-aligned corners. Click to commit, set different point heights, and save.
6. Try committing a shape with either side shorter than 5 m; it should be rejected without losing the first corner. Move the cursor farther away and click again.
7. Before setting the second corner, switch between Square and Rectangle and confirm the live preview changes while keeping the original first corner. Once a Free draw has more than one point, or a shape is committed, the mode controls should be disabled.

## Edit a saved Site

1. Reopen the saved Site. Click an unselected vertex and release within 250 ms; it should only select the point and must not move it. Then press and hold an unselected vertex for at least 250 ms before dragging; it should move during that same press. Already-selected points should retain immediate drag behavior. The boundary should update, while the Site should not change the terrain.
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

Report any crash, unexpected terrain change during planning/editing, boundary/height mismatch, incorrect save/reload, invalid edit acceptance, or failure to pause/review. Stage 13 remains pending until the in-game result is reviewed.
