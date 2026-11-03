# Stage 11 — first complete UI implementation (formerly Stage 10b)

Build: **0.13.9-stage11-site-rename-fix**. Use a fresh test save; previous WIP companion data is not migrated. The agent compiled and staged this build but did not launch the game or perform gameplay checks.

## Project and Site workflow

1. Start the project-local RML profile with `launch-prototype.cmd`, load a fresh test world and open Earthworks. Confirm there is no Plan tab.
2. Create two named Projects, switch between them, and verify the Sites list filters to the selected Project. Rename each Project and use a Site row's R/name icon to open the Site rename dialog, then save the new name.
3. Confirm the New, Rename, Delete, Facilities and Remove completed controls appear on their own row below the Project selector, and that the panel is narrower than the previous build.
4. Open the New Project dialog and confirm the game cursor stays drawn above the dialog, including when it extends outside the main panel.
5. Select a Project and click **New Site** below the list. Click two opposite terrain corners. The selection should finish without a crash or terrain edit and open a separate Site editor beside the footprint.
6. Check the editor's footprint dimensions, sampled ground mean, cut/fill estimates and target height. Enter a height and apply it; use **Sample target from map** to sample another point. The preview should update while the terrain stays unchanged.
7. Click **Save Site**. Confirm the Site enters the selected Project with a sequential `Site-N` name. Save Site is only planning; terrain work requires a row's explicit Start action.
8. Create and save a second unfinished Site. Start creating another Site with an overlapping footprint. Confirm the editor shows a readable overlap error naming the existing Site and disables **Save Site**. Move the draft to a clear area and confirm the error clears and saving becomes available. Finished/cancelled Sites should not block an area.
9. Close the Site editor and click the Site footprint to reopen it. Also close the main Earthworks panel and click the footprint; the panel and editor should reopen. Check that the editor follows the Site as you pan/zoom and stays on-screen near map edges.
10. Drag the editor by its header. Confirm the text is readable against the parchment, the red header band fits the parchment header, the editor keeps following the Site when the camera moves, and the in-game cursor remains visible over the editor.
11. Confirm the Sites table is narrower, Cancel and Rename use the game's icons without an extra button frame/effect, the icons fit the row, the View column is gone, and the remaining controls stay on one line. If convenient, check at a narrower display resolution too.
12. Cancel area selection with Escape. Confirm the next Escape closes the Earthworks panel without opening the game's escape menu. Normal map clicks and WASD should pass through when no selection or sampling mode is armed.

## Target editing and saved state

1. Change a queued Site's target and confirm it stays queued and terrain remains unchanged.
2. Start a Site, then try changing its target while Running/Waiting. Editing should be disabled with a pause-first message. Pause it, change the target, and confirm it requires terrain review before resuming.
3. Save and reload. Confirm Projects, names, Site states/targets, priorities, facility assignments and selected Project persist. The editor itself should reopen when you click a Site footprint.
4. Confirm a Project with unfinished Sites cannot be deleted. Cancel or complete its Sites and check that deletion removes the ended records. Bulk completed cleanup should affect only the selected Project.

## Facility assignment regression

Use the Projects view's Facilities page. Assign two Earthworks facilities to one Project; confirm both contribute capacity among that Project's Sites by priority. Then switch Projects and verify those facilities cannot be assigned a second time. Distance remains informational. Ordinary construction offices should remain unaffected.

Tell me which checks you completed and any unexpected panel behavior, input capture, save/reload result or terrain change. The complete Stage 11 implementation and Stage 10a review remain pending your in-game confirmation.
