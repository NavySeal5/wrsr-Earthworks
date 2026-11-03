# Stage 10b.2 — named Project management

For the combined first UI implementation (build 0.13.0), use [STAGE10B_FIRST_UI_TESTING.md](STAGE10B_FIRST_UI_TESTING.md), which supersedes the Plan-tab workflow below.

Build: **0.12.1-stage10b2-project-ui**. Use a fresh test save for the reworked companion schema. The build and RML staging are technical checks only; the game was not launched and no in-game test was run by the agent.

## Project and Site organization

1. Open Earthworks and check that `Project 1` is selected.
2. Create two named Projects, rename one, switch between them, and verify each Sites list stays with its Project.
3. In the current Plan tab, create a Site in each Project. Verify the Site appears only under the selected Project and receives a sequential `Site-N` name.
4. Rename a Site. Confirm the name remains unique within its Project and that a duplicate name is rejected.
5. Move Sites up/down in one Project and confirm the other Project's ordering is unchanged.
6. Try deleting a Project with an unfinished Site. It should be blocked. Cancel or complete the Site and retry; deletion should remove that Project's ended Site rows.
7. Remove completed Sites and confirm the bulk action affects only the selected Project.

## Facility assignment

1. Select a Project and open **Facilities**.
2. Use **Choose facility on map**, select a recognized Earthworks facility, then add it from the candidate list. The distance is informational only.
3. Add a second facility to the same Project. Confirm both IDs appear under Assigned to this Project.
4. Select another Project and inspect the same facilities. A facility already owned by the first Project should be unavailable there.
5. Assign two facilities to one Project, give it multiple Sites with different priorities, and start more than one Site. Confirm the assigned facilities' available workers and eligible machines contribute to that Project's Sites by priority. A Site with no allocated capacity should wait. Facilities must not be shared between Projects.
6. Save and reload. Confirm the selected Project, names, Site ordering/status, and Project facility assignments are retained.

## Report back

Tell me which steps you completed and any unexpected UI messages, state changes, or terrain edits. Stage 10b.1 storage and this 10b.2 management checkpoint remain pending your in-game review. The Plan tab and site-local editor changes are subsequent checkpoints, not part of this build.
