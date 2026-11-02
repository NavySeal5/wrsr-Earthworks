# Stage 8 readiness check — native work overlap

Build 0.8.0 reservation-probe and 0.8.1 partial observations are recorded below. The current diagnostic build is **0.8.2-angle-limit-probe**; it adds a read-only counter for the native `C3D_TERRAIN::EditRaiseLowerAngleLimit` import used by the vanilla terrain tool and displays the last game caller and brush inputs. It does not write terrain or reserve vehicles.

Build 0.8.1 was staged in the project-local RML runtime and loaded in-game for the earlier partial check. Build 0.8.2 is now staged in the project-local RML runtime. Built and staged DLL SHA-256: `a0186a5864956ceff05606e90bff873207376fa5e414df749eff937f84d7c6ac`. It has not been launched or tested in-game. Stage 8 readiness still requires tracing machine availability/reservation before any executor is added.

Purpose: check whether vanilla terrain work can use workers or machines held at the dedicated Earthworks facility. This is a diagnostic only. It does not start an Earthworks project or authorize autonomous terrain edits.

## Use a disposable save

1. Make a separate copy/save of the test world. Pause the game and leave no active Earthworks project assigned to the facility.
2. Confirm the Earthworks building has no ordinary construction job. Place one allowed bulldozer or excavator in its machine slots and note the vehicle type/ID if shown. Leave the machine parked and do not issue it another order.
3. Open Earthworks, use **Find Earthworks facility**, select the intended building and record the inspector's occupant count, parked count, H/A/P flags, raw native job state, and route-stop count for the machine. Take a screenshot.

## Check vanilla terrain overlap

4. In the bottom construction toolbar, open **Terrain tools** and choose **Raise terrain**. Set **Build from resources**. On a small, empty patch close to the facility, click and hold for about 10 seconds. Do not select roads, buildings, underground networks, or the Earthworks project area.
5. Pause. Reopen the Earthworks inspector and compare the same building and vehicle readings. Check the Stage 8 probe counter, last caller, and recorded brush/angle parameters. Check the native office/vehicle UI for a changed assignment, parked state, location, or fuel reading if one is visible.
6. Do not repeat with the machine removed for this check; the user already visually observed the excavator being used. A no-machine comparison can be planned later if the corrected trace raises a specific question.

## Focused check for build 0.8.2

1. With the game and project-local RML closed, run `launch-prototype.cmd`. In RML confirm **0.8.2-angle-limit-probe**, then launch the game and open the same disposable test world.
2. Open Earthworks and expand **Stage 8 | Native terrain-call probe**. It should say the angle-limited probe is available. Record the starting call count.
3. Select the native **Raise terrain** tool, keep **Build from resources** selected, and hold over a small empty patch near the facility for about 10 seconds. Do not confirm or start an Earthworks project.
4. Pause and inspect the Stage 8 section. The expected result is an increased `EditRaiseLowerAngleLimit` call count and a last caller inside `SOVIET64.exe`; capture the caller RVA and displayed parameters in a screenshot.
5. If the probe is unavailable, the counter remains zero, or the game behaves unexpectedly, stop and send a screenshot plus the relevant RML log. Do not save the test world. This check verifies only that the corrected diagnostic observes the native terrain-tool call; it does not prove equipment reservation, fuel use, or autonomous operation.

## Report

Send the before/after screenshots and note whether the vehicle moved, lost its parked link, acquired an assignment, showed fuel use, or changed its raw job state/route list. Also note whether the terrain tool behaved differently with the vehicle present.

The pinned native source reads the vehicle job-state field and route-stop list when filtering mechanism candidates. The Earthworks inspector now displays those raw values for bulldozers and excavators. Any observed movement, assignment, job-state change, route change, or fuel use means the machine cannot be treated as exclusively available during vanilla terrain work without additional interception. No visible change does **not** prove exclusive reservation: the native tool may use the machine without changing these exposed fields. This check can narrow the risk but cannot by itself clear the reservation blocker.

Do not run this check in a valuable save or with an active Earthworks operation.

## Agent-assisted partial observation — 2 November 2026

- The user explicitly authorized testing in a fresh save. The selected building was automatically recognized as Earthworks facility ID 14, category 12 / subtype 30. Baseline showed 0 occupants / 0 slots, 2 parked native slots, 2 home links, 2 assignment links, and 2 parked matches. The two candidates were `service_exvacator_eo302b` (ID 0) and `service_buldozer_stalined` (ID 1); both showed raw native job state `-1`, 0 route stops, and no first stop at the facility.
- The agent selected **Raise terrain** from the bottom toolbar's **Terrain tools** category. A brief stroke in money mode reduced the displayed ruble balance by 8. This was a tool confirmation stroke, not the resources-mode comparison.
- The agent enabled **Build from resources** and made one brief raise-terrain stroke on empty ground near the facility. The inspector still showed both machines parked (2/2 matches), both raw job states at `-1`, and both route lists at 0. No machine movement or visible fuel reading was observed.
- Limitation: computer-use drag released too quickly to provide a sustained 10-second terrain-tool hold. The resources-mode stroke therefore does not establish native vehicle dispatch or fuel use. This is a partial observation only; Stage 8 readiness remains pending. Do not infer machine exclusivity from unchanged diagnostics.
- No Earthworks project was started, no save was made, and no vehicles were purchased or removed. The fresh test save now has a small terrain alteration beside the facility; discard this test save or restore the patch manually to return to a pristine starting point.

## Static native-call follow-up

- On installed game build 23935965, `C3DDLL64.dll` exports `C3D_TERRAIN::EditRaiseLower` at RVA `0xF8E50`. MSVC `dumpbin` disassembly shows this method calculates the affected heightmap region and applies per-cell terrain edits. It accepts a terrain object, position and brush/height parameters; it does not accept a construction office or vehicle.
- Therefore the low-level terrain writer alone cannot establish who pays for, enables or performs manual terrain work. Vehicle selection, eligibility and any work-rate/resource accounting must happen in a caller or separate game subsystem. Do not use the Stage 3 direct-call experiment as proof of machine reservation or as the production executor without the missing caller-side safety checks.
- Build 0.8.1-terrain-call-probe optionally hooks this export only after matching its RVA and 18-byte prologue. It records a call count, the last return address and brush inputs, then forwards the exact arguments to the original function. A mismatch or failed hook leaves the existing diagnostic active. The probe's observations cannot establish vehicle use or reservation; they identify which native game caller reaches the terrain writer during the held operation.
- Build verification: Release x64 completed successfully and all three Tesmio exports were verified. No code tests were run. DLL SHA-256: `f01c0899ee3f1aea268f30f284ddfc02f5a49e257f63c788f1912b05e9ea42ec`. The staged DLL matches this hash; the plugin loaded in-game during the partial check below.

## Agent-assisted partial check — 2 November 2026

- The user explicitly asked the agent to run the Stage 8 check with the game open. The game was at 3 January 1970 with Earthworks facility ID 14, slot 14, descriptor `9100000012/EarthworksDevelopmentFacility`. The read-only baseline showed 0 non-null occupants / 0 slots, one native parked slot, one linked vehicle (ID 0, `service_exvacator_eo302b`) with home/assignment/parked links, raw job state `-1`, and zero route stops.
- The native Terrain tools category was opened and **Raise terrain** selected. The expanded bottom-right options visually showed **Build from resources** checked. The displayed ruble balance remained 9,983,084.
- A brief drag, a single click and one short 140-pixel brush drag were attempted on nearby ground. A brown surface swath appeared along the longer drag path, but no height sample was taken, so a terrain-height change is not confirmed. The native terrain-call probe remained at 0 observed `C3D_TERRAIN::EditRaiseLower` calls before and after these inputs. This suggests the manual tool did not reach the probed export during this partial check, or that the brief inputs did not invoke it; it does not identify an alternate writer.
- After the inputs, facility ID 14 still showed the same parked excavator candidate, H/A/P links, raw job state `-1` and zero route stops. No visible movement or fuel reading was observed.
- The computer drag API cannot sustain the planned 10-second press; this run is inconclusive about native dispatch, fuel and reservation. No save was made. Do not save this test world if a pristine fixture is needed; the brown swath is unsampled and should be treated as an unsaved test alteration. Stage 8 remains pending. The unexpected zero-call trace is a new source-tracing lead: the native Raise terrain UI may use a different terrain-write path than the hooked export.

## User follow-up — vanilla mechanism visibly used

- The user later reported that, at least visually, the excavator was being used during the vanilla Raise terrain operation, just as with vanilla mechanics. The accompanying screenshot still showed one linked/parked excavator candidate, raw job state `-1`, zero route stops, and zero calls observed by the `EditRaiseLower` probe; it also showed a contoured mound, but there was no before/after height readback.
- Treat the user's visual observation as evidence that vanilla terrain work can use the facility excavator even while these exposed diagnostic fields remain unchanged. The probe's zero count does not disprove the terrain change or mechanism use; it likely misses the relevant higher-level path, or this writer is not used by that UI path.
- Fuel use was not confirmed. Exclusive availability is not established. Before implementing autonomous Earthworks work, continue source tracing for vanilla terrain-tool mechanism selection/accounting and identify a safe reservation or interception strategy. No further in-game repetition is needed until that investigation gives a specific diagnostic target.

## Static trace progress — terrain brush entry point

- The pinned main-executable disassembly shows the terrain-tool dispatcher at `SOVIET64.exe+0x312A1E` calling the brush routine at `SOVIET64.exe+0x3A8A70` (the call is repeated for several terrain modes). At routine entry, the arguments are the world pointer (`RCX`) and an integer mode (`EDX`); there is no explicit vehicle or office argument.
- The routine reads world/tool state and iterates a pointer vector stored at world offset `+0x11C28`, using object-position/bounds helpers. The vector's concrete type and whether any helper performs mechanism selection, animation, cost, or fuel accounting are not identified yet. This is only a first pass through the entry path, not proof that the routine has no indirect machinery interaction.
- The next source task is to identify the world vector and trace its relevant helper calls, then relate the observed visual excavator use to any native state/resource updates. Stage 8 is not cleared: fuel and exclusive reservation remain unverified.

## Static trace correction and 0.8.2 probe target

- `dumpbin /imports SOVIET64.exe` maps IAT entry `EXE+0x86D240` to `?EditRaiseLowerAngleLimit@C3D_TERRAIN@@QEAAXVC3DVECTOR3@@MMMM_NM@Z`. The installed `C3DDLL64.dll` exports that symbol at RVA `0xF9380`; its pinned prologue starts `48 8B C4 F3 0F 11 58 20 55 48 81 EC 20 01 00 00 4C 8B 89 10 02 00 00`.
- This is the export called by the native terrain brush, unlike `EditRaiseLower` at `0xF8E50`, which the 0.8.1 probe watched. The 0.8.1 zero-call result was therefore a probe-target mismatch; it did not show that the UI used a different terrain writer. The 0.8.2 read-only probe validates the export RVA/prologue and the executable's exact IAT slot/target before patching that import, then forwards the original arguments unchanged.
- The angle-limited export's signature contains terrain, brush position/parameters, a flag and an angle limit, but no explicit office or vehicle argument. The corrected probe can identify the actual caller and brush inputs; it still cannot establish machine selection, fuel use or exclusive availability. The 0.8.2 user-run check below verified that it sees vanilla Raise terrain calls. Do not interpret the trace as proof of equipment reservation.
- The user-run 0.8.2 check succeeded. The screenshot showed 54,168 `EditRaiseLowerAngleLimit` calls and last caller `SOVIET64.exe+0x3AAE5E`; the disassembly shows that address immediately follows the terrain IAT call at `+0x3AAE58`. The screenshot also showed the native terrain contours and the Stage 7 dry-run panel. This confirms the corrected probe observes the live native brush path. The counter is an edit-call count, not a measure of volume, productivity, machine-hours or fuel. No save/reload or fuel measurement was reported.
- In the brush function, the terrain API is passed the terrain object, point and brush parameters, not an office or vehicle. The user's earlier visual observation of excavator use therefore remains a separate native behavior to trace; do not infer that this terrain-call hook reserves or consumes the excavator.

## Community-source follow-up

- The pinned RepublicMP community source (`external/RepublicMP/lockstep/brush_tools.*` and `external/RepublicMP/world/terrain_hook.cpp`) independently identifies `0x3A8A70` as the per-frame terrain brush path. It records the cursor, brush, target height, stroke timers and money/ledger state, and observes `EditRaiseLowerAngleLimit` calls. Its source does not describe construction-office mechanism selection or a vehicle reservation in this brush path.
- This narrows the trace: the terrain writer and stroke accounting are separate from the excavator behavior the user saw. RepublicMP's description is a reverse-engineered community reference, not proof that no indirect native helper handles machine animations or productivity. The remaining source task is to locate that separate mechanism-use path or establish a safe Earthworks policy around possible simultaneous native use.
