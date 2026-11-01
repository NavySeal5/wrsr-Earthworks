**Stage progress and in-game review log**

Updated 1 November 2026. A stage is accepted only when the user confirms the demonstrated result. Technical verification and user acceptance are separate fields.

**Current position**

The first planning prototype and Stage 3 controlled terrain experiment are accepted. After initially checking only raising/lowering, the user completed the remaining checks and reported: "Both deltas are 0, road planning & building over it felt just like normal". This records zero reload/guard deltas and normal native placement as user-reported observations. Stage 4a's first workforce/vehicle comparison is confirmed by the user except for fuel: "Can confirm everything except vehicle fuel, did not find anything regarding that". The user reports no fuel reading was found. No office mechanics are approved.

Latest workflow correction: the user said "Dont do the tests yourself, just provide me with test instructions". All further in-game testing is user-run. Do not resume agent computer-use/game tests. Stage 3's existing agent observations remain historical evidence. Version 0.4.0-worker-probe is compiled; the launch script stages it after the game and project-local RML are closed. The Stage 4 in-game checklist is in STAGE4_TESTING.md. No in-game check of 0.4.0 has been performed by the agent.

| Stage | Status | Technical evidence | In-game/user acceptance |
|---|---|---|---|
| Preparation | Complete | Toolchain installed, DLL built/loaded outside game, RML discovery inspected | Preparation is not an in-game stage acceptance |
| 1 — RML in-game diagnostic | Accepted | Original diagnostic evidence below; subsequent prototype launch/world/reload checklist passed | Covered by user's accepted prototype review |
| 2 — Read-only diagnostics | In progress | Frame snapshots, lifecycle gating, cursor/camera projection and terrain sampling implemented for the accepted preview; broader save-lifecycle/time-speed investigation remains | Preview subset accepted; full Stage 2 checklist not claimed complete |
| 3 — Controlled terrain edit | Accepted | Bounded native raise/lower, readbacks and historical saved-file comparisons; 0.3.1 diagnostic UI fixes | User reports raise/lower works, both deltas zero and normal native road/building placement |
| 4 — Workforce/machinery feasibility | In progress | 0.5.0 custom Workshop facility, session registration, and assignment-function filter compiled | Stage 4a accepted. Stage 4b in-game test pending; ordinary assignment-hook coverage and native slots require user verification |
| 5 — Planning preview | Accepted | 0.2.0-prototype; build/staged/reported DLL hashes match; RML active, 16 tracked hooks, zero hook failures | User: "All checks passed" |
| 6 — Durable plans | Planned | Not implemented | Pending |
| 7 — Office/assignments | Planned | Representation and rules open | Pending |
| 8 — One autonomous project | Planned | Not implemented | Pending |
| 9 — Multiple projects/operations | Planned | Scheduling rules open | Pending |
| 10 — Balanced candidate | Planned | Scope/balance open | Pending |

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
- Coverage is pending user confirmation. The pinned RepublicMP call map shows construction-office window, pick-tool and area-office direct assignments reaching this function. The office window's automatic “add construction” operation uses EXE+0x1561C0 and is not directly intercepted. The test instructions ask the user to verify this route before treating the exclusion as complete.

**Stage 4b implementation packet — 0.5.0-dedicated-facility, 1 November 2026**

- Build: CMake/MSVC x64 build completed successfully; x64 and the three Tesmio entry points were verified by the build script. No tests or game launch were performed by the agent.
- Plugin DLL SHA-256: `1f56bcb07c9b92b8517290690fa8da1ad1ed34b792de052a7ea1415af8bb5bcf`.
- Building content: generated locally as Workshop item `9100000012`, using native `construction_office3` assets/configuration as a temporary shell. It has 5 worker places and two vehicle-station placements. The item is marked generated and is not part of the Git commit; the checked-in staging script recreates it from installed game assets.
- Runtime feature: inspector can register or clear one building per loaded session; the assignment hook checks exact instance address plus world vector slot and building ID before blocking. World load clears registration. Assignments to other offices forward to the original function.
- User's gameplay test has not yet been run. Instructions: [STAGE4B_TESTING.md](STAGE4B_TESTING.md). Test status remains pending, including station slot behavior, direct assignment, area assignment, automatic add-construction workflow, and unchanged ordinary-office behavior.
- Limitations: no autonomous terrain work, project queue, multiple/persistent stations, release building art or balance. The automatic add-construction routine at EXE+0x1561C0 is outside the direct hook; its behavior must be observed in game before ordinary-job exclusion can be considered complete.

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
