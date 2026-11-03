# Stage 11 — current UI acceptance checklist (formerly Stage 10b)

Use a fresh test save; previous WIP companion data is not migrated. This checklist includes follow-up UI changes after the initial 0.13.13 implementation. The agent has compiled the changes but has not performed the in-game checks below.

## Project and Site workflow

1. Start the project-local RML profile with `launch-prototype.cmd`, load a fresh test world and open Earthworks. Confirm there is no Plan tab.
2. Create two named Projects, switch between them, and verify the Sites list filters to the selected Project. Rename each Project and use a Site row's R/name icon to open the Site rename dialog, then save the new name.
2a. Start a New Site draft, open New Project while the draft editor remains open, create a Project and confirm it becomes selected. Finish and save the draft; confirm the Site appears under that new Project.
3. Confirm the New, Rename, Delete, Facilities and Remove completed controls appear on their own row below the Project selector, and that the panel is narrower than the previous build.
4. Open the New Project dialog and confirm the game cursor stays drawn above the dialog, including when it extends outside the main panel.
5. Select a Project and click **New Site** below the list. Click two opposite terrain corners. The selection should finish without a crash or terrain edit and open a separate Site editor beside the footprint.
6. Check the editor's footprint dimensions, sampled ground mean, cut/fill estimates and target height. Enter a height and apply it; check the −/+ controls apply immediately (1 m normally, 10 m with Shift, 100 m with Ctrl). Hover the target-step and Sample target from map info icons to read their explanations. Use **Sample target from map**, then click a map point; the preview should update while the terrain stays unchanged.
7. Start, pause, resume/review, and cancel Sites using row buttons. Confirm these actions do not open the Site editor; clicking the Site name should still open it.
8. Click **Save Site**. Confirm the Site enters the selected Project with a sequential `Site-N` name. Save Site is only planning; terrain work requires a row's explicit Start action.
9. Create and save a second unfinished Site. Start creating another Site with an overlapping footprint. Confirm the editor shows a readable overlap error naming the existing Site and disables **Save Site**. Move the draft to a clear area and confirm the error clears and saving becomes available. Finished/cancelled Sites should not block an area.
10. Close the Site editor and click the Site footprint to reopen it. Also close the main Earthworks panel and click the footprint; the panel and editor should reopen. The editor should initially appear beside the Site, then stay fixed in screen coordinates as you pan or zoom; check screen edges.
11. Drag the editor by its header. Confirm the text is readable against the parchment, the red header band fits the parchment header, the editor stays at its manually chosen screen position while the camera moves, and the in-game cursor remains visible over the editor. A manually moved editor remains open until explicitly closed.
12. Confirm the Sites table is narrower, the progress bar fills its entire column, the Cancel and Rename headers are empty, hovering either empty header shows its full label, the row controls use compact game icons without an extra button frame/effect, the View column is gone, and the remaining controls stay on one line. If convenient, check at a narrower display resolution too.
13. Cancel area selection with Escape. Confirm the next Escape closes the Earthworks panel without opening the game's escape menu. Normal map clicks and WASD should pass through when no selection or sampling mode is armed.
14. While renaming a Site or Project, type digits that are assigned to native building/tool shortcuts. The text should receive the digits without activating those shortcuts. Escape and other exempt system keys should retain their expected behavior.

## Target editing and saved state

1. Change a queued Site's target and confirm it stays queued and terrain remains unchanged. Confirm typed height changes still use **Apply height**, while −/+ clicks apply immediately.
2. Start a Site and focus its target-height field while Running/Waiting. Confirm this automatically pauses the Site. Change the target and confirm terrain review is required before resuming. Also begin map sampling on a running Site; confirm it pauses, then click a point and verify review is required before resuming.
3. Save and reload. Confirm Projects, names, Site states/targets, priorities, facility assignments and selected Project persist. The editor itself should reopen when you click a Site footprint.
4. Confirm a Project with unfinished Sites cannot be deleted. Cancel or complete its Sites and check that deletion removes the ended records. Bulk completed cleanup should affect only the selected Project.

## Facility assignment regression

Use the Projects view's Facilities page. Assign two Earthworks facilities to one Project; confirm both contribute capacity among that Project's Sites by priority. Then switch Projects and verify those facilities cannot be assigned a second time. Distance remains informational. Ordinary construction offices should remain unaffected.

Stage 11 and the player-facing UI presentation are accepted as functionally complete. Use these checks when regression testing follow-up changes; the latest Project-during-draft and auto-pause behaviors are especially useful to verify. Report any unexpected panel behavior, input capture, save/reload result or terrain change.
