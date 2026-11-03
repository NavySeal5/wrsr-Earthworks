# Stage 10b — Named projects and in-context earthwork editing

**Status:** In progress; checkpoint 10b.2 is ready for user review. Stage 10b.1 schema acceptance remains unverified.

## Purpose

Organize multiple terrain work sites under named Projects, and make creating/editing an individual Site happen in context on the map. A Project is a named container that holds multiple Sites; each Site is one rectangular terrain operation with its own target, status, progress and priority. In player-facing text, use “Site” for an individual terrain work item (D81).

## Intended player workflow

1. Open the Earthworks panel. The Plan tab is gone; the Projects view is the main workflow.
2. Use a project selector to switch the Sites list to the selected named Project. Use adjacent inline New, Rename and Delete controls for Projects. Rename individual Sites as well. Remember the selected Project after save/reload.
3. Click **New Site** below the list. It is assigned to the selected Project and receives an editable sequential default name (`Site-1`, `Site-2`, etc.), then the player selects two opposite terrain corners using the current area-selection behavior.
4. After selecting the site, show a separate editor box beside that site. It contains footprint dimensions/area, sampled terrain height, cut/fill estimates and the target-height control. Angle is a possible future field, not part of this stage's first implementation.
5. Clicking the earthwork site opens its editor again. The selected site's preview and editor can be used without returning to a separate Plan tab.
6. Assign one or more Earthworks facilities to the named Project as a whole. A facility belongs to at most one named Project. Its facilities then provide capacity to the Sites in that container.
7. Manage each Site with the existing row workflow: within-Project priority ordering, start/pause/review/resume, cancel, disabled camera View, and removal after completion. Make the Sites window wide enough for row controls to fit on one horizontal line (D82). The list shows only Sites in the selected Project.

## Implementation scope

- Separate the persisted data model into named Project containers, project-level facility assignments, and Site records. Keep terrain plan, progress, review state, editable sequential default name (`Site-1`, `Site-2`, etc.) and scheduling priority on each Site; attach one or more facilities to the Project.
- Update the save companion format with bounded counts, validation, checksum and terrain fingerprint protections. Store named Projects, their facility pools, the selected-project ID, and child Earthworks with their names, plans and execution state.
- Use a fresh save for the reorganized schema. The user confirmed that old WIP records do not need migration; do not spend implementation time converting the old flat project list.
- Keep current terrain execution, rate, overlap, machine/worker allocation, facility rules and no-material/no-fuel behavior unchanged. This stage is organization and interaction only.
- Keep the accepted game-like parchment/control styling across the redesigned panel and detached editor.
- Remove the Plan tab. Keep the two-corner rectangle selection. **New Site** begins selection and binds the result to the selected container.
- Add the detached site editor. It shows the target height now; angle remains deferred. Reopening an Earthwork's editor must not start another site selection.
- The completed-item cleanup button affects only the selected named Project.
- Block deleting a named Project while it contains any queued, running, waiting or paused Site. If it contains only completed/cancelled Sites, deleting the container removes those history rows without changing terrain.
- Move-up/move-down priority controls order Sites only within their named Project.
- Assign one or more facilities to each named Project, and do not allow a facility to be shared between Projects. A Project's facilities form the work pool for its Sites; current staffing, machine capacity, concurrent work and priority rules operate within that Project's Sites list.

## Checkpoints and in-game review gates

Each checkpoint ends with a user-run in-game review. Do not move to the next checkpoint until the user confirms the behavior or requests adjustments.

### 10b.1 — Data model and fresh-save storage

Implement named Project and Earthwork records in a new companion schema while preserving execution behavior. A fresh world starts with one `Project 1` container; the selector and create/rename/delete controls arrive in 10b.2. No schema-4 migration is required. On a fresh save, test the default container, generated site names, Earthwork status/progress, project-level facility link and save/reload. Confirm planning/storage changes do not alter terrain. Selected-container reload behavior becomes directly selectable and testable once the 10b.2 selector is implemented.

### 10b.2 — Named-project management

Add the selector and inline create, rename, delete and facility assignment controls shown in [mockup A](mockups/STAGE10B_UI_PROPOSAL_A_WIDE.svg). Switching the selected Project filters the Sites list without changing Site state or scheduler state. Add a rename action for each Site. Assign and remove multiple facilities at the Project level; reject assigning a facility already owned by another Project. Test with several Projects containing mixed Site states, then save/reload. Deletion must be blocked while unfinished Sites are inside the container; ended records can be removed with their container.

### 10b.3 — Create Earthwork from the Projects view

Remove Plan tab and add **New Site** below the list. Confirm a selected Project is required, the new Site is assigned to it, the two-corner selection works, and normal map interaction returns when selection finishes or is cancelled. Confirm planning alone never edits terrain.

### 10b.4 — Site-local editor

Show a separate world-anchored editor beside the newly selected site with target height. Clicking the site reopens it. Confirm its position tracks the footprint as the camera pans/zooms, it remains usable near screen edges, it does not steal ordinary map input when closed, and changing target height updates the preview. Save/reload and reopen an existing Earthwork editor.

### 10b.5 — Regression and acceptance

Check facility assignment at the Project level, facility exclusivity across containers, within-Project ordering, facility concurrency, work controls, disabled camera View, review/pause/cancel, completion removal, overlapping-footprint rules, save/reload and filtering across multiple containers. Confirm the cleanup button removes completed Earthworks from the selected Project only. Stage 10b is accepted only after user review.

## Confirmed rules and remaining decision

- Existing saves do not need migration (D72); Stage 10b tests use fresh saves. The new companion format only needs to read its own new schema.
- Deletion is blocked if a container includes unfinished Earthworks (D73).
- The editor is world-anchored beside the site (D74).
- Earthwork row order is local to the selected Project (D75).
- Bulk completed cleanup affects only the selected Project (D76).
- Facilities are assigned to named Projects as a whole. One Project may use multiple facilities, and a facility cannot be shared between Projects. Its facility pool supplies the Earthworks in that Project, with existing staffing, machine and priority rules applied within the group (D77).
- Each Site gets an editable sequential default label (`Site-1`, `Site-2`, etc.) (D78), and the selected Project persists after save/reload (D79).
- Use inline Project actions and a wide, single-line Sites row layout as in mockup A (D80, D82).


## Deferred to later work

This stage does not balance productivity, add quarried stone/fuel accounting, add vehicle reservations, introduce non-rectangular/angled terrain operations, prevent buildings from being planned over active sites, or add machine animations. Angle is reserved for a future Earthwork editor extension.

