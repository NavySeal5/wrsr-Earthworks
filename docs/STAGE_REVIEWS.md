**Stage progress and in-game review log**

Updated 4 November 2026. A stage is accepted only when the user confirms the demonstrated result. Technical verification and user acceptance are separate fields.

**Current position**

The first planning prototype, Stage 3 controlled terrain experiment, Stage 4a comparison, Stage 4b dedicated-facility checklist, Stage 4d vehicle allowlist, Stage 6 persistence and Stage 7 facility assignment are accepted by the user. Stage 4a fuel remains unmeasured. Stage 4c deferred the rail-office alternative; the regular construction-office prototype remains selected. The dedicated facility blocks ordinary jobs on the verified direct assignment route. D31 sets the productivity direction; native rate calibration, multi-machine allocation, worker/machine non-duplication and reservation remain open. See [Stage 4 remaining feasibility research](STAGE4_RESEARCH.md).

Latest workflow correction: the user said "Dont do the tests yourself, just provide me with test instructions". The user has now explicitly authorized agent computer use for this Stage 8 readiness check only; other in-game checks remain user-run unless authorized. Stage 3's existing agent observations remain historical evidence. Version 0.4.0-worker-probe is compiled; the launch script stages it after the game and project-local RML are closed. The Stage 4 in-game checklist is in STAGE4_TESTING.md. No in-game check of 0.4.0 has been performed by the agent.

Stage 8's first autonomous project runner is accepted on build 0.9.1-bounds-review. The user reported “everything seems to work” after the latest review correction and authorized continuing. This is user acceptance of the current prototype, not a claim that every listed Stage 8 edge case was individually exercised. Follow-up balance, native vehicle reservation/fuel/materials and broader external-terrain-change hooks remain future work. Stage 9's concurrent scheduling checks have now been completed; the user confirmed save/reload worked. The user says the current player-facing UI is functional enough to accept; Stage 10a presentation is folded into the Stage 11 UI. The former Stage 10b named-Project/Site workflow is Stage 11, the purpose-built facility is Stage 12, and balancing is Stage 13.

**Stage 4e user observation — native terrain productivity**

- User reports one YAG DA-0.25 excavator present during 10 seconds of native terrain raising with the default brush. One sampled point changed from 11.66 m to about 16.35 m (about +4.69 m).
- User could not isolate 10 workers; the operation appeared to draw workers from across the city. The measurement is not attributable to a known worker count and does not establish area/volume throughput or autonomous Earthworks productivity.
- Worker-only calibration remains open. A proposed isolated low-population comparison is appended to [the Stage 4e instructions](STAGE4E_PRODUCTIVITY_TESTING.md).

**Stage 12.1 first model and placeable item — 4 November 2026**

- The approved two-bay/single-frontage concept now has a native ModelViewer mesh, a native material with authored submaterial colors, and a 512 px Workshop preview image. Blender source and OBJ exports remain editable in the repository.
- The staging script replaces the temporary large construction-office model and 16 inherited parking points with the custom model, two native parking entries and a provisional straight frontage connector. The item remains a construction-office type with inherited support/bounds files and a temporary five-worker setting. Dynamic worker capacity is not implemented yet.
- The user opened ModelViewer for this work. The model and native material loaded there; visual appearance, scale, placement, road snapping, vehicle routing, save/reload and demolition have not been checked in game. 12.1 is pending the user-run [test instructions](STAGE12_1_TESTING.md).

**Stage 12.1 material-path correction — 4 November 2026**

- The user reported a black model with atlas-like texture fragments in game. The installed game log showed ten failed reads of `media_soviet/buildings/blankdiffuse.dds`; the native blank diffuse file is at the media root. Corrected the MTL to reference root-level `blankdiffuse.dds` and restaged the Workshop item. No game was launched. The fix awaits a fresh in-game check.
- The user's follow-up screenshot showed the artifacts gone but the model reading as nearly uniform brown. Reordered native `$SUBMATERIAL` blocks to match OBJ/NMF order, added the native `$END` terminator, and increased contrast between roof/steel, light office/concrete, blue glass, olive tank, pale markings and safety red. Updated the Blender source palette and restaged. This iteration has not been checked in game.

**Stage 12.1 color response revision — 4 November 2026**
- The next user screenshot showed the whole model reading as gray. Kept the authored per-part diffuse palette and matched each ambient color to it, while reducing neutral specular brightness that could wash out the saturated colors under game lighting. Restaged for another user-run visual check; cause and result remain unconfirmed until checked in game.

**Stage 12.1 material-binding diagnostic — 4 November 2026**
- The user's next screenshot still looked effectively monochrome. File inspection confirmed the current MTL is copied byte-for-byte into the live Workshop item, but that alone does not prove the game binds its submaterials. Prepared an exaggerated temporary color palette for a decisive visual check. If the placed building remains gray, investigate NMF material-slot mapping and ModelViewer export/renderconfig association before further color tuning. No gameplay test was run.
- The user reported the high-contrast palette still rendered the same. Updated the staged renderconfig to reference `material.mtl` beside `model.nmf`, and the staging script now copies the MTL there as well as at item root. This removes ambiguity in relative material-file resolution; it does not yet prove that the game binds per-submaterial entries.

**Stage 12.1 renderconfig material-path correction — 4 November 2026**

- The user confirmed the placed model remained grayscale after a fresh game launch. Read-only inspection showed the game log had no Earthworks asset-load diagnostics, while ModelViewer displayed the expected distinct palette from the same staged NMF/MTL pair.
- Compared the Workshop layout with local modding reference documentation and working Workshop item structure: shared `material.mtl` belongs at the Workshop item root, and the object's `renderconfig.ini` points to it as `MATERIAL ../material.mtl`. The current Earthworks staging script instead pointed at a duplicate beside the NMF. Changed the source staging script to the canonical parent-relative reference and stopped copying the duplicate. This is a targeted path-resolution fix, not yet staged or verified in game because the game is open.
- After the user closes the game, rerun `scripts/Stage-EarthworksBuilding.ps1`; then the user should launch a fresh game and place a new facility. Verify that the finished building shows distinct roof, office, tank and trim colors. If it remains grayscale, capture the game log after that run and continue investigating NMF material-slot binding.

| Stage | Status | Technical evidence | In-game/user acceptance |
|---|---|---|---|
| Preparation | Complete | Toolchain installed, DLL built/loaded outside game, RML discovery inspected | Preparation is not an in-game stage acceptance |
| 1 — RML in-game diagnostic | Accepted | Original diagnostic evidence below; subsequent prototype launch/world/reload checklist passed | Covered by user's accepted prototype review |
| 2 — Read-only diagnostics | In progress | Frame snapshots, lifecycle gating, cursor/camera projection and terrain sampling implemented for the accepted preview; broader save-lifecycle/time-speed investigation remains | Preview subset accepted; full Stage 2 checklist not claimed complete |
| 3 — Controlled terrain edit | Accepted | Bounded native raise/lower, readbacks and historical saved-file comparisons; 0.3.1 diagnostic UI fixes | User reports raise/lower works, both deltas zero and normal native road/building placement |
| 4 — Workforce/machinery feasibility | In progress | 0.5.2 vehicle allowlist accepted; D31 direction and D32 provisional relative rate selected | User: "working great now" after the 0.5.2 correction. The initial formula is documented; the unisolated native sample still does not validate it. Multi-machine allocation, availability/reservation and fuel remain open |
| 5 — Planning preview | Accepted | 0.2.0-prototype; build/staged/reported DLL hashes match; RML active, 16 tracked hooks, zero hook failures | User: "All checks passed" |
| 6 — Durable plans | Accepted | Versioned companion-state encoding, atomic file replacement, native save-trigger observation and saved-heightmap SHA-256 matching implemented | User accepted after confirming ordinary save/reload preserves the draft and queued project. Queued grey-wireframe visibility, stale-state protection, Save As, autosave, and corrupt/mismatched companion warning behavior were not reported as tested and are deferred. See [STAGE6_TESTING.md](STAGE6_TESTING.md) |
| 7 — Office/assignments | Accepted | Build 0.7.3-facility-identity; companion save preserves assignment by building ID and descriptor | User confirmed separate Save As/reload retained project 1's assignment to facility ID 14 and automatic recognition of `9100000012/EarthworksDevelopmentFacility`; screenshot dry-run showed 0 occupants/candidates and no terrain work |
| 8 — One autonomous project | Accepted | Build 0.9.1 adds a bounded native terrain runner, simulation-time pacing, persisted progress, start/pause/resume/review/cancel controls, and overlap-only pausing for vanilla Raise/Lower brush calls. D39–D45 set prototype rules | User: “everything seems to work”; accepted and authorized progression. Earlier probe confirmed the native terrain call target. This acceptance does not claim unreported edge cases or later balance/integration work |
| 9 — Multiple projects/operations | Accepted, with facility-slot correction carried to Stage 12 | Build 0.10.0-stage9-multiproject adds project selection, priority reordering, per-facility concurrent allocation, persistent Waiting status, paused-project reassignment and confirmation-time overlap rejection. The test item inherits 16 vehicle parking points from its native template, although D49 called for two | Concurrent project allocation, paused-project skip behavior and scheduler progression were observed in the test world. User confirmed save/reload worked. Scheduler behavior is accepted; the facility's maximum native slot capacity was not tested as two |
| 10a — Player-facing UI presentation | Accepted as part of Stage 11 | The old Plan/Projects layout was replaced by the named-Project/Site workflow. Visual styling, native icon treatment, explicitly requested icon tooltips, cursor behavior and input capture are part of the accepted current UI. |
| 11 — Named Projects and in-context Site editing | Accepted by user as functionally complete | Build 0.13.13-stage11-row-actions-stay-in-list plus follow-up commits includes the Project/Site workflow, fixed-screen Site editor, inline validation, compact panel, one-line Site rows, game-style icon hover/tooltip behavior, padded height/name inputs, modifier-based immediate target steps, sample-target information, Project creation during an open draft and automatic pause on active target editing. Invalid geometry, overlaps, missing Project, target-range errors and Site-count limits are displayed before saving. The editor includes footprint, sampled ground mean, cut/fill estimates and target height. The camera View action remains disabled by request with a TODO. Checks for the latest draft-transfer and automatic-pause changes remain useful regression tests but do not block UI acceptance. Use [the Stage 11 regression checklist](STAGE10B_FIRST_UI_TESTING.md). |
| 12 — Purpose-built facility | 12.1 first placeable model staged; user-run in-game gate pending | Split into 12.1 model/item, 12.2 native building UI and 12.3 Project/facility integration; two vehicle slots; worker cap initially scales at 0.5 workers per Strength/Level point of parked groundworks machines, summed and rounded up; without such a machine worker capacity is zero; decorative tank. The initial single-frontage layout is approved; through-access is superseded. Blender `.blend`, OBJ/MTL, ModelViewer NMF, native material and preview icon are prepared. The Workshop item uses two provisional parking entries and a provisional single-front road connector. The native UI remains construction-office based, its support/bounds files are inherited, and the five-worker value is temporary; dynamic worker capacity remains unimplemented. | ModelViewer conversion and the custom placeable item are prepared. No game test was run. Follow [Stage 12.1 instructions](STAGE12_1_TESTING.md) to review scale, road approach, both bay routes, save/reload and demolition in a disposable save. |
| 13 — Balanced candidate | Planned | Scope/balance open | UI presentation and Stage 11 are accepted; Stage 12 facility design, implementation and in-game review remain before balancing |

**Stage 11 row actions remain in list — 0.13.13-stage11-row-actions-stay-in-list**

- Start, pause, resume/review, and cancel row commands now act directly on the requested Site ID without selecting it first. Explicit Site-name selection still opens its editor.
- x64 build and Tesmio exports passed; staged locally; no in-game check was run. DLL SHA-256: `DAD12FFC88BBEC751E4EA701DE2769F093676B6D4E911E78988085BF7C78FEA6`.

**Stage 11 full-width progress bar — 0.13.12-stage11-full-progress-column**

- The progress bar now explicitly uses all available content width in its table cell instead of the default item width.
- x64 build and Tesmio exports passed; staged locally; no in-game check was run. DLL SHA-256: `6E0A2F6F55EE9B8C2F6282D238B37907362D5C1BF33A78128C13EA09175E2330`.

**Stage 11 empty action headers — 0.13.11-stage11-empty-action-headers**

- Kept the Cancel and Rename header cells visually empty while preserving their compact 40 px width and full-label hover tooltips.
- x64 build and Tesmio exports passed; staged locally; no in-game check was run. DLL SHA-256: `299BEA277B7739544AA1BD75FAD927C3C7A0A0388FF6E3F809F645489B7BA08C`.

**Stage 11 compact table headers — 0.13.10-stage11-icon-headers**

- Replaced the Cancel and Rename column header text with their game icons. Fixed both columns to 40 px, sized around the icon hit area, and added full-label tooltips on header hover.
- x64 build and Tesmio exports passed; staged locally; no in-game check was run. DLL SHA-256: `1F7C4B9421D8C3BECEFD73780AC097D82BBF46A7571E494EF47690767AD0978E`.

**Stage 11 Site rename popup fix — 0.13.9-stage11-site-rename-fix**

- Site-row clicks now open the rename modal after leaving both the row and Sites-list child ID scopes, matching the dialog ID and allowing existing Site names to be edited.
- x64 build and Tesmio exports passed; staged locally; no in-game check was run. DLL SHA-256: `BDE8B118182FFCFA489C4D942FB6E1D9599418FFCCF36BD00AD615B9A2248B16`.

**Stage 11 dialog cursor fix — 0.13.8-stage11-dialog-cursor**

- Track modal popup bounds with the panel and editor so the custom in-game cursor is drawn above Project name, Delete Project, and Site name dialogs, including when a dialog extends beyond the main panel. Modal bounds also remain included in mouse capture.
- x64 build and Tesmio exports passed; staged locally; no in-game check was run. DLL SHA-256: `309E0B268D87A47D1AF2942E2F159FC65E900603DC415A1D07794FAD4C114440`.

**Stage 11 clean icon buttons — 0.13.7-stage11-clean-icon-buttons**

- Removed the extra ImGui button frame and hover fill from the Site-row Cancel and Rename icons. Enlarged them to 30 px and centered each in its cell.
- x64 build and Tesmio exports passed; staged locally; no in-game check was run. DLL SHA-256: `E0CB29577380C0A7A026540DFE70EF9665B9569070164C0FF57117D4570D45AF`.

**Stage 11 compact icon rows — 0.13.6-stage11-icon-rows**

- Reduced the default panel width to 580 px (about 17% narrower than the prior 700 px build), replaced each Site-row Cancel and Rename text button with the game icons button_cancel.png and button_rename.png, and removed the disabled View column. The table retains Site, Progress, Priority, Work, Cancel and Rename in a single row.
- x64 build and Tesmio export verification passed; git diff --check passed. Staged locally; no in-game check was run. DLL SHA-256: `0ADC54364A647ED44A265D2FF2234955BD0C4DB2EBDFBB93725BB4E2EDA7C260`.

**Stage 11 Project controls row — 0.13.5-stage11-project-controls-row**

- Moved New, Rename, Delete, Facilities and Remove completed below the Project selector, and reduced the panel default width to 700 px. The Sites row layout remains horizontally single-line.
- x64 build and Tesmio export verification passed; git diff --check passed. Staged locally; no in-game check was run. DLL SHA-256: `DECE5171D0CCF25EBE01EB2F1C8FD715A8C61D9F50AE82B1BA32FCF525E385AD`.

**Stage 11 Site selection crash hotfix — 0.13.4-stage11-site-selection-hotfix**

- The user reported a crash after selecting a new Site. The local RML log ends with game exit code 0xC0000005 immediately after the completed area was recorded. The new inline validation called CheckProjectArea without an output rectangle; its internal SnapInside helper dereferenced that null pointer. The checker now supplies a local rectangle when the caller does not request one.
- x64 build and Tesmio export verification passed; git diff --check passed. The build is staged locally. No in-game retest was run. User-run retest: create a New Site and complete its two-corner selection, then verify the editor opens and overlapping-draft validation still works.
- Staged DLL SHA-256: `BCE66ACBA91306E1CAD6FB934BE73DA226C9542DDD8D61EEEFC860524FF0C9EE`.

**Stage 11 compact Sites panel — 0.13.3-stage11-compact-panel**

- Finished the first version of the Projects workflow, New Site flow, two-corner selection and separate world-anchored Site editor. Added typed and map-sampled target height, sampled ground mean, footprint dimensions, and estimated cut/fill. Existing named Projects, per-Site controls, Project-level facility assignment and Project-scoped capacity scheduling remain available.
- Clicking a Site footprint reopens its editor, including from a closed Earthworks panel. The editor starts beside the Site, then stays fixed on screen as the camera moves; after manual dragging it remains at the chosen screen position until explicitly closed. Finished Sites have visible outlines and read-only editors. Editing a running/waiting Site's target automatically pauses it; a changed target requires terrain review before resuming (D85 supersedes D83).
- The New Site editor validates the selected Project, completed geometry, native heightmap bounds/cell limits, finite target height, unfinished-footprint overlaps and save Site-count limit. It displays the first blocking reason inline and disables **Save Site** until the draft passes. Confirmation uses the same validation so displayed readiness and save behavior stay consistent.
- The user authorized continuing implementation through the first version of all Stage 11 elements. The current UI was subsequently accepted by the user as functionally complete. No game was launched and no in-game tests were performed by the agent for the latest follow-up changes.
- x64 build and Tesmio export verification passed; `git diff --check` passed. The build was staged to the project-local RML runtime. DLL SHA-256: `6257B57566F7FB20A2A8BE4AEFD1E4538D46F298F518DE7B026B40AE5AA69C4B`; staged DLL hash matches. No in-game checks were run by the agent. User-run review checklist: [STAGE10B_FIRST_UI_TESTING.md](STAGE10B_FIRST_UI_TESTING.md).

**Stage 6 implementation — accepted with deferred checks**

- User chose to preserve both drafts and confirmed projects across save/reload (D33).
- Added versioned companion-state encoding with CRC validation, bounded decode, a SHA-256 fingerprint of the matching save's `heightmap.dds`, and atomic replacement at `media_soviet/save/<slot>/earthworks.dat`. The runtime observes the native save trigger through the existing Late hook and restores only after the loaded save's fingerprint matches.
- Added a queue-only confirmation control. Confirmation moves the current draft into the queue; queued work remains visible as a grey wireframe while a new draft can be planned in cut/fill colors. Stage 6 never executes terrain work. Scheduling and autonomous work remain later stages.
- Missing companion data leaves the screen empty. Corrupt or terrain-mismatched metadata is rejected and shown as a status message; the native save and terrain are not changed. The user chose warning-and-discard with no import/recovery flow for now (D34).
- Build: `0.6.1-queued-wireframe`; x64 build succeeded and was staged in the project-local RML runtime. Built and staged DLL SHA-256: `7511A8054DBDB8AFF66C08151937F09E68CE476A8348A3E6250ECFDFA9380903`. No game or automated tests were run by the agent. User checklist: [STAGE6_TESTING.md](STAGE6_TESTING.md).
- User-run result and acceptance: ordinary save and reload preserved both the editable draft and queued project. The user accepted Stage 6 while deferring stale-state protection. Queued grey-wireframe visibility, Save As, autosave, and corrupt/mismatched companion warning behavior were not reported as tested. These are recorded as unverified; they are not treated as passed.

**Stage 7 implementation — accepted by user**

- User authorized continuing to the next stage after accepting Stage 6.
- User decisions: show distance only without an in-range claim (D37); manually assign a queued plan to the selected facility (D38); every dedicated Earthworks facility instance should be automatically recognized without per-session registration (D36).
- Current probe readings cannot establish that parked vehicles are idle, natively available or exclusively reserved. UI wording must call them candidates. Stage 4 reservation/availability remains a blocker before autonomous execution.
- The official native references establish different distances for different mechanics: terrain machinery in a depot/Construction Office within 1,000 m, and citizen walking up to 480 m on the fastest paths. Neither establishes this mod's facility range. See [Stage 7 preparation](STAGE7_PREPARATION.md).
- Community HDO source for the target game build documents the type descriptor at building+0x318 and says descriptor entries begin with an internal asset name. The user screenshot shows the actual full descriptor string `9100000012/EarthworksDevelopmentFacility`; the initial implementation compared only `EarthworksDevelopmentFacility`, causing the selected facility to be rejected as "Other building".
- Build **0.7.1-assignment-ux** automatically recognizes facility instances, applies the existing direct ordinary-job block and vehicle allowlist to those instances only, supports manually assigning one queued project, and displays observed occupants, parked bulldozer/excavator candidates, distance to plan centre and the provisional one-machine D32 rate. No range claim, completion estimate, reservation, resource use or terrain editing is part of Stage 7. The Stage 7 panel now exposes **Find Earthworks facility** and spells out map selection before assignment.
- Companion schema v2 stores the assigned native building ID; v1 remains readable with projects unassigned. Reload resolution requires one unique instance whose ID and exact asset name both match. An unresolved assignment remains queued and is not reassigned.
- Build **0.7.0-stage7-dryrun**: x64 Release build succeeded; the build script verified AMD64 and the three Tesmio exports. The built and staged DLL hashes matched: `077B8DDAA164725FDA8E23E1C7B58B8CFDB9E3617401D7B0E01D5CB459CE275D`. Staging completed and the game was not launched.
- UX correction build **0.7.1-assignment-ux** compiles successfully and the build script verifies AMD64 and all three Tesmio exports. DLL SHA-256: `B40529C52863D60D43B7FEE6D699B06638CC4ECFCE74F34BB8FB98C84CBBEE40`.
- Staging 0.7.1 was attempted while the project-local RML/game process held the previous DLL open; replacing `tools/runtime/rml/rml/plugins/EarthworksPlanning.dll` failed with a file-in-use error. The binary was not replaced; close the game and RML, then rerun `scripts/Stage-Diagnostics.ps1` before testing 0.7.1. No game was launched and no tests were run by the agent.
- Staging refreshed the project-local RML development preset and generated Workshop item `9100000012`; Steam-subscribed RML configuration is untouched. The game was not launched. No tests were run by the agent.
- User separate Save As/reload screenshot confirms project 1 remains assigned to facility ID 14, with the exact descriptor recognized automatically. The user then asked "whats next", accepting Stage 7 and authorizing progression. Dry-run showed no occupants or candidate vehicles; no terrain work occurred. See [STAGE7_TESTING.md](STAGE7_TESTING.md).

**Stage 8 readiness probe — 0.8.0-reservation-probe**

- Pinned RepublicMP `determinism/office_guard.cpp` reads `vehicle+0x698` as a mechanism job-state value and `vehicle+0x680` as a route-stop list when checking native candidates. `lockstep/route_tools.cpp` shows depot dispatch also applies an unusable/capability check, depot/type checks, and current home/owner state; these observations do not amount to a general reservation API.
- Extended the read-only Earthworks inspector to display raw job-state and route-stop readings for bulldozers and excavators. It does not classify them as available or reserve them. User-run comparison steps are in [STAGE8_READINESS_TESTING.md](STAGE8_READINESS_TESTING.md).
- The user selected no extra costs for this prototype; later fuel and quarried-stone behavior is recorded. Pause/cancel, conflict and progress display rules are recorded in D39–D42.
- Release x64 build succeeded; the build script verified AMD64 and all three Tesmio exports. DLL SHA-256: `A68AE45920F37BF95D1A810B8A374070315FA875D2856FFE72E5CB55E29C9756`. `git diff --check` passed. No tests were run and the agent did not launch the game.
- Initial staging could not replace the old project-local DLL because another process held it. The staging script now uses a version-and-hash DLL filename and updates the plugin manifest to reference it, avoiding replacement of a loaded binary. Stage-Diagnostics succeeded; the staged DLL hash matches the build hash. The agent did not launch the game. User-run instructions are in [STAGE8_READINESS_TESTING.md](STAGE8_READINESS_TESTING.md); in-game acceptance remains pending.
- On 2 November 2026, the user explicitly authorized the agent to use the open computer for this check. The dedicated facility was recognized as ID 14 and contained two parked machine candidates. After a brief money-mode stroke (displayed balance −8 rubles), the agent switched to **Build from resources**, made one brief raise-terrain stroke on empty ground, and allowed about 10 seconds of gameplay time. The inspector still showed both vehicles parked, raw job state `-1`, and zero route stops; no visible movement or fuel reading was observed. The computer-use drag was too brief to meet the planned 10-second held operation, so this does not establish native dispatch or fuel behavior. The test save has a small terrain alteration beside the facility; no save was made. Readiness remains pending. Full details are in [STAGE8_READINESS_TESTING.md](STAGE8_READINESS_TESTING.md).
- Static follow-up disassembled the installed `C3D_TERRAIN::EditRaiseLower` export at `C3DDLL64.dll+0xF8E50`. It applies changes to the heightmap and has no office or vehicle parameter. This does not reveal how the higher-level game tool selects equipment, tracks work or prevents another tool from using a candidate. That caller-side path and exclusive reservation remain the technical gate before production terrain writing.

**Stage 7 panel readability update — 0.7.2-wide-panel**

- User feedback: long Earthworks panel text is cut off; request was to widen the panel or make it side-scrollable.
- Changed the initial panel width from 430 px to 660 px (bounded by screen width), replaced automatic content sizing with a bounded, normally scrollable window, and wrapped the queued-plan note, distance disclaimer and candidate asset/category line. The window can be resized and moved during the session.
- Build **0.7.2-wide-panel** compiled in Release; the build script verified AMD64 and all three Tesmio exports. DLL SHA-256: `91BAD3D44D2F97E5A68B50555B07B13B30234B0C4D4FDCEBAF549E720C0A0E1F`. `git diff --check` passed. No tests were run.
- Staging is deferred while the game/RML holds the old DLL. After closing both, run `scripts/Stage-Diagnostics.ps1`; then use the updated [Stage 7 checklist](STAGE7_TESTING.md) to verify width, wrapping, move/resize behavior, and the facility assignment flow. User review remains pending.

**Stage 7 facility identity correction — 0.7.3-facility-identity**

- User-provided inspector screenshot shows the candidate row as **Other building ID 14**, with asset name `9100000012/EarthworksDevelopmentFacility`, category 12 / subtype 30. This proves the building selection works and the previous exact-string identity predicate omitted the Workshop item ID prefix.
- Updated the predicate to match that observed full asset path. This retains exact identity matching and does not broaden recognition to ordinary construction offices.
- Build **0.7.3-facility-identity** compiled in Release; the build script verified AMD64 and all three Tesmio exports. DLL SHA-256: `095744DF0CEA482B78DF302A38963D6E87A39302D9C10000D522463C812C0F29`. `git diff --check` passed. The game and tests were not run by the agent.
- At build time, no in-game check had been performed. The user later staged 0.7.3 and explicitly authorized agent computer use for this test stage. The live inspector recognized building ID 14 (category 12 / subtype 30) as an Earthworks facility; the staged DLL hash matched the 0.7.3 build hash. The agent clicked **Assign selected facility**, and the panel reported "Project assigned" and "Assigned Earthworks facility ID 14". The wide panel displayed wrapped text. Dry-run showed 0 occupants, 0 parked machine candidates and 0.00 manual-rate units, matching the empty facility. No terrain mutation or native save/reload was attempted. Ordinary-office exclusion remains unchecked.
- This is targeted evidence for the identity/assignment correction, not acceptance of all Stage 7 checks. The user still needs to review the result and, if desired, check ordinary-office behavior and save/reload.
- Follow-up computer-assisted attempt: the visible test area contains the dedicated facility but no ordinary construction office to inspect; none was placed. Save/reload was not performed because the current slot could not be identified safely from the available UI, so no save was overwritten. The game was left open with Earthworks reopened, project still assigned to ID 14, and no terrain edits made. These checks remain pending.
- User then placed a large native construction office for inspection. The Stage 4 inspector reported **Other building ID 15**, asset `construction_office4`, category 12 / subtype 30, and the Stage 7 panel stated the selected building was not an Earthworks facility. The native **Carabadag Construction office** panel opened and displayed its ordinary construction-source controls. No construction jobs were assigned and no vehicles were purchased, so only identity separation and native-panel access were observed. The previously queued project remained assigned to Earthworks facility ID 14; terrain was not edited. Save/reload remains pending.
- Save/reload attempt after the user reopened the game: Load Saved Games showed the newest entry as `A` (`save\autosave2`, game date 15 January 1970). Selecting it produced a **Missing Workshop item(s)** prompt asking to subscribe. The prompt was canceled without subscribing or loading the save. No save was modified or deleted. Persistence remains unverified until the local Workshop building item is available to the loaded game without that prompt.

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

**Stage 8 diagnostic iteration — 0.8.1-terrain-call-probe, 2 November 2026**

- Added an optional, read-only probe at the pinned native `C3D_TERRAIN::EditRaiseLower` export. It activates only when the module RVA and exact 18-byte prologue match; otherwise the plugin continues without the probe. When active, it records call count, game caller address, position, brush inputs and flag, then forwards the original call unchanged. The overlay displays these readings and states that they do not identify or reserve equipment.
- This is instrumentation only. It does not perform Earthworks work, modify terrain, add costs, or establish machine reservation. Stage 8 readiness remains pending the user's sustained vanilla terrain-tool observation and review of the resulting trace.
- Release x64 build completed; the three Tesmio exports were verified; no tests were run. DLL SHA-256: `f01c0899ee3f1aea268f30f284ddfc02f5a49e257f63c788f1912b05e9ea42ec`.
- The build was subsequently staged with `scripts/Stage-Diagnostics.ps1` after confirming the game and project-local RML were closed. The staged DLL hash matches the build. The game was not launched, and the in-game check remains pending.
- The user then explicitly asked the agent to run the check in the open game. The selected fresh world was at 3 January 1970. Earthworks facility ID 14 had 0 occupants / 0 slots and one parked `service_exvacator_eo302b` (ID 0); H/A/P were all true, raw native job state was -1, and route-stop count was 0. These readings remained unchanged after short terrain-tool inputs.
- The agent selected Terrain tools > Raise terrain; the expanded mode panel visually showed Build from resources checked. One brief drag, a click and a short 140-pixel brush drag were attempted. The probe remained at 0 native `EditRaiseLower` calls, while a brown swath appeared visually along the longer drag path. No height readback was taken, so actual elevation change is unconfirmed. Money balance remained unchanged. No save was made.
- The computer drag interface could not sustain the planned 10-second hold. This partial check does not establish dispatch, fuel use or exclusive availability. The zero-call reading suggests the terrain UI may use a different terrain-write path or the brief inputs did not reach the hooked export; further source tracing is required. Full observations and limitations are in [STAGE8_READINESS_TESTING.md](STAGE8_READINESS_TESTING.md).
- The user subsequently reported that the excavator was visibly used during vanilla Raise terrain, despite the screenshot retaining unchanged H/A/P links, raw job state -1, zero route stops, and zero `EditRaiseLower` calls. Treat this as evidence that the native tool can use a facility machine without changing the exposed diagnostics; fuel use and exclusive reservation remain unverified. Trace the higher-level mechanism-selection/accounting path before proceeding with autonomous terrain writing.
- The earlier one-off authorization was renewed explicitly for this check only. No authorization for other agent-controlled gameplay testing is inferred.

**Stage 8 corrected probe — 0.8.2-angle-limit-probe, 2 November 2026**

- Static import mapping showed the native terrain tool calls `C3D_TERRAIN::EditRaiseLowerAngleLimit` through EXE IAT RVA `0x86D240`, targeting C3DDLL64 RVA `0xF9380`; the prior 0.8.1 probe watched `EditRaiseLower` and therefore could not see the vanilla tool call.
- Replaced the read-only hook with a guarded IAT probe for the exact decorated symbol, expected C3DDLL64 RVA, EXE IAT RVA/target, and pinned 23-byte export prologue. It records the game caller, position, brush parameters, flag and angle limit, then forwards the original call unchanged. It does not read or change vehicle state and does not modify terrain.
- Release x64 build succeeded; all three Tesmio exports were verified. DLL SHA-256: `a0186a5864956ceff05606e90bff873207376fa5e414df749eff937f84d7c6ac`. `scripts/Stage-Diagnostics.ps1` staged the build and matching hash in the isolated project-local RML runtime after confirming the game and project-local RML were closed. The game was not launched and no tests were run.
- Next check is user-run: load the disposable test world, make one ~10-second vanilla Raise terrain hold with **Build from resources**, then confirm the call count increments and capture the last caller. This only validates the diagnostic target; machinery reservation, fuel and autonomous execution remain open.
- User-run result: the probe showed 54,168 calls and `SOVIET64.exe+0x3AAE5E` as the last caller. This is the return address after the native brush call at `SOVIET64.exe+0x3AAE58`, confirming the correct hook. The counter is not a productivity measure. The previous user report of visible excavator use remains significant, but no fuel readout or machine reservation was observed.

**Stage 8 first runner — 0.9.1-bounds-review, 2 November 2026**

- Implemented a one-project terrain executor using the pinned native `EditRaiseLower` writer, native height readback and heightmap texture open/close. It rejects stale sampled terrain before start, writes at most 64 vertices per frame inside the selected footprint, and pauses for review if the footprint cannot be read or the 40,000-vertex cap is exceeded.
- Added simulation-time pacing, persisted project status/progress/cursor/work remainder, start/pause/resume/review-and-resume/cancel controls, and a progress bar. User decisions D43/D44/D45 select overlap-only pause for vanilla Raise/Lower brush calls, simulation-time pacing, and a provisional rate of 10 m3 per simulation hour per manual-rate unit.
- The rate uses up to 10 observed facility workers and one parked machine candidate; without a candidate it uses the facility's observed workers alone. No machine is reserved, animated, dispatched or charged fuel. External worker sources, exact vehicle skill ratings, other terrain-changing hooks, actual completion shape/rate, and multi-machine scaling remain incomplete or unverified.
- Release x64 build completed and all three Tesmio exports were verified. `git diff --check` passed; no test executable was run and no in-game test was performed. DLL SHA-256: `c6dfa03f21706b0fc9334ac62cdca58fbe65552cd8d69a08d211daeb5452e769`.
- The matching DLL and manifest were staged in the isolated project-local RML after confirming the game and project-local RML were closed. Stage 8 is ready for the user-run checklist in [STAGE8_READINESS_TESTING.md](STAGE8_READINESS_TESTING.md); acceptance is pending.

**Stage 8 bounds-review correction — 0.9.1-bounds-review, 2 November 2026**

- The user reported that new projects over old finished/cancelled footprints pause and show a terrain-review message. With permission to inspect the open game, the live panel showed project 5 paused at 0% and the specific runtime message “Terrain bounds changed or cannot be read.” The displayed “terrain changed in the footprint” text was generic for every `needsReview` pause and did not identify the cause.
- Separated native-grid/footprint validation from the stale-terrain comparison. Start/resume/review now validate grid bounds before entering Running; the executor distinguishes missing terrain, invalid grid, out-of-map, too-small and too-large footprints. A geometry rejection no longer asks for terrain-conflict review. Added runtime logging with grid origin/spacing/dimensions and project coordinates, plus logs for actual post-planning height deltas.
- Updated the review UI to say only that review is required, without asserting that the terrain changed. Build 0.9.1 compiled and passed Tesmio export verification; `git diff --check` passed, no tests were run. DLL SHA-256: `8ddea23a28884b3665f12dad5c49e8fea7a74a913471a724029f16ffe31954a5`. It is not staged yet because the game/RML session is open; no terrain was edited during inspection. User retest is pending after the game and project-local RML are closed and the new build is staged.

**Stage 8 acceptance — 0.9.1-bounds-review, 2 November 2026**

- User report: “everything seems to work, continue”. Stage 8 is accepted and Stage 9 is authorized to begin.
- Acceptance covers the user's observed first autonomous prototype behavior after the bounds-review correction. No claim is made here that every persistence, conflict, resource-use, vehicle-reservation or edge-case check was separately performed. Deferred items remain candidates for later integration/balance work.

**Stage 9 scheduling decisions — 2 November 2026**

- User selected concurrent projects using available machines, with the option for one project to use multiple machines. The player can reorder priority; each project requires explicit Start. Facility assignments remain manual under D38.
- Allocate facility workers to higher-priority eligible projects first. Skip paused or review-blocked projects so lower-priority eligible work may proceed.
- Reject confirmation of a project whose footprint overlaps any queued, waiting, running or paused project. Completed and cancelled projects do not reserve their old footprint.
- A paused project may be reassigned to a different Earthworks facility without losing progress; any existing terrain-review requirement remains in force (D47/D48).
- For Stage 9, the user approved increasing the developer-only facility from 5 to 20 worker places while keeping two machine stations. This is not a release-balance decision (D49).
- D46–D49 record the chosen rules. No Stage 9 in-game checks have been performed by the agent; follow [Stage 9 instructions](STAGE9_TESTING.md).

**Stage 9 first implementation — 0.10.0-stage9-multiproject, 2 November 2026**

- Added a scrollable project list, selection, move-up/move-down priority controls, grey outlines for live projects, per-project progress/allocation display, confirmation-time overlap rejection and a persisted `Waiting` state. Save schema 4 accepts existing schemas 1–3.
- Started jobs share each assigned facility's observed worker and parked-machine candidate counts in priority order. Each started job first receives one machine when available; remaining machines and workers go to higher-priority jobs. If there are no machines, workers are assigned to the highest-priority started project under D31. Paused/review-blocked projects are skipped. Paused work can be reassigned without losing progress.
- Reused the Stage 8 terrain executor. No machine candidate or worker is counted twice among Earthworks projects using the same facility. Candidate counts still do not prove exclusive native vehicle availability/reservation; no fuel, stone, animation, external worker source or balancing integration was added.
- The regenerated test building has 20 worker places so the user can test two fully staffed machines and concurrent projects at one facility; it retains two machine stations and remains temporary test content.
- `build.cmd` completed successfully, verifying x64 architecture and all three Tesmio exports. `git diff --check` passed. No tests were run and the game was not launched. Built/staged DLL SHA-256: `19b2dd39eb4f2c70ef3f78491eede658e10f2528de458feae8918e1b4676a0c6`.
- The isolated project-local RML and dedicated facility content were staged; built and staged DLL hashes match. In-game test instructions are in [STAGE9_TESTING.md](STAGE9_TESTING.md).
