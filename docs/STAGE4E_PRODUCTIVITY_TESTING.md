# Stage 4 — manual and worker productivity comparison

This is a user-run vanilla comparison for the accepted D31 rule. The agent has not opened the game or run these checks. No new Earthworks build is required; use the current 0.5.2 development setup if you want the inspector's worker/vehicle counts.

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
4. Record each patch's starting and ending height with the Earthworks height readout/preview. If you cannot get a reliable height readout, use the visible result and mark the measurement approximate.
5. Keep the terrain-edit financial mode and all other game settings unchanged between patches.

## Runs

1. **Control:** with no nearby workers, passenger stations, excavators or bulldozers, use the native terrain tool for the timed interval. Record the height change.
2. **Workers only:** place or use a Labour Hall about 100–200 m from the second patch. Arrange for about 10 workers to be present there and confirm the actual count in the inspector if possible. Keep excavators and bulldozers out of the surrounding area. Repeat the same terrain action and interval.
3. **Workers plus machine:** keep the same worker source and count, then place one excavator about 100–200 m from the third patch. Repeat the same action and interval. If you can only test a bulldozer, use the native action it supports and report that separately.

Stop if the tool refuses a run or changes construction/funding mode behavior; report the message and do not change multiple settings to force a result.

## Report back

- Game version and terrain-edit financial mode.
- Actual worker count for the worker-only and worker-plus-machine runs.
- Vehicle type and distance from the patch.
- Before/after height for each patch, or approximate visual change.
- Whether the terrain action was allowed in each case and any on-screen messages.
- Whether you want worker output to continue in addition to machine output while machines are active. D31 sets the staffing rules but leaves additive output open.

Keep the results in this test save; no project records or native terrain data are modified by the plugin. Only the normal native terrain tool changes these three patches.

## References

- [Official Report for the Community #102](https://www.sovietrepublic.net/post/report-for-the-community-102)
- The installed Labour Hall definition is `media_soviet/dlc3/buildings/labour_pickup_hall/building.ini`.
