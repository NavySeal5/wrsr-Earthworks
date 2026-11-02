# Stage 8 readiness check — native work overlap

Build: **0.8.0-reservation-probe**. DLL SHA-256: `A68AE45920F37BF95D1A810B8A374070315FA875D2856FFE72E5CB55E29C9756`.

The build is staged in the project-local RML runtime. Close the game and project-local RML if they are open, then run `launch-prototype.cmd`; confirm the loaded plugin reports this build before starting the check.

Purpose: check whether vanilla terrain work can use workers or machines held at the dedicated Earthworks facility. This is a diagnostic only. It does not start an Earthworks project or authorize autonomous terrain edits.

## Use a disposable save

1. Make a separate copy/save of the test world. Pause the game and leave no active Earthworks project assigned to the facility.
2. Confirm the Earthworks building has no ordinary construction job. Place one allowed bulldozer or excavator in its machine slots and note the vehicle type/ID if shown. Leave the machine parked and do not issue it another order.
3. Open Earthworks, use **Find Earthworks facility**, select the intended building and record the inspector's occupant count, parked count, H/A/P flags, raw native job state, and route-stop count for the machine. Take a screenshot.

## Check vanilla terrain overlap

4. Use the native raise/lower terrain tool on a small, empty patch close to the facility for about 10 seconds. Do not select roads, buildings, underground networks, or the Earthworks project area.
5. Pause. Reopen the Earthworks inspector and compare the same building and vehicle readings. Check the native office/vehicle UI for a changed assignment, parked state, location, or fuel reading if one is visible.
6. If convenient, repeat once with the vehicle removed from the facility. This comparison can reveal whether the machine changes native terrain-tool behavior; it is not required if the first pass is unclear.

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
