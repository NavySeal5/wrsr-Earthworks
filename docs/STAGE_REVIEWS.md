**Stage progress and in-game review log**

Updated 2 November 2026. A stage is accepted only when the user confirms the demonstrated result. Technical verification and user acceptance are separate fields.

**Current position**

The first planning prototype, Stage 3 controlled terrain experiment, Stage 4a comparison, Stage 4b dedicated-facility checklist and Stage 4d vehicle allowlist are accepted by the user. Stage 4a fuel remains unmeasured. Stage 4c deferred the rail-office alternative; the regular construction-office prototype remains selected. The dedicated facility blocks ordinary jobs on the verified direct assignment route. D31 sets the productivity direction; native rate calibration, multi-machine allocation, machine availability/reservation and fuel remain open. See [Stage 4 remaining feasibility research](STAGE4_RESEARCH.md).

Latest workflow correction: the user said "Dont do the tests yourself, just provide me with test instructions". All further in-game testing is user-run. Do not resume agent computer-use/game tests. Stage 3's existing agent observations remain historical evidence. Version 0.4.0-worker-probe is compiled; the launch script stages it after the game and project-local RML are closed. The Stage 4 in-game checklist is in STAGE4_TESTING.md. No in-game check of 0.4.0 has been performed by the agent.

Stage 4's accepted productivity direction is recorded as D31. The user has now authorized D32's provisional rate scale: 22 worker-equivalent points define one manual-rate unit; 10 workers fully staff one machine; partial staffing scales linearly; workers do not add separate output while operating machinery. This is a relative starting model only; no volume/time estimate is claimed. The actual worker-only and machine-assisted native comparison remains unisolated. Machine availability/reservation, allocation across multiple machines and fuel remain unresolved. The user identified the relevant native worker source as the Labour Hall/worker pickup hall. See [STAGE4_RESEARCH.md](STAGE4_RESEARCH.md).

**Stage 4e user observation — native terrain productivity**

- User reports one YAG DA-0.25 excavator present during 10 seconds of native terrain raising with the default brush. One sampled point changed from 11.66 m to about 16.35 m (about +4.69 m).
- User could not isolate 10 workers; the operation appeared to draw workers from across the city. The measurement is not attributable to a known worker count and does not establish area/volume throughput or autonomous Earthworks productivity.
- Worker-only calibration remains open. A proposed isolated low-population comparison is appended to [the Stage 4e instructions](STAGE4E_PRODUCTIVITY_TESTING.md).

| Stage | Status | Technical evidence | In-game/user acceptance |
|---|---|---|---|
| Preparation | Complete | Toolchain installed, DLL built/loaded outside game, RML discovery inspected | Preparation is not an in-game stage acceptance |
| 1 — RML in-game diagnostic | Accepted | Original diagnostic evidence below; subsequent prototype launch/world/reload checklist passed | Covered by user's accepted prototype review |
| 2 — Read-only diagnostics | In progress | Frame snapshots, lifecycle gating, cursor/camera projection and terrain sampling implemented for the accepted preview; broader save-lifecycle/time-speed investigation remains | Preview subset accepted; full Stage 2 checklist not claimed complete |
| 3 — Controlled terrain edit | Accepted | Bounded native raise/lower, readbacks and historical saved-file comparisons; 0.3.1 diagnostic UI fixes | User reports raise/lower works, both deltas zero and normal native road/building placement |
| 4 — Workforce/machinery feasibility | In progress | 0.5.2 vehicle allowlist accepted; D31 direction and D32 provisional relative rate selected | User: "working great now" after the 0.5.2 correction. The initial formula is documented; the unisolated native sample still does not validate it. Multi-machine allocation, availability/reservation and fuel remain open |
| 5 — Planning preview | Accepted | 0.2.0-prototype; build/staged/reported DLL hashes match; RML active, 16 tracked hooks, zero hook failures | User: "All checks passed" |
| 6 — Durable plans | In progress | Versioned companion-state encoding, atomic file replacement, native save-trigger observation and saved-heightmap SHA-256 matching implemented; draft restored on a match | User confirms ordinary save/reload preserves the draft and queued project. Stale-state protection was not checked and is deferred; Save As/autosave checks also remain unreported. See [STAGE6_TESTING.md](STAGE6_TESTING.md) |
| 7 — Office/assignments | Planned | Representation and rules open | Pending |
| 8 — One autonomous project | Planned | Not implemented | Pending |
| 9 — Multiple projects/operations | Planned | Scheduling rules open | Pending |
| 10 — Balanced candidate | Planned | Scope/balance open | Pending |

**Stage 6 implementation — awaiting user in-game review**

- User chose to preserve both drafts and confirmed projects across save/reload (D33).
- Added versioned companion-state encoding with CRC validation, bounded decode, a SHA-256 fingerprint of the matching save's `heightmap.dds`, and atomic replacement at `media_soviet/save/<slot>/earthworks.dat`. The runtime observes the native save trigger through the existing Late hook and restores only after the loaded save's fingerprint matches.
- Added a queue-only confirmation control. Confirmation moves the current draft into the queue; queued work remains visible as a grey wireframe while a new draft can be planned in cut/fill colors. Stage 6 never executes terrain work. Scheduling and autonomous work remain later stages.
- Missing companion data leaves the screen empty. Corrupt or terrain-mismatched metadata is rejected and shown as a status message; the native save and terrain are not changed. The user chose warning-and-discard with no import/recovery flow for now (D34).
- Build: `0.6.1-queued-wireframe`; x64 build succeeded and was staged in the project-local RML runtime. Built and staged DLL SHA-256: `7511A8054DBDB8AFF66C08151937F09E68CE476A8348A3E6250ECFDFA9380903`. No game or automated tests were run by the agent. User checklist: [STAGE6_TESTING.md](STAGE6_TESTING.md).
- User-run result: ordinary save and reload preserved both the editable draft and queued project. The user has not yet checked stale-state protection and will do so later. Save As/autosave outcomes have not been reported, so Stage 6 remains in progress.

Allowed stage statuses: Planned, In progress, Ready for in-game test, Awaiting user review, Changes requested, Accepted. Record a failed technical check explicitly rather than promoting the stage. A stage can be split if feedback would otherwise come too late.

**Stage 1 review packet — original diagnostic evidence**

- Plugin version: 0.1.0-dev, setup diagnostic only.
- Built DLL: build/windows-x64/bin/EarthworksPlanning.dll.
- SHA-256: 1ee4382a6d79705e3632fae86abb8b9189bc893528785d8edd3790ebebcb0497.
- Host: pinned RML 1.0.1 using its Tesmio API 4 compatibility layer.
- Game: build 23935965, detected as 1.1.1.9; full binary identity is in game-baseline.json.
- Profile: Earthworks Development, one diagnostic plugin enabled, zero Workshop items enabled.
- Scope: plugin initialization, main menu, disposable world entry/re-entry and normal exit. No terrain/simulation feature hooks.
- Save fixture: fresh custom game on A New Republic Is Born, Easy preset with 1970 start, saved by the user as Earthworks_Test_S01. Native folder: media_soviet/save/4504 - Earthworks_Test_S01; saved game date 5 January 1970. This fixture is for diagnostic testing, not a gameplay-balance decision.

Test procedure once the user starts Stage 1:

1. Confirm the staged diagnostic DLL still matches this packet. Close the development RML window before restaging if a new build is needed.
2. Open the project-local launcher via launch-diagnostics.cmd and confirm the named playlist and single enabled diagnostic.
3. Launch from RML and reach the main menu.
4. Enter the agreed disposable test world. Check ordinary camera/menu interaction.
5. Return to the menu and reload that test world; observe any crashes or unexpected behaviour.
6. Exit normally. Inspect the RML runtime report and log for successful plugin initialization/start and any errors.
7. Summarize actual results and ask for the user's assessment before Stage 2.

Expected diagnostic messages, possibly with an RML prefix:

    earthworks  setup diagnostic initialized; API 4, supported PE timestamp
    earthworks  setup complete: no terrain hooks, simulation hooks or gameplay changes installed

Evidence locations after execution:

- tools/runtime/rml/rml/logs/rml-runtime.log
- tools/runtime/rml/rml/state/last_runtime_report.json

Observed results, 1 November 2026:

| Check | Observed result |
|---|---|
| Pinned artifact/game | Staged DLL matches the recorded build; original executable and engine hashes match the baseline before and after the test |
| RML bootstrap | Bootstrap status 0; TsmPluginInit and TsmPluginStart returned 0; plugin reported TESMIO ACTIVE through API 4 |
| Initial menu/world entry | Main menu and fresh A New Republic Is Born custom game opened successfully |
| Ordinary interaction | Mouse camera zoom changed the view; native Resource Status and Settings panels opened |
| Save fixture | User completed Save As to Earthworks_Test_S01 and confirmed it; the native save folder exists |
| Return/reload | Returned to the full main menu, selected the named save dated 5 January 1970, reloaded it, and observed the world and Settings panel functioning |
| Normal exit | Returned to the main menu again, clicked Quit, observed the game window close; RML recorded game PID 27596 exiting with code 0x0 |
| Runtime diagnostics | Zero plugin gameplay hooks, zero hook failures, zero unmanaged init/start code changes, zero late-change windows and zero audit issues in the saved runtime report |

The session ran from approximately 18:00:52 to 18:11:09 Europe/Vienna; RML recorded 10m16.953s. This includes native asset loading and manual interaction, not a gameplay-performance benchmark. No terrain or staffing feature implementation was added in this stage.

Automation limitation: game captures were strongly overexposed, making the Save As input unreliable; the user supplied that step. Escape key injection did not visibly close native panels, so keyboard controls were not certified. Mouse-based menu navigation worked. These are recorded automation limitations, not established game/mod defects.

Session ID: 20261101T170052.326696900Z-31716. Final report/log and successful clean-exit verification are preserved in [the session evidence folder](evidence/stage-01/20261101T170052.326696900Z-31716/verification.json). The session-ID suffix identifies a launcher context; the actual game PID is taken from the session's process-creation log entry.

Added Verify-DiagnosticSession.ps1 to validate the specific session's report, plugin hash, pinned game/runtime, inactive gameplay hooks, audit state and optional clean exit. It cannot accept a stage or infer world transitions from a bootstrap report.

Verification command executed successfully:

    .\scripts\Verify-DiagnosticSession.ps1 -SessionId 20261101T170052.326696900Z-31716 -RequireCleanExit

The script's initial PID assumption was corrected using RML's actual process-creation/exit records, then the corrected script was run against this completed session. The plugin DLL itself was unchanged; no rebuild was needed. Stage 2 was not started.

Original diagnostic review was superseded by the user's batch instruction through the first planning prototype and subsequent acceptance of its launch/world/reload checklist. This does not establish completion of all later diagnostic experiments.

**Stage 5 review packet — 0.2.0-prototype accepted, 1 November 2026**

- User authorization: "Continue until the first prototype of the terraforming planning is finished"; D03/D17/D18 define the prototype's interaction.
- User acceptance: "All checks passed, Whats next on the plan" after the supplied manual checklist. These are user-reported results; the final checklist was not silently substituted with agent-only testing.
- DLL SHA-256: 8da0b348cb5ce0653a21811ee8cf66c2681a7f8b7fb9b999d131fe5bc9087e34. The compiled DLL, staged DLL and RML session report all match.
- RML session: 20261101T194757.114269700Z-16628. Bootstrap status 0; plugin 0.2.0-prototype TESMIO ACTIVE through API 4; 16 tracked hooks (2 inline, 14 IAT), zero hook failures. The directly patched COM Present slot is additional to the tracked count. Latest inspected heartbeat reports zero audit anomalies. The log subsequently recorded game PID 34428 exiting with code 0x0. Report/log and acceptance evidence are preserved in docs/evidence/stage-05/20261101T194757.114269700Z-16628/.
- Agent checks before handoff: MSVC x64 /W4 /WX build; geometry checks for reversed corners, flat/sloped/mixed cut-fill volumes and undo/redo/reset; in-game area selection, entered target, sampled height, cut/fill shading, undo/redo, zoom, clearing, paused planning and rejection/recovery of a zero-area selection.
- User checklist reported passed: latest build launch in Earthworks Development; Earthworks_Test_S01 load; on-screen button; two-corner selection; entered/sampled height; cut/fill display; undo/redo/clear; camera alignment on zoom; normal native controls after closing; paused planning; no preview/button on the main menu; button returns and old draft is cleared on reload; actual ground remains unchanged.
- Scope: read-only flat rectangular preview with sharp edges and sampled volume estimates. Session-only drafts, no execution/office/persistence. No native terrain edit export or heightmap write path is bound.
- Readback diagnostic samples the old footprint after draft commands and logs maximum native height difference. Inspected final-session entries report 625 samples with maxDelta=0.000000; the log also records a new world generation and successful world readiness on reload. This is supporting evidence, not proof that every native terrain sample or future edit path has been validated.
- Iteration history: early temporary-swap-chain graphics bootstrap crashed through Steam's overlay chain; observing actual game device creation resolved startup in the agent's gameplay session. Agent then found a stale overlay on returning to the main menu. The final build gates copied snapshots on fixed-address world availability signals; final menu/reload checks passed according to the user.
- Stage 3 terrain mutation and Stage 4 staffing/machinery experiments were deferred to reach this read-only preview, not marked complete. Stage 2's broader save-lifecycle and simulation-speed checks also remain unclaimed.
- Next proposed stage: controlled terrain-edit proof in the disposable world, after agreeing the exact temporary operation. No workforce/range/cost/rate/cancel/persistence choices are inferred from accepting this preview.

**Stage 3 review — accepted from user report, 1 November 2026**

- User first reported raising/lowering works, then clarified that reload/placement remained untested. Progression was held at that gate.
- Remaining checks subsequently completed by the user: "Both deltas are 0, road planning & building over it felt just like normal". Recorded as zero guard/reload deltas and normal native road/building placement in the disposable fixture, not independent agent gameplay verification.
- 0.3.1-terrain-test built DLL SHA-256: `a23939f539b7dd25e454b05cdc477e096c0946ee3cf9b4e6d49b53edf81875b1`. DLL/PDB preserved under tools/releases/0.3.1-terrain-test before rebuilding. This identifies the delivered build; no new runtime report has been used to claim the exact user's session hash.
- Scope accepted: controlled small raise/lower, guard/reload readback and native placement. This does not approve production terrain protection, edge blending, rates or accounting.
- Prior "Seems to work, next step now" plus completion of the remaining checks authorizes Stage 4's feasibility diagnostic.

**Stage 4a review — 0.4.0-worker-probe, user confirms checks except vehicle fuel**

- DLL SHA-256: `af750fe923e6e2cc4a4a81ce4ce31c8369830ad72371f09d323a1a871ce51730`.
- Host/game unchanged: pinned RML 1.0.1, Tesmio API 4 compatibility, supported game 1.1.1.9. No additional hooks for this inspector.
- Added a read-only inspector using a click and explicit choice among nearby building origins. It copies occupant-list entries, parked membership and vehicle home/assignment links, raw types/categories and horizontal distance. A magenta origin marker assists identity checking. Live reads stay on the game thread; the renderer receives copied values only.
- Sampling continues while Earthworks is closed, so the user can use native controls between comparisons. World load clears selection, and unresolved building identities require reselection. Invalid lists/partial vehicle scans are distinguished from zero counts. No office, productivity formula, eligibility rule or reservation is implemented.
- Technical checks actually run: MSVC x64 /W4 /WX build and AMD64/Tesmio export checks passed. Synthetic-memory executable workforce_checks passed: candidate lookup, null occupant entries, overlapping vehicle links, a vehicle linked to home while away, malformed list rejection, unreadable vehicle handling, ID mismatch invalidation and world-boundary reset. This executable did not open or attach to the game.
- In-game checks: **not run by the agent**. User reports all checklist checks passed except finding a vehicle fuel reading. Numeric readings were not supplied. No values are inferred. User checklist: [STAGE4_TESTING.md](STAGE4_TESTING.md).
- Read-only community-source scan of the pinned RepublicMP building/vehicle modules found no reliable fuel-field reference; incidental uses of the word "consumed" concern unrelated event handling. Fuel remains unavailable in this probe, with no claim that vehicles lack fuel or that the native game lacks a display elsewhere.
- Stage 4a comparison accepted from the user's report. Full Stage 4 remains active: effective productivity/eligibility and exclusive equipment reservation need evidence; native/manual competition remains a blocker for autonomous execution.
- Vehicle fuel costs or modelling remain a gameplay decision under D09; this report does not settle that choice.

**Stage 4b source review — native vehicle availability and reservation**

- User authorized continuing implementation after accepting the Stage 4a comparison, except fuel was not found.
- Pinned RepublicMP route_tools.cpp documents native depot dispatch as a sequence of checks: a vehicle compatibility predicate at EXE+0x68B2B0, depot/type compatibility calls, current owner/home relation, route/home status, and other category-specific state. This demonstrates that home/owner/parked readings alone do not identify a universally available machine.
- The referenced +0x68B2B0 predicate is a pure type/capability check in this executable, but it is only one filter used by a native depot workflow. It does not establish Earthworks eligibility. Native dispatch also changes vehicle routing/assignment state; no generic read-only reservation API was identified in the pinned source.
- This source review did not establish a general reservation mechanism or resolve machine eligibility. The user's later D25/D26 decisions selected a dedicated facility and blocking ordinary construction jobs. No native vehicle fields are changed and no dispatch predicate is called by the mod.

**Dedicated building decision — D25**

- User: "Earthworks should not use vehicles in construction offices, there should be its own building with worker slots + machine slots".
- Decision recorded: a dedicated building with its own worker and machine slots; do not borrow vehicles from ordinary construction offices.
- Temporary prototype capacity: 5 worker places and 2 vehicle stations, selected by the user for this test item only. This is not a release balance decision.
- User resolved dispatch behavior: "Block normal jobs (Recommended)". Record as D26. This build offers session-only registration of one exact building identity, then blocks that identity in the native assignment function at EXE+0x161C90.
- At implementation time, assignment coverage required user verification. The pinned RepublicMP call map shows construction-office window, pick-tool and area-office direct assignments reaching this function. The office window's automatic “add construction” operation uses EXE+0x1561C0 and is not directly intercepted. The user later reported no Stage 4b checks outstanding; individual route outcomes were not itemized.

**Stage 4b implementation packet — 0.5.0-dedicated-facility, 1 November 2026**

- Build: CMake/MSVC x64 build completed successfully; x64 and the three Tesmio entry points were verified by the build script. No tests or game launch were performed by the agent.
- Plugin DLL SHA-256: `1f56bcb07c9b92b8517290690fa8da1ad1ed34b792de052a7ea1415af8bb5bcf`.
- Building content: generated locally as Workshop item `9100000012`, using native `construction_office3` assets/configuration as a temporary shell. It has 5 worker places and two vehicle-station placements. The item is marked generated and is not part of the Git commit; the checked-in staging script recreates it from installed game assets.
- Runtime feature: inspector can register or clear one building per loaded session; the assignment hook checks exact instance address plus world vector slot and building ID before blocking. World load clears registration. Assignments to other offices forward to the original function.
- User's gameplay test has not yet been run. Instructions: [STAGE4B_TESTING.md](STAGE4B_TESTING.md). Test status remains pending, including station slot behavior, direct assignment, area assignment, automatic add-construction workflow, and unchanged ordinary-office behavior.
- Limitations: no autonomous terrain work, project queue, multiple/persistent stations, release building art or balance. The automatic add-construction routine at EXE+0x1561C0 is outside the direct hook; its behavior must be observed in game before ordinary-job exclusion can be considered complete.

**Stage 4b user report and acceptance — worker display discrepancy**

- User reports: "Can confirm nearly everything, the building doesnt show worker occupation in its vanilla UI, but i see workers running to it, and the earthworks ui shows workers there".
- Recorded observation: workers were seen travelling to the dedicated facility and the Earthworks inspector reported workers there. The vanilla building panel did not display worker occupation. This supports that workers are assigned/present, while the conflicting UI display remains unexplained; it does not by itself establish native slot limits or productivity.
- The user subsequently confirmed "no checks outstanding". Stage 4b's checklist is therefore accepted as complete. Individual route outcomes were not itemized in the conversation, so this record does not claim specific counts or outcomes beyond the worker observation above.
- The vanilla worker-display discrepancy remains documented for possible follow-up. It did not leave a Stage 4b check outstanding according to the user.

**Stage 4c source review — native staffed-office precedent**

- Installed native configuration: `construction_office3.ini` declares `$TYPE_CONSTRUCTION_OFFICE` and `$RESOURCE_SOURCE_WORKERS`; it does not define `$WORKERS_NEEDED`. The 0.5.0 staging script adds a temporary `$WORKERS_NEEDED 5`. The user's observations establish worker arrivals and non-null occupant-list readings, but the vanilla panel does not display occupation. The precise meaning/effect of those occupants at this regular construction-office type remains unverified.
- Installed native `rail_construction_office.ini` declares `$TYPE_CONSTRUCTION_OFFICE_RAIL`, `$WORKERS_NEEDED 60` and three vehicle stations; the large office declares 170 worker places. This is a genuinely staffed native office type, not simply a regular construction office with extra slots.
- The official game wiki describes rail-office workers as reporting to the office, operating track builders, and working on assigned rail construction; it describes the rail office as having internal stores for its construction materials. The official construction reference defines workdays as labor and distinguishes vehicle-produced work from worker labor. These are rail-building semantics and do not establish Earthworks machine eligibility or a general work-rate formula. Sources: [Rail construction office](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Rail_construction_office), [Construction](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Construction).
- Community precedent: the Manual Rail Laying plugin's Steam description reports office workers building assigned track sections without traveling, sharing labor across sections, and consuming stored materials; a track builder adds work. The Steam item page currently marks the item removed/incompatible, and its implementation source was not available locally, so this is a community-described precedent rather than verified source code or a dependency: [Steam Workshop item 3818624868](https://steamcommunity.com/sharedfiles/filedetails/?id=3818624868).
- The public WRSR Industrial Tycoon technical guide documents other RML plugins that substitute worker productivity in native production/building routines, and stresses that synthetic tests do not establish in-game rates or resource behavior: [technical guide](https://github.com/paladin2005/WRSR-Industrial-Tycoon/blob/main/guide.md). This supports the technical feasibility of narrow, version-checked work-rate hooks, not compatibility with the Earthworks building or a chosen rate.
- Feasibility conclusion: native worker-at-office/no-site-travel mechanics exist in the rail-construction subsystem, and community plugins have modified native labor routines. The current regular-office prototype has not demonstrated productive Earthworks work, and a rail-office type brings rail-specific native vehicle/material/assignment behavior. No productivity, machine, material, fuel, or reservation rules are inferred.
- The user initially requested an RCO-type comparison, then superseded that request with D29: "lets just stick with the construcction office prototype for now, the RCO seems overkill". The separate comparison was staged locally but never launched or tested. Its generated game-content directory, staging script, and test checklist have been removed; the research above remains as background only. The regular construction-office prototype is still the active test building.

**Stage 4d vehicle allowlist decision — D30**

- User decision: "The building should only accept Bulldozers, excavators, dumpers, and busses". Normalize spelling to buses; accepted classes are exclusive, with all other vehicle classes excluded.
- Initial source inspection: the native `construction_office3.ini` expresses the construction-office type and vehicle-station positions, with no evident per-building vehicle-class allowlist directive. The temporary Earthworks building inherits this generic type. Installed vehicle scripts expose bulldozer and excavator work skills and a dumper deck directive, while buses appear as a broad family of vehicle scripts. These are candidate classification signals, not yet proof that the native office's purchase or slot assignment code uses the same classification.
- The official construction-office reference documents the standard office's broad construction vehicle purchasing role; it does not document a configurable per-office whitelist. [Official construction office reference](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Construction_office). The official Q&A lists buses and dump trucks among construction vehicle needs generally, but does not specify an office allowlist. [Official Q&A](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Questions_and_answers).
- Implementation packet: build **0.5.1-vehicle-allowlist** hooks the pinned executable's vehicle slot selector at EXE+0x3E3F40 after checking the exact prologue. The hook filters only the exact session-registered Earthworks building and forwards other buildings unchanged. A conservative internal-name/category classifier permits bulldozer/buldozer and excavator/exvacator names in road-service category 2, plus dumper/dump_truck/dumptruck/gravel_ and bus/bust names in road category 1. Unreadable, unmatched or other-category types fail closed. The native route is used by the pinned purchase replay's depot-slot check; whether all in-game vehicle-transfer routes use it must be verified by the user.
- Limitation: the classifier does not inspect the vehicle's parsed `$SKILL_CONSTRUCTION_*` values or `$DUMPER_DECK_PIVOT` flag. Internal-name matching may miss mod vehicles with non-descriptive names and can classify name-matching types that lack the intended work skill. The official community vehicle guide describes skills and dumper animation as separate properties; this build uses names/categories only. No skill offsets are inferred.
- Technical verification: the default Release x64 build completed and the output DLL exported all three Tesmio entry points. The initial RelWithDebInfo preset failed in MSVC with C1902 (program database manager mismatch), so the preset now selects Release and the standard build script succeeds. No gameplay was launched and no tests were run. Release DLL SHA-256: `7511e8653b540223cb0b04a142cd7e571d6cb0d4a397ed1432e20db7bb0d1c8f`.
- In-game checks: pending user. Follow [Stage 4d vehicle allowlist instructions](STAGE4D_TESTING.md). The user must verify one example from each allowed class, at least one excluded type, existing vehicle transfer into the station, and unchanged behavior for an ordinary construction office. Treat D30 enforcement as unaccepted until those results are reported.
- Build-system note: the repository's RelWithDebInfo preset repeatedly failed before compilation with MSVC C1902 (program database manager mismatch), including on untouched translation units. The Release configuration built successfully; the preset now selects Release to make the project's default build reproducible. Existing tests were compiled as part of CMake's all target but were not run.

**Stage 4d first user test and correction — 0.5.1 → 0.5.2, 2 November 2026**

- User confirms the facility was registered. In build 0.5.1, disallowed vehicle types were refused, but allowed excavators and the ZiS-585 dumper were also refused. The user reports examples: all excavators, including EO-302B; dumper ZiS-585.
- The user's RML session log confirms native internal names were rejected: `service_exvacator_eo302b`, `service_exvacator_e10011`, DLC3 excavators and bulldozers, and `DLC3_zis150_gravel` (localized as ZiS-585). This is direct evidence that the allowlist was active and the name/category predicates were too restrictive.
- Root causes: the first filter required category 2 for bulldozers/excavators although these game types are road category 1; it only matched `gravel_` at the start of a type name, while the ZiS-585 identifier contains `gravel` at the end.
- Build 0.5.2 permits named earthmovers on road/road-service categories 1 or 2, and matches `gravel` anywhere within a road type name. Non-road categories and nonmatching names remain rejected. The user's next run is a focused retest, not stage acceptance.
- 0.5.2 Release x64 build succeeded and exported all three Tesmio entry points. No tests were run and the game was not launched by the agent. DLL SHA-256: `3db5d182e054a2d7b97e4160ff84d0f9a4bfc0b05ae62f9cbb5715658bad7627`.
- The installed ZiS-585 script lists availability as 1947–1963. In the user's new 1970 republic it may be unavailable due to the game's year rules even though the corrected allowlist recognizes its `DLC3_zis150_gravel` name. Use the IFA W50 gravel/dump truck for the focused 1970 test, or use a pre-1964 fixture to verify the ZiS-585 specifically.
- User acceptance: after the focused correction, the user reported "working great now". Stage 4d's vehicle allowlist is accepted. The exact checklist cases were not itemized, so this records acceptance of the observed filter behavior without claiming specific transfer-route or ordinary-office checks beyond what the user described.

**Template for each stage/test iteration**

- Stage and iteration:
- User instruction authorizing the stage:
- Resolved/open decision IDs:
- Feature changes and important limitations:
- Source revision/build identity and DLL hash:
- RML/game versions and fixture/settings:
- Technical checks actually run and results:
- In-game steps, each with expected and observed outcome:
- Logs/screenshots or other evidence:
- Performance observations where relevant:
- User feedback, including requested adjustments:
- Fixes and focused retest result:
- Acceptance: pending/accepted/changes requested, with the user's statement:
- Authorization to start the next stage:

A single user statement can accept the current result and authorize progression. Record it once; do not turn the process into repeated approval prompts. If the user requests changes, revise and retest the same stage first.
