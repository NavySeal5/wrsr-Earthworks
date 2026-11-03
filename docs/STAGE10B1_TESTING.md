# Stage 10b.1 — named Project and Earthwork storage

Build: **0.12.0-stage10b1-named-storage**. The agent compiled and staged the plugin but did not launch the game or test a save. Use a new test save: the previous WIP companion schema is intentionally not migrated.

## Setup

1. Close the game and the project-local RML window.
2. Run `scripts/Build.ps1`, then `scripts/Stage-Diagnostics.ps1` from the mod folder.
3. Run `launch-prototype.cmd`, select **Earthworks Development**, and launch the game.
4. Start a fresh world and save it under a new name before creating Earthworks.

## Checkpoint

1. Open Earthworks and switch to **Projects**. Confirm the selected container is **Project 1**.
2. Create and confirm two non-overlapping Earthworks. Confirm the list labels them **Site-1** and **Site-2** and shows their queued status.
3. Assign the Earthworks facility to one queued site. Confirm the assignment is shown for Earthworks in that same container. Start a disposable site if you want to include a running status and progress in the save check.
4. Save normally, exit to the menu, and reload the same save. Confirm the site names, plans, statuses/progress and facility assignment are restored, and the terrain is unchanged by planning or loading.
5. Confirm the project container still reads **Project 1**. Once this checkpoint is accepted, add another site and confirm the next default is **Site-3**.
6. Confirm the save companion is associated only with this new save. Do not use a prior WIP save for acceptance; prior schema files are not converted.

## Expected results

- A fresh world has one named container, **Project 1**.
- Confirmed sites receive sequential names (`Site-1`, `Site-2`, …); name editing arrives in checkpoint 10b.2.
- The selected named Project ID, site records, project-level facility links and work state are stored in schema 5.
- Save/reload preserves plans and work state without changing terrain.
- Prior schema data is not migrated. Use a fresh save for this checkpoint.

Tell me which steps you completed, whether names/status/progress/facility assignment survived reload, and whether any terrain changed unexpectedly. The selected-container persistence choice is recorded; it can be directly tested after checkpoint 10b.2 adds multiple named Projects.
