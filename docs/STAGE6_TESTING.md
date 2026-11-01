# Stage 6 — durable draft and queued-project save/load check

User-run in-game check for the versioned Earthworks companion state. The agent must not perform these game checks. Do not use a valuable save; make a disposable copy first. Native terrain remains unchanged by this stage.

## Save and reload a draft

1. Start the disposable save with the Stage 6 build. Open Earthworks and create a complete rectangle preview with a distinctive target height.
2. Record the native ground height at one point inside the rectangle and the preview target shown by Earthworks.
3. Click **Confirm and queue project**. Confirm Earthworks reports one queued project and explicitly says no terrain work runs in this stage.
4. Select a different rectangle and set a second target height so the save contains both a queued project and an unconfirmed draft.
5. Use the game's ordinary Save command. Wait for it to finish, then check the matching folder under `media_soviet/save/` for `earthworks.dat`.
6. Return to the main menu and reload that same save.
7. Open Earthworks. Confirm the second rectangle/target and queued-project message return, and confirm actual ground at the recorded point is unchanged.

## Save As and stale-state protection

8. If autosave is enabled, wait for one autosave interval. Check that `earthworks.dat` is present or updated and reload the autosaved slot; the draft and queued project should return.
9. Use Save As to create a new disposable slot. Confirm it has its own `earthworks.dat` and reload that slot; the draft and queued project should return.
10. For a mismatch check, use two disposable save slots with different terrain. Back up the target slot's files, copy the source slot's `earthworks.dat` into the target slot, then load the target. Earthworks should restore neither plan and show a mismatch status. Restore the backed-up companion afterward. Do not copy or replace native terrain files for this check.
11. Load an older save from before the Earthworks companion existed. Earthworks should open with no restored plan and no error dialog; the game save must remain loadable.

## Report

- Whether `earthworks.dat` appeared after Save and Save As, and in which slot folders.
- Whether the draft/target and queued project returned after each reload.
- Whether actual terrain stayed unchanged during save/load.
- Any Earthworks log line about saving, restoring, or refusing a mismatched companion.
- Whether the game returned to the menu and exited normally.
- Whether saving or loading caused a noticeable pause.

This validates draft and queued-project persistence. Queue ordering and execution are not implemented in this stage. The terrain fingerprint ties companion data to a save snapshot; it does not revalidate each draft's sampled ground after terrain is edited and saved again. Stage 6 performs no terrain writes, and Stage 8 must revalidate the project footprint before any execution.
