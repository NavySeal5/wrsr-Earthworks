# Stage 4 — manual and worker productivity comparison

This is a user-run vanilla comparison to estimate the rate for the future autonomous system. **It does not test Earthworks automation:** the current build only previews plans, so no registered Earthworks facility will autonomously work on the selected patch. Use the game's native terrain tool for each run. The agent has not opened the game or run these checks. No new Earthworks build is required; use the current 0.5.2 development setup if you want the inspector's worker/vehicle counts.

## What this measures

Compare how much terrain changes during the same timed use of the native terrain tool in three cases:

1. Player-driven control with no nearby labor or earthmoving machines.
2. Worker-only terrain work from a nearby Labour Hall or other worker source.
3. The same worker source plus one nearby excavator.

The official Report for the Community #102 says workers from nearby buildings and worker passenger stations can perform manual terrain flattening when machines are unavailable. It does not publish a rate. This check measures a local baseline; it does not decide the final Earthworks radius, cost or fuel rules.

## Setup

1. Use a new disposable vanilla-compatible save or a separate copy. Do not use a save you care about.
2. Choose three small, similarly flat, unobstructed test patches. Each should be the same size and begin at about the same height. Keep buildings, worker sources and vehicles at least 2 km away from the control patch.
3. Use the same native flatten-to-height action, brush size, target-height difference and game speed for all patches. Keep simulation at 1x and hold the terrain control steady for exactly 60 real seconds on each patch. If 60 seconds is inconvenient, use the same timed interval for all three.
4. To measure a point, open the Earthworks preview, click **Sample height**, then click the same point on the patch. Record the displayed **Target height (m)** value. Repeat at that point after the native terrain-tool run. Sampling only reads terrain and changes the preview target; it does not alter the ground.
5. Keep the terrain-edit financial mode and all other game settings unchanged between patches.

## Runs

1. **Control:** with no nearby workers, passenger stations, excavators or bulldozers, use the native terrain tool for the timed interval. Record the height change. This is a manual-player baseline, not an Earthworks facility run.
2. **Workers only:** place or use a Labour Hall about 100–200 m from the second patch. Arrange for about 10 workers to be present there and confirm the actual count in the inspector if possible. Keep excavators and bulldozers out of the surrounding area. Repeat the same terrain action and interval.
3. **Workers plus machine:** keep the same worker source and count, then place one excavator in a depot/construction office about 100–200 m from the third patch. Your registered Earthworks Development Facility can supply the excavator for this *native manual-tool* comparison; it still will not autonomously modify terrain. Repeat the same action and interval. If you can only test a bulldozer, use the native action it supports and report that separately.

Stop if the tool refuses a run or changes construction/funding mode behavior; report the message and do not change multiple settings to force a result.

## Report back

- Game version and terrain-edit financial mode.
- Actual worker count for the worker-only and worker-plus-machine runs.
- Vehicle type and distance from the patch.
- Before/after height for each patch, or approximate visual change.
- Whether the terrain action was allowed in each case and any on-screen messages.
- Any refusal message or unexpected change in the native terrain tool's behavior.

Keep the results in this test save; no project records or native terrain data are modified by the plugin. Only the normal native terrain tool changes these three patches.

## References

- [Official Report for the Community #102](https://www.sovietrepublic.net/post/report-for-the-community-102)
- The installed Labour Hall definition is `media_soviet/dlc3/buildings/labour_pickup_hall/building.ini`.

## User observation — 2 November 2026

- One YAG DA-0.25 excavator was present. The user used the native terrain tool to raise terrain for 10 seconds using the default brush.
- A sampled point rose from 11.66 m to about 16.35 m (approximately +4.69 m).
- The user could not isolate a 10-worker group; the native operation appeared to use workers across the city.
- This is a useful native-tool observation, but it does not establish autonomous facility productivity, area/volume per time, or the agreed 10-worker comparison. Worker-only productivity remains unmeasured.

For a cleaner worker-only comparison, use a separate low-population disposable republic with roughly 10 total workers near a single Labour Hall and no other workers or earthmoving vehicles within 2 km of the patch. Compare the native worker-only run with one excavator plus the same worker group. If the native tool still draws a wider workforce, record that as the behavior and leave the worker-only rate uncalibrated.

## Developer snapshot — vehicle construction skill lookup

The Developer tab can log the parked vehicles linked to a selected Earthworks facility and read their configured construction-skill values from the installed vehicle `script.ini`. This is configuration evidence, not a measured productivity rate.

The latest snapshot for building 16 found two linked/parked machines:

- `DLC3_service_exvacator_MIIIP` (vehicle ID 21): `$SKILL_CONSTRUCTION_GROUNDWORKS 15`; its `$SKILL_CONSTRUCTION_BULLDOZER 12` line is commented out.
- `DLC3_bulldozer_stalinets_s60` (vehicle ID 23): `$SKILL_CONSTRUCTION_BULLDOZER 15`.

The snapshot initially reported both as unavailable because the diagnostic only searched `media_soviet/vehicles/<type>/script.ini`. DLC vehicle internal names have a `DLC<n>_` prefix, while their definitions live under `media_soviet/dlc<n>/vehicles/<asset-name>/script.ini`. The lookup now resolves that layout, including the two DLC3 vehicles above. Their configured skill values do not by themselves establish their autonomous Earthworks throughput or the game's displayed “Speed/Level” number; that still needs behavioral evidence or further game-code/asset research.
