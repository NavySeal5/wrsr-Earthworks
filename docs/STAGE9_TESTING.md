# Stage 9 — multiple projects and facility scheduling

Build: **0.10.0-stage9-multiproject**. These are user-run gameplay checks; the agent has not launched the game or changed a save.

## Setup

1. Use a disposable copy of `Earthworks_Test_S01` or a fresh test republic. Keep projects away from roads, buildings, utilities and other infrastructure. D49 selected **20 worker places and 2 machine stations** for this test fixture. A later asset audit found the generated construction-office template actually retained **16 vehicle-parking points**; Stage 9 scheduler behavior was reviewed, but a two-slot maximum was not established. Stage 11 will correct the facility item.
2. Close the game and project-local RML. From the mod folder run `launch-prototype.cmd`, confirm **Earthworks Planning - Multi-project Prototype** version **0.10.0-stage9-multiproject**, then launch the game from RML.
3. Open Earthworks and confirm the panel lists projects and shows the Stage 9 header. The launch script does not load a save for you.

## Project list, priority and overlap

1. Plan and confirm two or three separate rectangular footprints. Each should be large enough to contain a complete terrain cell and should not share an edge or area with another live project.
2. Confirm each appears in the project list and as a grey terrain outline. Select different entries and check that the detail panel and wireframe follow the selected project.
3. Use **Priority up/down** on live projects. Confirm the displayed order changes and remains after a save/reload.
4. Select a new rectangle that overlaps a queued or active project and press **Confirm and queue project**. The mod should refuse it with an overlap message and should not add another list entry.
5. Complete or cancel one project, then replan over its old footprint using the terrain as it now exists. Confirmation should be allowed once it no longer overlaps any queued, waiting, running or paused project.

## Assignment, start and concurrent work

1. Assign two disjoint queued projects to the same Earthworks facility. Assignment should remain manual; assigning alone must not start work.
2. Ensure the facility has at least two parked eligible machine candidates and at least 20 workers assigned there. Start both projects explicitly.
3. Verify both advance at once, the list reports **Running**, and the selected detail shows per-tick workers/machines allocated. Increase active projects beyond the facility's workers/machines and confirm unstaffed projects show **Waiting**.
4. Reorder the projects. The higher-priority started project should receive workers first; lower-priority work should wait when the worker pool runs out. A single project may use several observed machines when no higher-priority project takes those resources.
5. Pause a high-priority project and verify eligible lower-priority work continues. Resume it and confirm the priority allocation returns to the new order. Trigger a review pause on one footprint with native terrain editing; other non-overlapping work should continue while the reviewed project is skipped.
6. Assign another started project to a different Earthworks facility and verify its workers/machines do not reduce the first facility's allocations.

## Save/load and recovery

1. Save while projects are queued, running, waiting and paused (use separate saves/checkpoints if convenient). Reload each checkpoint.
2. Confirm project order, assignments, progress and statuses return. Running work should resume scheduling from the saved progress; machine/worker allocations are recalculated from current facility readings rather than persisted.
3. If a facility is demolished or unreadable, confirm its active projects pause and explain the missing facility. Select another Earthworks facility and reassign the paused project; progress should remain, and any terrain-review requirement should remain until review-and-resume.
4. Finish at least one project and verify its terrain reaches the saved target without changing another project's footprint.

## Report back

Please report which numbered checks you completed, what the project list and allocation counts showed, whether save/reload preserved priority and progress, and any exact warning text. Do not use the disposable fixture or terrain-test buttons for normal republic saves.
