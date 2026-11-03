# Stage 11 — first complete UI implementation (formerly Stage 10b)

Build: **0.13.1-stage11-ui-polish**. Use a fresh test save; previous WIP companion data is not migrated. The agent compiled and staged this build but did not launch the game or perform gameplay checks.

## Project and Site workflow

1. Start the project-local RML profile with `launch-prototype.cmd`, load a fresh test world and open Earthworks. Confirm there is no Plan tab.
2. Create two named Projects, switch between them, and verify the Sites list filters to the selected Project. Rename each Project and a Site.
3. Select a Project and click **New Site** below the list. Click two opposite terrain corners. The selection should finish with no terrain edit and open a separate Site editor beside the footprint.
4. Check the editor's footprint dimensions, sampled ground mean, cut/fill estimates and target height. Enter a height and apply it; use **Sample target from map** to sample another point. The preview should update while the terrain stays unchanged.
5. Click **Save Site**. Confirm the Site enters the selected Project with a sequential `Site-N` name. Save Site is only planning; terrain work requires a row's explicit Start action.
6. Close the Site editor and click the Site footprint to reopen it. Also close the main Earthworks panel and click the footprint; the panel and editor should reopen. Check that the editor follows the Site as you pan/zoom and stays on-screen near map edges.
7. Drag the editor by its header. Confirm the text is readable against the parchment, the red header band fits the parchment header, the editor keeps following the Site when the camera moves, and the in-game cursor remains visible over the editor.
8. Cancel area selection with Escape. Confirm the next Escape closes the Earthworks panel without opening the game's escape menu. Normal map clicks and WASD should pass through when no selection or sampling mode is armed.

## Target editing and saved state

1. Change a queued Site's target and confirm it stays queued and terrain remains unchanged.
2. Start a Site, then try changing its target while Running/Waiting. Editing should be disabled with a pause-first message. Pause it, change the target, and confirm it requires terrain review before resuming.
3. Save and reload. Confirm Projects, names, Site states/targets, priorities, facility assignments and selected Project persist. The editor itself should reopen when you click a Site footprint.
4. Confirm a Project with unfinished Sites cannot be deleted. Cancel or complete its Sites and check that deletion removes the ended records. Bulk completed cleanup should affect only the selected Project.

## Facility assignment regression

Use the Projects view's Facilities page. Assign two Earthworks facilities to one Project; confirm both contribute capacity among that Project's Sites by priority. Then switch Projects and verify those facilities cannot be assigned a second time. Distance remains informational. Ordinary construction offices should remain unaffected.

Tell me which checks you completed and any unexpected panel behavior, input capture, save/reload result or terrain change. The complete Stage 11 implementation and Stage 10a review remain pending your in-game confirmation.
