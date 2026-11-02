# Stage 10a — player UI review

Build: **0.11.7-stage10a-game-ui**. This is a user-run in-game review. The agent compiled and staged the player build but did not launch the game or inspect a save.

## Setup

1. Use a disposable test save or a copy of the Stage 9 fixture.
2. Close the game and project-local RML before restaging.
3. Run `scripts/Build.ps1`, then `scripts/Stage-Diagnostics.ps1` from the mod folder. The stage script prepares the isolated RML profile and does not launch the game.
4. Run `launch-prototype.cmd`, confirm the RML profile is **Earthworks Development**, then launch the game from RML and load the test save.

## Plan and Projects tabs

1. Open Earthworks. Confirm the window has **Plan** and **Projects** tabs and no workforce inspector, raw terrain readings, or terrain-probe controls.
1a. Confirm the Earthworks launcher button appears by itself without a separate rectangular background or border, and still opens/closes the panel.
2. In **Plan**, select a rectangle with two opposite-corner clicks while leaving the panel open. Set or sample a target height and check that footprint, cut and fill estimates appear.
3. Add the plan to the project queue. Switch to **Projects** and confirm the new project appears.
4. Choose a facility on the map. The map click should identify nearby buildings, then the Projects tab should offer recognized Earthworks facilities by distance. Select the intended facility and assign it.
5. Select different projects and check that status, progress, priority controls, and facility assignment follow the selected project.
6. Review available queued, running, waiting, paused, review-required, completed and cancelled displays. Start or pause work only in a disposable area if you want to check those states.
7. Check error/status messages, long text, window scrolling/resizing, screen edges, and your usual UI scale.
8. Leave the panel open while using native game controls. Click ordinary map controls and terrain to confirm those left clicks pass through when no terrain-picking action is active.
9. Save and reload the test save if desired, and confirm the Plan/Projects controls remain consistent with the stored projects.
10. Compare the panel to a native window: Arial UI text, rounded parchment corners with no extra rectangular box, and a red title strip integrated into the parchment art. Check the white title and close/minimize controls, then resize the window and check the corners.
11. Check the title: no white rectangle around the parchment, white title text, and visible “−” minimize and “X” close controls. Minimize, restore, and close the panel.
12. With Earthworks open but **Select area** or **Sample height** inactive, click ordinary map controls and terrain; vanilla input should pass through and no Earthworks area should be created. Then arm Select area and verify only its two terrain clicks are captured; repeat with Sample height's single terrain click. After each action completes, ordinary terrain clicks should pass through again.
13. While Select area is active, press Escape once and confirm picking stops while the panel stays open; press Escape again and confirm the panel closes without opening the game's Escape menu. Repeat with Sample height. Holding Escape should count as one press, not cancel and close at once.
14. Check selected tabs and pressed/hovered controls: their backgrounds should be lighter, dark text should remain the same color and readable, and text color should not switch with the state.

## Expected results

- Plan creation and project management are on separate tabs.
- Map selection remains available while the Earthworks window is open.
- Facility picking/assignment is handled in the Projects tab without raw diagnostic data.
- Developer-only inspector and terrain-probe UI are absent from this player build.
- Earthworks uses the installed game's integrated parchment-and-header menu strip art with the red/tan control palette; unavailable art falls back to a solid parchment surface and red header.
- Panel text uses the game's Arial UI font defined in `media_soviet/gui/default.ini`, with a default-font fallback if the system Arial file is unavailable.
- Ordinary left clicks pass through while no map-picking action is armed; Select area and Sample height capture only their intentional terrain clicks.
- Existing project behavior and terrain are unchanged by opening or navigating the UI.

## Report back

Tell me the resolution/UI scale used, whether the two tabs make the workflow easy to follow, whether map-based facility selection worked, and any clipped text or confusing action/status labels. Include which project states you viewed and whether you chose to start work.
