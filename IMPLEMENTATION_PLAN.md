**Earthworks Planning — staged implementation plan**

Updated 2 November 2026. RML is the primary host. The user has accepted the first planning prototype (0.2.0), controlled terrain-edit experiment (0.3.1), Stage 4a comparison (fuel unmeasured), Stage 4b dedicated-facility checklist and Stage 6 durable plans. The user fixed the Earthworks building's accepted vehicles to bulldozers, excavators, dumpers and buses only. D32 sets a provisional 22 worker-equivalent productivity reference for later balancing; it is not a measured terrain rate. Queued grey-wireframe visibility, stale-state, Save As, autosave and corrupt/mismatched companion checks were not reported as tested and remain documented as unverified. See the stage review log for evidence and progression.

**1. What we are building**

The player plans a desired terrain shape, reviews it, confirms a project, and lets workers and machinery carry it out autonomously while continuing to play. Workers and machines can remain at a nearby operation rather than travelling to individual sites. A dedicated building with worker and vehicle slots is a possible implementation.

Accepted prototype interaction: an on-screen Earthworks button, two opposite-corner clicks defining a rectangle, entered or sampled target height, and coloured wireframe/cut-fill shading with sharp footprint edges. Office rules, balance, persistence and the wider released tool scope remain open where listed in [the decision register](docs/DECISIONS.md).

The first end-to-end demonstration should let the player select an area, preview the result, confirm it, assign a staffed/equipped operation, leave the tool, and observe gradual work that survives saving and reloading. Extra terrain brushes, multiple offices, larger projects and release polish are added through later reviewed stages.

**2. Working agreement and stage gates**

Each stage follows the same loop:

1. Review the previous stage's accepted result and identify this stage's important gameplay/UX questions.
2. Ask the user about those questions before implementing the dependent behaviour. Give concrete options, tradeoffs and a recommendation.
3. Implement only the current authorized stage and complete the relevant technical checks.
4. Deliver an identifiable build with exact launch instructions, a short in-game checklist, expected results and known limitations.
5. Provide the checklist for the user to run in an agreed disposable test world. Record the user's actual observations separately from expectations; do not control or test the game on their behalf.
6. Pause for the user's review. The user can accept, request changes or revise the design. Fixes remain in the same stage and receive a focused retest.
7. Move to the next implementation stage only after explicit user confirmation. One reply such as "works as expected, continue" accepts the stage and authorizes the next; do not request the same confirmation twice.

A compiler pass, successful DLL load, RML discovery, screenshot or agent-only technical test cannot substitute for the user's in-game acceptance. Silence, elapsed time and a preselected answer are not acceptance.

Routine implementation details, read-only investigation, builds and fixes within an authorized stage do not need repeated questions. Important gameplay/UX choices and changes to scope do. If an unknown forces a fallback, present the evidence and alternatives before adopting it.

Before starting a stage, verify that its required user decisions are resolved. Independent investigation can continue while a decision is pending, but do not implement the choice-dependent feature or quietly select a default. Keep the outcome in the decision register and the stage review log.

Current records: [decisions](docs/DECISIONS.md), [stage reviews](docs/STAGE_REVIEWS.md), [tool setup](docs/SETUP_STATUS.md). These records are the source of truth for acceptance; this document describes the intended sequence.

The user authorized continuing without intermediate acceptance stops through the first planning prototype. That batch is now complete and accepted. The normal stage gates resume; acceptance of the preview does not approve untested terrain mutation, office mechanics or persistence rules. Stage numbers below are retained for traceability rather than implying that Stages 3 and 4 were completed before Stage 5.

**3. Review findings and changes to the earlier plan**

The earlier plan bundled terrain access, office integration, preview, execution and persistence into milestones that were too large for useful in-game feedback. This revision separates those concerns into ten observable stages.

It also treated several proposals as requirements without user confirmation. The numbers 20 workers, four machines, a 1,000-metre range and a 5-centimetre completion tolerance are not approved gameplay settings. A single active project, priority ordering, soil abstraction, omitted fuel costs, cancellation leaving completed work and rejection of all overlaps are also proposals rather than settled release rules.

The linked workplace/yard design remains an engineering alternative, not an automatically authorized substitute for a dedicated building. Terrain and office experiments establish the feasible options; the user then chooses how the mod should behave.

Persistence now has its own in-game gate before autonomous execution. Terrain protection is introduced with the first controlled edit and rechecked when execution expands; it is not postponed to final polishing.

A remaining technical uncertainty is whether the staffing/machinery combination can be built safely from existing native building behaviour. Another is whether bounded native terrain edits can converge to a preview while keeping collision and saved terrain consistent. The stages below test these questions before committing to a large feature.

**4. Preparation already completed**

- MSVC x64, the Windows SDK, CMake, Ninja and Git are available.
- An x64 diagnostic DLL builds and loads outside the game; its API-version export returns 4.
- RML 1.0.1 is pinned, with source and runtime hashes recorded.
- The existing plugin uses RML's Tesmio API 4 compatibility host. A standalone TesmioLoader runtime is not used.
- RML recognizes the installed game as 1.1.1.9 and displays the diagnostic as the single enabled plugin in Earthworks Development.
- The original game hashes are recorded. Discovery does not establish successful initialization inside the game.

The diagnostic profile currently enables zero Workshop items. Its first test world must therefore be a fresh vanilla-compatible fixture, not a normal republic that depends on disabled mods. Do not assume a copied existing save is suitable.

**5. Stage map**

| Stage | Small deliverable | What we confirm in game | Decision gate |
|---|---|---|---|
| 1 | Current RML diagnostic | Game and a disposable world open normally; plugin initializes | Agree on test fixture and review the result |
| 2 | Read-only world/terrain diagnostics | Positions, heights, simulation time and world lifecycle readings are credible | Developer display only; final UI remains open |
| 3 | One controlled terrain edit | Target, footprint, collision and native save/reload agree | Agree on the exact temporary test operation |
| 4 | Staffing and machinery feasibility | Readings follow real workers/vehicles; feasible office options are documented | Discuss office/availability options before choosing a production design |
| 5 | Area selection and target-height preview | Planning is usable and never changes actual ground | Selection controls, preview, edge treatment and initial tool scope |
| 6 | Durable drafts and queued projects | Correct plan state returns after save/load, autosave and Save As | Draft retention and recovery interaction |
| 7 | Office and project assignment | Agreed operation reports capacity and reserves equipment correctly | Building model, staffing, range, costs and assignment rules |
| 8 | One autonomous project | Staffed/equipped operation changes terrain gradually and resumes correctly | Throughput model, work appearance, pause/cancel and conflicts |
| 9 | Multiple projects and offices | Agreed scheduling and conflict rules remain understandable and correct | Queue policy, sharing, priorities and overlap behaviour |
| 10 | Balanced candidate package | Agreed workflow works in intended gameplay settings and a clean install | Rates, limits, presentation and release scope |

Every row is a separate in-game review boundary. A stage may be split further if its checklist becomes too broad. Splitting never removes an existing acceptance gate. Public publishing is a separate user decision.

**6. Stage 1 — verify the existing diagnostic inside the game**

Implementation: use the current diagnostic build and pinned RML profile; do not add terrain, staffing or simulation hooks.

In-game checklist:

1. Open the project-local RML launcher and confirm Earthworks Development selects only the diagnostic.
2. Launch the game and reach the main menu.
3. Open a fresh, disposable vanilla-compatible world and use the normal camera and menus.
4. Return to the menu and load that test world again.
5. Exit normally and inspect the RML lifecycle result and the diagnostic's initialization/start messages.

Acceptance evidence: successful plugin initialization and start, normal menu/world transitions, no unexpected game behaviour, and the user's confirmation. If initialization fails, fix only the loader/ABI diagnostic and repeat this check.

Current status: baseline diagnostic checks were verified, and the subsequent planning prototype was accepted by the user after its supplied launch/world/reload/input checklist passed. The diagnostic evidence remains in the stage log.

**7. Stage 2 — read-only world and terrain observations**

Implementation: establish safe game-thread observation, a world-generation token, verified simulation-time readings, terrain coordinates/grid spacing, height sampling and observations of save lifecycle. Provide a clearly labelled developer display or diagnostic output, not a final product interface.

In-game checklist:

1. Sample several visible high/low points and compare with available native elevation information.
2. Move the cursor and camera and confirm coordinates and height readings follow the correct location.
3. Pause and change game speed; confirm simulation observations stop/scale as expected.
4. Return to the menu, load a different fixture and reload the original.
5. Save and Save As while observing lifecycle events.

Acceptance evidence: credible measurements, no stale references after world changes, no terrain writes and no crashes. Save event observations are evidence about timing, not proof of snapshot consistency.

Ask about final overlay placement and keybindings before turning this temporary developer display into a player-facing control.

**8. Stage 3 — controlled terrain-edit proof**

Implementation: in an explicitly selected, empty dry test patch, perform a small bounded edit toward a known target through verified native operations. Read back the result and establish required mesh/collision/terrain-state updates. Include the operation's entire influence area in its bounds.

The temporary diagnostic control is for this experiment only. It does not establish released project costs, duration, cancellation behaviour or office rules.

In-game checklist:

1. Record patch heights, target and boundary before the operation.
2. Trigger one agreed small edit; compare visible terrain and measured heights.
3. Check nearby samples outside the footprint remain unchanged.
4. Compare cursor picking and native building/road placement against the changed ground.
5. Save the test result and reload it; confirm the changed terrain is retained correctly.
6. Repeat a small raise and lower if the chosen native path supports them safely.

Acceptance evidence: bounded predictable movement, consistent placement/collision, correct native save/reload and user confirmation that the visible result matches the test. Measure native quantization and convergence rather than imposing a guessed tolerance.

If only brushes are usable, prove their influence and convergence before promising precise target-height projects. Unexpected native charges or borrowed native machine capacity must be explained and resolved, not hidden.

**9. Stage 4 — prove workforce and machinery access**

Implementation: observe actual staffing, effective productivity where available, vehicle ownership/skills and availability in existing native test buildings. Separate read-only feasibility probes from a final office design. Investigate equipment exclusivity and any native/manual terraforming interaction in a dedicated fixture.

Current iteration: 0.5.0-dedicated-facility adds a local Workshop building cloned from the native construction-office shell, with temporary 5-worker / 2-vehicle-station settings. The user chose to block normal jobs. The Earthworks plugin lets the player register one selected building per loaded session and skips the native construction-office assignment routine for that exact building instance; registration clears on reload. The user reports no Stage 4b checks outstanding. The vanilla panel did not show worker occupation, though the user saw workers arrive and the inspector counted them. The user chose to keep the regular construction-office type and defer the rail-office alternative as overkill. The user then fixed the permitted vehicle classes to bulldozers, excavators, dumpers and buses only. Stage 4 must establish enforcement and modded vehicle classification, plus productivity and equipment reservation, before execution is designed.

In-game checklist:

1. Let workers arrive at and leave a test workplace; compare native UI values with diagnostic readings.
2. Add, move and sell eligible machinery; verify ownership and availability readings.
3. Distinguish usable idle machines from travelling or otherwise assigned ones.
4. Reload the fixture and demolish/replace a test building; confirm stable identity handling.
5. Demonstrate any proposed reservation mechanism with a reversible test and show whether native/manual work can still use the same machine.

Acceptance evidence: truthful readings and a written account of which single-building or linked-building options are technically supported. Where exclusivity remains unproven, record it as a blocker for autonomous execution.

The user chose a dedicated Earthworks building with its own worker and machine slots (D25), and chose to block ordinary construction jobs from it (D26). The first test item uses a native construction-office donor with temporary values of 5 worker places and two vehicle-station placements. The player registers that exact building instance for the loaded session; an RML hook blocks the native assignment routine for it. The Workshop content item is staged alongside the plugin. Test instructions are in docs/STAGE4B_TESTING.md. Model, capacities for a candidate release, and registration persistence remain open.

**10. Stage 5 — planning interaction and preview**

Current status: 0.2.0-prototype accepted by the user on 1 November 2026: "All checks passed". The preview's required read-only bindings were implemented and tested. Stage 3's separate controlled terrain experiment has since been accepted; office integration and draft persistence remain pending.

Confirmed interaction: select a rectangle with two opposite-corner clicks and enter or sample a target height, opened through an on-screen Earthworks button only. Use coloured wireframe/cut-fill shading and sharp footprint edges for this prototype.

The prototype's selection, preview and edge questions were resolved explicitly by the user (D17/D18). Ask about any expanded terrain tools or changed edge behaviour before implementing them.

Implementation: maintain a separate planning grid, support draft editing/undo, display the proposed shape and communicate invalid areas. No native terrain writes are allowed during planning, confirmation in this stage or closing the interface.

In-game checklist:

1. Select a small area at different camera angles and zoom levels.
2. Enter a target elevation, then sample a height and compare the preview.
3. Review both cut and fill cases and the complete edge/transition footprint.
4. Edit, undo and cancel a draft; compare actual ground before and after.
5. Open and close the tool while playing normally; verify input capture is released.
6. Review invalid selections and messages together.

Acceptance evidence: user-approved interaction and a credible preview, with unchanged native terrain. Record desired changes to the UX before implementing execution.

**11. Stage 6 — persist plans before executing them**

Implementation: create versioned draft/queued-project records and match their metadata to the correct native save snapshot. The 0.6.1-queued-wireframe prototype writes an atomic `earthworks.dat` companion in the native save folder after the observed native save trigger completes. It records a checksum and SHA-256 of that slot's `heightmap.dds`; load restores only a valid matching companion. A queued project remains visible as a grey wireframe while the editable draft uses cut/fill colors. This stage can confirm one plan as queued, but cannot execute terrain work. The code remains subject to the user's Save/Save As/autosave/reload check.

The user chose to retain drafts and confirmed projects (D33). Missing companion data produces an empty plan state; corrupt or mismatched data is refused with a status message and discarded, with no manual recovery/import flow for now (D34). The native save and terrain are never rewritten during recovery. A terrain fingerprint binds metadata to a save snapshot but does not replace execution-time checks that compare each project's sampled ground to the current terrain.

In-game checklist:

1. Save and reload a draft and a confirmed queued project.
2. Verify targets, footprint, assignment if any and explicit paused state.
3. Trigger autosave, use Save As, and load an older branch of the fixture.
4. Confirm plans from a later branch never leak into an older save.
5. Exercise a controlled missing/corrupt metadata case and review the message.
6. Load a different world and confirm no plan or engine references carry over.

Acceptance evidence: correct matching snapshots and user-approved retention/recovery behaviour. Steam Cloud support is not assumed; test it separately if requested for the release.

**12. Stage 7 — office assignment dry-run**

The existing dedicated 5-worker/2-machine development facility is the test office. Identify every placed copy by its unique type-descriptor asset name; do not require per-session registration. Allow a player-selected assignment from one queued project to a selected facility, then show a dry-run of observed occupants and matching parked vehicles. Treat parked vehicles as candidates only: current evidence does not prove idle status, native availability or exclusive reservation. Show distance to the plan centre without claiming coverage. Do not edit terrain, consume resources, reserve vehicles, or estimate completion time in this stage. Show the provisional D32 relative rate only with a clear estimate label. Save the selected native building ID with the queued project and re-resolve it by both ID and asset name after load.

Resolved choices: show distance only without a coverage claim (D37), manually assign one queued project to a selected facility (D38), and recognize every dedicated Earthworks facility instance automatically (D36). Runtime matches the observed full descriptor asset name `9100000012/EarthworksDevelopmentFacility`; generic construction offices remain unaffected. D08's production range and partial-plan rules remain open. Fuel, cost and soil/material accounting (D09) must be resolved before Stage 8 execution. Native availability and exclusive reservation remain Stage 4 blockers, not promises of this dry-run.

In-game checklist:

1. Verify automatic identity on multiple Earthworks facilities and confirm a native construction office remains unaffected.
2. Assign one queued plan manually to a selected recognized facility and check that the summary is labelled as a dry-run.
3. Change occupants and parked equipment candidates; confirm observed counts and the provisional one-machine relative rate respond.
4. Confirm the panel shows distance only and never claims coverage, availability, reservation or completion time; terrain remains unchanged.
5. Save/reload and confirm the queued project resolves only to the assigned ID plus exact facility asset name; demolition must show unresolved without silently substituting another facility.

Acceptance evidence: assignment, dry-run labels, displayed observations and chosen provisional range behavior match the user's expectations, with no terrain edits. This does not establish actual resource availability or exclusive machinery reservation. Both remain prerequisites for Stage 8.

**13. Stage 8 — first autonomous project**

Implementation: connect the approved project model, terrain adapter, office capacity and save handling. Limit the first demonstration to one operation and one agreed small patch. Progress follows simulation time; no held input or open planning window is required.

Before implementation, ask about the proposed throughput formula and visible working pattern, pausing, stopping/cancelling partially completed work, and what the player should do after external terrain changes. Do not implement free rollback, permanent partial work or automatic replanning as an unconfirmed default.

In-game checklist:

1. Preview, confirm and assign one project.
2. Leave the tool and use the camera/other menus while work continues.
3. Remove staffing or machinery and observe the agreed waiting behaviour.
4. Pause/change game speed and check progress is consistent.
5. Save while partially complete, reload and continue from that exact saved state.
6. Test the agreed pause/cancel interaction and a manual-terrain conflict.
7. Verify final heights, protected boundary, collision and saved result.

Acceptance evidence: first complete player-reviewed autonomous earthworks loop, with correct partial save/resume and no unexplained costs or double-counted machinery. If partial-save synchronization fails, the stage remains open.

**14. Stage 9 — multiple projects, offices and conflicts**

Before implementation, ask about sequential versus shared capacity, priorities/automatic assignment, whether blocked projects hold up a queue, overlap rules and reassignment behaviour.

Implementation: add only the chosen scheduling model. Reuse the same validated single-project executor; do not create a second terrain-writing path.

In-game checklist:

1. Queue several projects and verify the agreed order and status display.
2. Run two operations and show that workers/machines are counted only once.
3. Block or pause the first project and observe the chosen queue behaviour.
4. Exercise overlapping plans, reassignment and an office being demolished.
5. Add new infrastructure or manual edits near an active footprint; verify protective checks and conflict handling.
6. Save/reload with several states present and repeat the queue check.

Acceptance evidence: the user can predict which project runs and why, capacity is conserved and conflicts are explained. Split scheduling and expanded terrain/protection support into separate review builds if needed.

**15. Stage 10 — balance and candidate package**

Before implementation, review rates, office capacity/radius, costs/fuel/soil scope, supported terrain tools, large-project limits, completion feedback and intended gameplay settings. Features not yet chosen remain outside the candidate rather than being silently added.

Implementation: tune the accepted behaviour, finish necessary presentation, set measured performance limits, and prepare an installable candidate with source/notices and clear compatibility notes. Public uploading is not part of this stage.

In-game checklist:

1. Complete the agreed workflow in the intended gameplay settings, including representative cut and fill projects.
2. Compare duration/costs with the user's expectations; adjust and repeat.
3. Run the agreed maximum project/queue size and inspect frame-time impact.
4. Install the candidate cleanly, reload the test saves and verify upgrade/disable behaviour.
5. Review messages, inputs and completion feedback in normal play.
6. Test any explicitly promised cloud/mod interoperability separately.

Acceptance evidence: the user approves the candidate's behaviour, UX and balance; observed limits and unresolved issues are recorded. Decide distribution and additional features afterwards.

**16. Engineering constraints retained across stages**

Keep native game bindings isolated from gameplay calculations and the host adapter. Every binding needs its supported binary identity, signature/calling convention, evidence and failure behaviour recorded. Code that only works on a known binary must decline unsupported builds rather than guess.

All native terrain changes and mutable engine reads run at a verified safe game-thread boundary. Renderers use snapshots and a command queue; background geometry uses copied data and revalidates the world generation on return. Persist stable identities, not raw pointers.

Represent affected terrain with verified grid coordinates, current expected heights, target heights and an explicit influence mask. Include transition/brush spill areas in previews, ownership/range checks, volume estimates and conflict checks. Use native triangulation or verified area weights to avoid counting each vertex as a full cell.

Use bounded native edits and measure successful movement by readback. Characterize derived terrain updates, unexpected native charges and native machine participation. Do not substitute direct raw height-buffer writes for a missing safe update path.

Native time drives work; wall-clock FPS does not. Cap per-update work, never accumulate work without the required resources, and detect stalls/oscillation. Completion tolerance follows measured native precision plus the accepted UX.

Keep reservations exclusive across mod and native tasks. Revalidate staffing, machine ownership and availability before consuming capacity. Release stale reservations on world unload, reassignment, pause/cancel and destruction according to the agreed gameplay rules.

Protect occupied terrain from the first mutation experiment. Initially use a small verified empty dry fixture and include underground infrastructure and the full influence footprint in later validation. If protection cannot be established, restrict the experiment and report the limitation instead of claiming general support.

Keep actual terrain in native saves. Save plugin metadata at the same native snapshot boundary with schema/version and matching world/snapshot identity. Atomic companion-file replacement alone does not solve synchronization with an asynchronous native save. Mismatch/corruption must not auto-resume terrain writes.

Use meaningful core tests for geometry/volume, state transitions, scheduling, capacity and persistence as those behaviours are introduced. Native consistency, machine availability, save matching and the interaction itself also require in-game evidence. Do not describe unexecuted checks as passed.

**17. Evidence and source references**

The pinned local RML source, inspected compatibility SDK, original game configuration and recorded hashes are the current engineering baseline. References describe useful mechanisms, not a ready-made terraforming-job API:

- [RML plugin SDK](https://github.com/Ultimate-Universe/WRSR-RepublicModLoader/blob/main/Source/docs/PLUGIN_SDK.md)
- [RML Tesmio compatibility](https://github.com/Ultimate-Universe/WRSR-RepublicModLoader/blob/main/Source/docs/TESMIO_COMPATIBILITY.md)
- [Tesmio API documentation](https://github.com/MaxLegend/TesmioLoader/blob/master/docs/09-plugins.md)
- [RepublicMP terrain/world integration architecture](https://github.com/E3418/RepublicMP/blob/main/ARCHITECTURE.md)

Local road construction-office configuration declares machinery but no worker-capacity declaration in that file; rail construction-office configuration declares both, with different native behaviour. Neither proves a combined earthworks building is available by adding a token. Preserve upstream licences and source provenance for adapted code.

The earlier, ungated plan is preserved for history in [the archived plan](docs/archive/IMPLEMENTATION_PLAN-before-stage-review-2026-11-01.md). It is superseded by this document and its decision/review records.

**18. Immediate next action**

Stage 6 is accepted with stale-state protection deferred. Stage 7 is accepted: the user confirmed that a separately saved and reloaded test slot retained queued project 1's assignment to Earthworks facility ID 14, and the inspector again recognized `9100000012/EarthworksDevelopmentFacility`. The dry-run remains read-only; the screenshot showed 0 observed occupants, 0 machine candidates and rate 0.00. Details are in [STAGE7_TESTING.md](docs/STAGE7_TESTING.md).

Stage 8 is the next implementation stage, but autonomous terrain work remains gated on resolving actual native machine availability and exclusive reservation, then agreeing on fuel, cost and soil/material accounting. Before implementation, ask the user about throughput, visible work behaviour, pause/cancel semantics and terrain-change conflicts as described in Stage 8. Do not present Stage 7's candidates or dry-run rate as proof that equipment is available.
