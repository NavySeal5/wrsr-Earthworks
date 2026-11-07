# Stage 13 — polygon Sites and per-point heights

This build replaces rectangular flat Sites with editable polygon Sites. Use a fresh disposable save or a clear area in a copy of your test save. The agent has not performed in-game tests; these checks are for the user to run.

## Startup and draw/save a polygon

1. Start the project-local RML development profile with `launch-prototype.cmd`. First confirm the game reaches the main menu without closing. Then load a test world and open Earthworks.
2. Select a Project and choose **New Site**. Click four points around a clear area, keeping each point at least 5 m from every other point. Choose **Finish shape**. The boundary should close and the editor should show the polygon footprint and cut/fill estimates; terrain must remain unchanged.
3. Discard that draft, start another Site, and click six points to make an L-shaped/concave footprint. Finish it and confirm the wireframe follows the six-sided boundary instead of filling its bounding rectangle.
4. Select each vertex in the editor. Set at least three different heights using the numeric field, then use **Sample point** and click a map location. The wireframe should interpolate the point heights into sloped triangular surfaces.
5. Save the Site. Confirm it appears in the selected Project and that its name/status and polygon preview remain available.

## Shape modes

1. Choose **New Site** and confirm the editor now offers **Free draw**, **Square**, and **Rectangle** before any map point is placed.
2. In **Free draw**, click three or more vertices, confirm the outline follows the clicks, then use **Finish shape**. This should behave as it did before the shape-mode change.
3. Discard the draft and create another Site. Choose **Square**, click one corner, and move the cursor. Confirm a closed square wireframe follows the cursor and stays aligned to the map axes. The longer cursor offset should define both equal side lengths. Click to commit it.
4. Confirm the square opens the ordinary point-height editor with four selectable corners. Set different heights on two corners and confirm the wireframe shows the slope. Save the Site.
5. Repeat with **Rectangle**. Its live wireframe should follow the first corner and cursor as two opposite, map-aligned corners. Click to commit, set different point heights, and save.
6. Try committing a shape with either side shorter than 5 m; it should be rejected without losing the first corner. Move the cursor farther away and click again.
7. Before setting the second corner, switch between Square and Rectangle and confirm the live preview changes while keeping the original first corner. Once a Free draw has more than one point, or a shape is committed, the mode controls should be disabled.

## Edit a saved Site

1. Reopen the saved Site. Drag one vertex to a nearby position and release. The boundary should update, while the Site should not change the terrain.
2. Select an edge and use **Add point on next edge**. Confirm the new point appears on that edge, can be selected, and can receive an independent height.
3. Remove a selected point. Confirm the polygon remains valid and has at least three points.
4. Try to move vertices into a self-crossing shape, within 5 m of another point, or into an overlap with a queued/running/paused Site. Invalid edits should be rejected and the prior valid shape should remain.
5. Confirm the editor reports a 32-point limit. Test exceeding it only if convenient; do not spend time placing 32 points if the counter and disabled control already make the limit clear.

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
