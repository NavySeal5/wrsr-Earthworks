Earthworks Planning — implementation plan
=======================================

Prepared 1 November 2026. Status: the initial toolchain and diagnostic plugin scaffold are now prepared; gameplay integration remains pending. See [setup status](../SETUP_STATUS.md) for verified local results.

**1. Intended result and first-release scope**

The player designs a desired terrain shape, confirms it as a project, and assigns an earthworks office. Actual workers at the office and eligible machinery held by that office determine how quickly the ground changes. Workers and machines do not travel to project sites. The player can leave the tool and continue playing while work proceeds.

The first usable release must include:

- A planning interface with a target elevation, rectangular selection, and a visible terrain preview.
- Confirmation before any actual terrain changes, plus draft undo/redo and cancellation.
- A real, staffed workplace and real owned machinery. Configured worker slots alone do not count as available workers.
- A configurable operating radius, one active project per office, and a priority queue.
- Gradual terrain changes driven by simulation time, including pause and game-speed handling.
- Clear reasons for waiting: no office, no workers, no eligible machinery, out of range, obstruction, or changed terrain.
- Save/load, autosave and Save As support for drafts, projects, assignments and progress.
- Protection for buildings, infrastructure, water and other projects.

First-release terrain tools are flatten to an entered or sampled height, and raise/lower a selected rectangle by an entered amount. A circular paint brush follows after the rectangle version works. Smoothing, graded ramps and polygon selections are later additions.

The first release does not simulate hauling soil, a soil stockpile, machines travelling across the worksite, or multiplayer. Fill material is abstracted, so cut and fill volumes are reported independently without a physical mass-balance requirement. Any additional fuel consumption for remote work is a separate later feature; the initial release must clearly describe this simplification. Existing native vehicle operating requirements must still be respected wherever they apply.

**2. Evidence, limits and community foundations**

The installed Steam manifest reports build 23935965. Community WRSR-MODS documentation associates that build with game version 1.1.1.9. This is a useful starting point, not sufficient verification of binary compatibility: record executable and engine hashes before installing hooks.

TesmioLoader provides a native DLL plugin lifecycle, logging and hooking helpers. Its documentation describes game-specific features as plugin responsibilities. Build against a pinned release and matching headers rather than the changing upstream default branch. [Tesmio plugin documentation](https://raw.githubusercontent.com/MaxLegend/TesmioLoader/master/docs/09-plugins.md)

Republic Mod Loader is the selected primary host. Its native API provides game information, symbols and shared services; API 1 does not supply a general managed hook/callback interface. Initially retain the diagnostic plugin's Tesmio API 4 interface through RML's compatibility host. A standalone TesmioLoader runtime is not a project requirement. Consider native RML exports when their benefits justify migration, and verify each host-mediated hook rather than assuming interchangeable behaviour. [RML plugin SDK](https://raw.githubusercontent.com/Ultimate-Universe/WRSR-RepublicModLoader/main/Source/docs/PLUGIN_SDK.md)

RepublicMP documents terrain-edit interception, heightmap access, simulation and world-load hooks, and an ImGui overlay. This establishes a useful research path, not an existing autonomous-earthworks API. Study its terrain and lifecycle modules without making multiplayer a runtime dependency. The documented integration is build-specific. [RepublicMP architecture](https://raw.githubusercontent.com/E3418/RepublicMP/main/ARCHITECTURE.md)

Soviet Mod Loader and Workshop Bridge are distribution options to evaluate after the standalone plugin works. Avoid requiring resource-expansion plugins solely to deliver this feature. [Soviet Mod Loader](https://github.com/lucasmorais99/soviet-mod-loader), [WRSR-MODS](https://github.com/Kespri/WRSR-MODS)

Local configuration evidence:

- `media_soviet/buildings_types/construction_office.ini`: native road construction office, eight working vehicles, parking declarations, no worker-capacity declaration in this file.
- `media_soviet/buildings_types/rail_construction_office.ini`: rail construction office, four working vehicles and sixty workers.
- `media_soviet/scripts/SOVIETInstructions.txt`: building-type and vehicle-skill constants useful as research landmarks.

Neither example proves that a road machinery office can employ workers merely by adding a token. Community building-generation documentation also distinguishes new building content from a genuinely new engine building type. Do not invent `$TYPE_EARTHWORKS_OFFICE` and expect the engine to implement it. [Building documentation](https://raw.githubusercontent.com/MaxLegend/TesmioLoader/master/docs/13-buildings.md)

When adapting community code, record its commit and licence. Several candidate projects use GPLv3; preserve required notices and provide corresponding source when distributing derivatives as required. This is a packaging task, not a reason to copy an entire loader or multiplayer project.

**3. Implementation approach and project structure**

Use an x64 C++ plugin hosted by a pinned Republic Mod Loader release, initially via its Tesmio API 4 compatibility layer. Keep the game integration separate from the host adapter so native RML exports can be added later without rewriting gameplay. Use MSVC C++ build tools, the Windows SDK, CMake and Git. Use Ghidra and an x64 debugger only where existing source and exported engine functions do not answer a required question. The initial office may reuse suitable game building assets in a separate development item; custom modelling is unnecessary for the prototype.

Keep the gameplay calculations independent of game memory and rendering. Suggested source layout:

```text
earthworks-mod/
  IMPLEMENTATION_PLAN.md
  CMakeLists.txt
  src/
    plugin/          entry points, host adapter, version checks, diagnostics
    game/            terrain, clock, world lifecycle, offices, vehicles, save adapter
    core/            plans, geometry, projects, scheduler, capacity, validation
    ui/              input, planning panel, preview, office panel, project list
    persistence/     serialization, schema versions, snapshot verification
  assets/            office development item, icons, translations
  tests/             focused calculation and persistence tests
  docs/              discovered bindings, manual tests, installation, compatibility
```

The names above describe interfaces to implement, not functions known to exist in the game. In particular, the terrain adapter needs operations equivalent to read region, query protected area and apply a bounded edit; each must be backed by verified native behaviour.

All native addresses, layouts and calling conventions belong in the game adapter. The core receives stable handles and value snapshots, never raw engine pointers. Every binding records supported hashes, how it was found, expected signatures and the observation that validates it.

Rendering consumes immutable snapshots and submits UI commands to a queue. Only a verified safe game-thread callback reads mutable engine state or changes terrain. Heavy geometry preparation can use copied data off-thread, but results must be revalidated against the current world generation before use.

**4. Technical investigation gates**

Complete these before investing in building art or a polished interface.

| Gate | Required investigation | Passing evidence | If it fails |
|---|---|---|---|
| G1: Host and lifecycle | Plugin load, game identity, simulation time, world loading/unloading and shutdown | A diagnostic plugin survives repeated loads, menu transitions and pause/speed changes without terrain writes | Fix lifecycle support before any mutation |
| G2: Terrain access | Coordinates, grid spacing, elevation units, bounds, height sampling and native edit functions | A small dry-land patch changes predictably, respects its boundary and remains correct after save/reload | Investigate a lower-level supported native edit path; do not ship raw height-buffer writes |
| G3: Terrain consistency | Terrain mesh, collision, placement tests, terrain masks and any derived state | Visual height, cursor height, road/building placement and saved height agree | Identify and invoke missing native update steps |
| G4: Office capacity | Actual staff count/productivity, vehicle ownership, vehicle skills and availability | Observed readings follow workers arriving/leaving and vehicles being bought, moved or sold | Try the linked workplace/yard design below; no fictional staffing fallback |
| G5: Building behaviour | Native workplace plus owned machinery, and suppression of unrelated dispatch | Office employs workers, holds eligible vehicles and reserves them exclusively | Use a staffed workplace linked to an existing machinery yard for the initial release |
| G6: Persistence | Successful-save notifications, paths, load identity and consistent snapshots | Mid-project save/reload and Save As resume exactly the saved project state | Persistence remains a release blocker; never silently resume mismatched state |

Use disposable test saves for mutation experiments. G2 and G3 are not satisfied merely because a brush can be replayed. We need to establish that the chosen call can approach a target height without overshooting, charging unexpected native costs or affecting protected neighbouring cells.

**5. Earthworks office and equipment model**

Preferred design: a dedicated development building based on an existing native type, augmented by plugin behaviour. Its panel shows actual staffing, eligible machinery, operating radius, assigned projects and status. Prototype balancing values are 20 worker places, four machinery places and a 1,000-metre radius; these are proposed settings, not established engine limits.

A new visual building does not need a new engine type. Investigate a native workplace shell with a vehicle container, or a machinery-office shell whose worker reception can be supported safely. Preserve normal worker commuting to the office. Suppress unrelated production, construction assignments or dispatch only for positively identified earthworks offices.

Fallback if one building cannot safely support both roles: a native staffed workplace linked to a nearby native construction office or depot. The player selects the yard explicitly. Together they are presented as an earthworks operation, with the workforce read from the workplace and equipment from the yard. This retains actual workers and vehicles. It should be identified as a prototype compromise until the combined building is proven.

Eligible machines must be owned by the selected operation, of a supported earthmoving skill, and in a verified usable state. Exclude travelling, assigned elsewhere, sold, destroyed or otherwise unavailable vehicles. Start with configurable per-model capacity values if the native productivity field cannot be interpreted reliably. Identify skills rather than relying on English model names; allow explicit configuration for Workshop vehicles.

Use the equipment as a pool for the office's single active project. Release reservations on pause, cancellation, completion, reassignment, office demolition and world unload. Reservation must also prevent native/manual terraforming or other native jobs from simultaneously using these vehicles. Prove this interception or enforce a genuinely exclusive yard; an internal reservation flag alone is insufficient.

Count current effective workers, not nominal capacity, nearby population or workers employed elsewhere. Read native effective productivity where verified; otherwise use current staffed count with a documented initial rate model. Lost workers reduce the next simulation step's capacity.

**6. Planning data and terrain preview**

Store the plan in a separate, sparse grid aligned to the verified native terrain grid. Do not edit the real world and then roll it back to create a preview: those edits can affect more than visible height.

Each draft contains a project UUID, world identity, grid bounds, selected-cell mask, original heights, target heights, tool parameters, intended office and a revision counter. Include any transition band in the explicit footprint, volume calculation, overlap check and range check.

Planning sequence:

1. Open the tool and choose flatten, relative raise or relative lower.
2. Drag a rectangle; enter height/delta or sample a point on existing ground.
3. Calculate target heights, edge transitions, cut/fill volumes and invalid cells from a copied terrain snapshot.
4. Render a ghost surface, boundary and legend: cut, fill and invalid/protected regions. Display numeric current/target height under the cursor.
5. Allow undo/redo of draft edits. Apply revision checks so an older background preview cannot replace a newer one.
6. On confirmation, reread the affected terrain and protection state. Require a new preview if the draft has become stale.
7. Freeze the target into a queued project; closing the UI does not cancel it.

Use the game's actual triangulation or verified cell-area weights when estimating volume. Height samples at mesh vertices must not each be charged as a complete terrain cell. Until numerical integration is calibrated, label volumes and completion times as estimates.

For the first release, reject overlapping project influence areas. Reject footprints that cross the selected office's radius, including their transition band; the farthest affected point determines eligibility. Apply a simple horizontal radius with no route-to-site requirement, consistent with remote work.

Start preview rendering with wireframe/contours if a translucent surface is not yet practical. Validate preview picking and occlusion at different zoom levels and display scaling before adding visual polish.

**7. Project states and player controls**

Use a persistent lifecycle state plus a derived wait reason. Suggested states: Draft, Queued, Running, Paused, Completed and Cancelled. Suggested wait reasons: NoOffice, OfficeUnavailable, NoWorkers, NoMachinery, OutOfRange, Obstruction, TerrainChanged and IncompatibleSave.

Confirmation moves Draft to Queued. The scheduler starts the highest-priority eligible project for an office. A missing prerequisite leaves the project waiting without progress. A user pause is distinct from a temporary lack of workers, so restoring staffing cannot override a deliberate pause.

Allow rename, priority change, reassignment, pause/resume and cancellation. A project already in progress cannot have its target silently edited; create a revised draft from current ground after pausing or cancelling it. Completed and cancelled records may be retained as compact history.

Cancelling an untouched draft removes it. Cancelling an active project stops future changes and leaves completed earthworks in place. Returning to the original terrain is a new reverse project with its own work requirement and validation, not a free undo.

For a demolished office, release machinery, retain the remaining project and show NoOffice. Do not transfer it automatically to a different operation unless the player has enabled auto-assignment. A replacement building at the same coordinates is not automatically the same office.

**8. Work calculation and gradual execution**

Express rates in cubic metres per simulated hour after verifying what the game's simulation-time source represents. Do not use rendering frame rate or wall-clock time as the production clock.

Initial capacity model:

```text
worker_capacity  = effective_workers * configured_volume_per_worker_hour
machine_capacity = sum(capacity_of_each_reserved_usable_machine)
office_rate      = min(worker_capacity, machine_capacity)
work_budget      = office_rate * elapsed_simulation_hours
```

Require both a positive workforce and at least one eligible machine. Give excavators and bulldozers simple configurable initial rates; operation-specific cut/fill bonuses can follow after basic balancing. Prototype rates must be measured against ordinary terraforming before becoming defaults.

Execution at a safe simulation boundary:

1. Verify world generation, project assignment and office identity.
2. Refresh staff and equipment, acquire reservations, and calculate this step's budget.
3. Recheck the candidate edit's footprint for obstructions and terrain changes.
4. Select a bounded batch of incomplete cells in a stable spatial order, using shallow passes to avoid temporary spikes or deep pits.
5. Ask the terrain adapter for a validated native edit that moves toward the stored target without exceeding its influence area.
6. Read back the affected heights. Charge only measured successful movement against work, and update expected heights and progress.
7. Refresh derived native terrain state as required. Publish a UI snapshot and complete the project only when the validated height error is within tolerance.

Choose the completion tolerance after measuring native height quantization and brush convergence; 5 cm is an initial experiment, not a guaranteed engine capability. Detect no-progress and oscillation rather than running indefinitely.

Prefer bounded native height-setting operations if they exist and maintain all dependent state. If only brushes are viable, implement a feedback controller with verified brush footprint and influence strength. Reject this path if it cannot prevent collateral changes or reliably converge to the preview. Never compensate for an unsafe brush by writing raw heights behind the engine's back.

Bound work per callback by edited samples and dirty terrain regions. Carry only justified, capped work backlog; never bank work during a pause or while staffing/equipment are unavailable. A frame hitch must not create a large, unsafe terrain jump. Native money charges and native machine participation must be characterized so the plugin does not accidentally charge twice or borrow unreserved equipment.

**9. Protection, conflicting edits and performance**

Before confirmation and before each edit batch, validate buildings, road/rail paths, utilities, bridges, tunnels and other occupied ground using verified native queries and conservative exclusion bounds. Inspect below-ground as well as visible structures. Include the native edit's full influence radius, not just its centre.

First-release work is limited to dry, unoccupied land. Exclude water and a measured shoreline buffer, including proposed heights that would cross the water surface. If a required occupancy category cannot be queried reliably, restrict the supported work area and describe the restriction rather than claiming full protection.

Track the expected current height of each affected sample. If the player manually terraforms there, another mod edits it, or construction changes it, pause the project with TerrainChanged. Offer explicit re-planning from current terrain. Do not restore old ground automatically or silently overwrite external changes. Revalidate occupancy even when heights have not changed.

Proposed performance targets, to calibrate during profiling:

- Planning and closing the UI produce zero native terrain writes.
- One medium office project plus ten queued plans adds under 2 ms average CPU time per displayed frame on the test machine, measured at normal and maximum supported game speed.
- Limit each native-edit callback to a calibrated sample/region budget; instrument long calls and report the 95th and 99th percentiles.
- Store only affected patches, with initial configurable limits on project area and total planned samples.
- Cache office snapshots and dirty-region validation results with explicit invalidation. Do not scan the entire map every frame.

**10. Persistence and world lifecycle**

Integrate save/load during the first execution prototype, before long-running projects become normal gameplay. Native terrain must remain in the native save. Store mod metadata in a versioned companion file, ideally in the same save directory if the game permits it. Its exact path and inclusion in compressed saves must be established during G6.

Persist schema/plugin version, world/save identity, snapshot generation, projects and their targets, expected current heights, completed-volume counters, queue order, user pause state, office handles and settings needed to interpret progress. Do not persist pointers or trust old reservations; reconstruct reservations after load from current validated vehicles.

Capture native and plugin state at the same safe simulation boundary. Establish when the game's save snapshot is actually taken; a notification that saving began or finished is not enough for asynchronous saves. Stage the companion snapshot and publish it only for a successful matching native save. Use atomic file replacement and retain a previous matching snapshot where possible.

Tie companion data to a fingerprint of the native saved terrain payload or another verified snapshot identity, plus the world identity. A directory name or world UUID alone cannot distinguish an older autosave from a later branch of that world. Save As must publish to the new slot; copying or renaming a complete save should retain its association.

On load, discard all old engine references, advance the world-generation token, verify snapshot identity, resolve offices and vehicles, revalidate affected terrain and reconstruct the queue. Missing or mismatched metadata must never trigger automatic earthworks. Corrupt data is quarantined without rewriting the terrain. Missing offices leave projects waiting for reassignment.

Test Steam Cloud separately: arbitrary companion files may not be synchronized. Do not advertise cloud support until verified. If unsupported, provide clear local-save behaviour and an export/import bundle containing the native save and matching companion file.

Loading with the plugin disabled leaves already changed native terrain as saved; remaining work cannot execute. Keep required office assets installed while they are referenced by a save, and verify this scenario rather than promising that deleting the whole package is harmless. Provide explicit stop/export/uninstall instructions for active projects.

**11. Delivery milestones and completion criteria**

| Milestone | Deliverable | Completion criteria |
|---|---|---|
| M0 — Baseline | Reproducible x64 build, pinned SDK/host, hashes, disposable test save, diagnostics | Loads on the supported build, declines incompatible builds before writes, survives unload/load cycles |
| M1 — Terrain experiment | Small bounded edit toward an explicit target, with readback | Terrain, placement/collision and save/reload agree; no change outside the validated influence area |
| M2 — Capacity experiment | Actual workers, owned vehicles, exclusive machinery reservation and office-shell decision | Arrival/departure/purchase/sale affect readings correctly; no double use; G4/G5 pass |
| M3 — First vertical slice | Rectangle, preview, confirm, gradual execution, basic save/load | A staffed operation finishes a real project while the player uses other tools; pause and reload work |
| M4 — Office workflow | Dedicated office or explicit linked workplace/yard, radius, queue and status panel | Multiple offices cannot double-count machinery; priorities and reassignment work; office demolition is handled |
| M5 — Release candidate | Full save lifecycle, protection rules, brush if validated, performance limits, packaging | Functional test matrix passes on the supported game/host combination; known limitations documented |
| M6 — Public release | Versioned package, source, installation guide, compatibility statement and example save | Clean installation and upgrade tested; no original game configuration or binary files overwritten |

Dependencies: M0 precedes all native integration; M1 and M2 are independent investigations after M0; both must pass before M3 is treated as feasible gameplay. M3 includes persistence rather than postponing it to the end. M4/M5 build on that working slice. These are work packages, not instructions to delegate to other agents.

Do not commit to a calendar estimate before M1 and M2. Most uncertainty lies in native terrain correctness and the combined staffing/equipment model, not in queue or UI code. At those checkpoints, record the discovered interfaces, remaining blockers and revised effort estimate.

**12. Verification matrix**

Use focused automated tests for the durable core: triangulated cut/fill integration, radius checks, target interpolation, queue ordering, state transitions, capacity bounds, serialization round trips and schema migration. These test meaningful behaviour and stay independent of engine offsets.

Use in-game integration tests for native behaviour:

| Scenario | Expected result |
|---|---|
| Create/edit/cancel draft | Actual terrain and native save heights remain unchanged |
| Flatten, raise and lower an empty dry patch | Matches the preview within calibrated tolerance; outside footprint remains unchanged |
| Player moves camera and builds elsewhere | Project continues without held mouse input or an open mod panel |
| Game pause, speed changes, frame-rate differences and a long hitch | Work follows simulation time; no pause progress or unbounded catch-up |
| Workers reach zero or vehicle becomes unavailable | Work stops at the next safe update, then resumes only when eligible |
| Two projects and two offices share an eligible yard | Exclusive assignment prevents duplicated capacity |
| Office demolished, vehicle sold, new building reuses an index | No stale pointer use or accidental reassignment |
| Project intersects structures, shoreline, map edge or another plan | Confirmation is blocked or active work pauses before unsafe edits |
| Manual terrain edit or new construction inside the influence area | Project pauses with a useful conflict reason |
| Save mid-project, autosave, Save As, reload an older save, copy a save | Matching project state restored; future branch state never leaks backward |
| Companion data missing/corrupt, failed native save or failed companion write | No mismatched automatic resume; recoverable diagnostic and preserved native terrain |
| Plugin disabled, re-enabled, or updated | Remaining work resumes only after compatible metadata and assets are validated |
| Unsupported game hash or competing hook owner | Feature declines activation clearly instead of applying guessed bindings |
| Maximum supported project area and queue size | Meets measured time/memory budgets without long game stalls |

Release acceptance: the player can plan two earthworks projects, assign a staffed and equipped operation, continue playing elsewhere, observe gradual correct work, interrupt staffing and see work stop, save/reload, and finish both projects without any terrain changes outside their validated footprints.

**13. Initial implementation backlog**

1. Create the build skeleton, logging, configuration and exact-build binding registry.
2. Pin and document the host/SDK; compile and load a read-only diagnostic DLL.
3. Establish world-generation, simulation-time and native-save snapshot callbacks.
4. Verify terrain coordinates, height sampling, native editing and derived-state updates.
5. Read current staff, vehicle ownership, equipment skills and availability; prove exclusive use.
6. Decide and document the single-building or linked-building office shell from observed results.
7. Implement pure planning geometry, volumes, project states and capacity calculations.
8. Add rectangle selection and a separate terrain preview, then confirmation and validation.
9. Implement bounded simulation-time execution with readback and conflict detection.
10. Complete matched save/load for the first working project before expanding the interface.
11. Add queues, operating radius, reassignment, office controls and status explanations.
12. Complete protection and performance tests, then package and document the supported release.

The first concrete checkpoint is a diagnostic plugin that demonstrates safe target-driven terrain editing and reliable readings of real staffing and machinery. Those observations determine whether the final office can be one native building or needs the explicitly linked workplace/yard design.
