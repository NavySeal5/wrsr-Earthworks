# Stage 10a — player UI review

Build: **0.11.0-stage10a-player-ui**. This is a user-run in-game review. The agent compiled and staged the player build but did not launch the game or inspect a save.

## Setup

1. Use a disposable test save or a copy of the Stage 9 fixture.
2. Close the game and project-local RML before restaging.
3. Run `scripts/Build.ps1`, then `scripts/Stage-Diagnostics.ps1` from the mod folder. The stage script prepares the isolated RML profile and does not launch the game.
4. Run `launch-prototype.cmd`, confirm the RML profile is **Earthworks Development**, then launch the game from RML and load the test save.

## Plan and Projects tabs

1. Open Earthworks. Confirm the window has **Plan** and **Projects** tabs and no workforce inspector, raw terrain readings, or terrain-probe controls.
2. In **Plan**, select a rectangle with two opposite-corner clicks while leaving the panel open. Set or sample a target height and check that footprint, cut and fill estimates appear.
3. Add the plan to the project queue. Switch to **Projects** and confirm the new project appears.
4. Choose a facility on the map. The map click should identify nearby buildings, then the Projects tab should offer recognized Earthworks facilities by distance. Select the intended facility and assign it.
5. Select different projects and check that status, progress, priority controls, and facility assignment follow the selected project.
6. Review available queued, running, waiting, paused, review-required, completed and cancelled displays. Start or pause work only in a disposable area if you want to check those states.
7. Check error/status messages, long text, window scrolling/resizing, screen edges, and your usual UI scale.
8. Keep the panel open for terrain selection; close it before using native game controls. Confirm normal game clicks work again after closing it.
9. Save and reload the test save if desired, and confirm the Plan/Projects controls remain consistent with the stored projects.

## Expected results

- Plan creation and project management are on separate tabs.
- Map selection remains available while the Earthworks window is open.
- Facility picking/assignment is handled in the Projects tab without raw diagnostic data.
- Developer-only inspector and terrain-probe UI are absent from this player build.
- Existing project behavior and terrain are unchanged by opening or navigating the UI.

## Report back

Tell me the resolution/UI scale used, whether the two tabs make the workflow easy to follow, whether map-based facility selection worked, and any clipped text or confusing action/status labels. Include which project states you viewed and whether you chose to start work.
