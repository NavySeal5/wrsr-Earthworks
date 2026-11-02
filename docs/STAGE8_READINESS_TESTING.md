# Stage 8 readiness check — native work overlap

Purpose: check whether vanilla terrain work can use workers or machines held at the dedicated Earthworks facility. This is a diagnostic only. It does not start an Earthworks project or authorize autonomous terrain edits.

## Use a disposable save

1. Make a separate copy/save of the test world. Pause the game and leave no active Earthworks project assigned to the facility.
2. Confirm the Earthworks building has no ordinary construction job. Place one allowed bulldozer or excavator in its machine slots and note the vehicle type/ID if shown. Leave the machine parked and do not issue it another order.
3. Open Earthworks, use **Find Earthworks facility**, select the intended building and record the inspector's occupant count, parked count, and H/A/P flags for the machine. Take a screenshot.

## Check vanilla terrain overlap

4. Use the native raise/lower terrain tool on a small, empty patch close to the facility for about 10 seconds. Do not select roads, buildings, underground networks, or the Earthworks project area.
5. Pause. Reopen the Earthworks inspector and compare the same building and vehicle readings. Check the native office/vehicle UI for a changed assignment, parked state, location, or fuel reading if one is visible.
6. If convenient, repeat once with the vehicle removed from the facility. This comparison can reveal whether the machine changes native terrain-tool behavior; it is not required if the first pass is unclear.

## Report

Send the before/after screenshots and note whether the vehicle moved, lost its parked link, acquired an assignment, or showed fuel use. Also note whether the terrain tool behaved differently with the vehicle present.

Any observed movement, assignment, or fuel use means the machine cannot be treated as exclusively available during vanilla terrain work without additional interception. No visible change does **not** prove exclusive reservation: the native tool may use the machine without changing these exposed fields. The probe has no verified native availability or reservation signal, so this check can narrow the risk but cannot by itself clear the reservation blocker.

Do not run this check in a valuable save or with an active Earthworks operation.
