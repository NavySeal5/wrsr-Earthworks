# Stage 4c — compare regular and rail-office worker readings

This is a read-only comparison of two separate test buildings. The existing **Earthworks Development Facility (test)** remains the regular construction-office type. **Earthworks Rail Office Comparison (test)** is a separate copy of the native rail construction office, temporarily set to 5 worker places and retaining its native three rail vehicle stations and rail-only behavior. Do not register the rail comparison as the Earthworks station. This check does not test productivity or authorize a rail-office design for Earthworks.

## Run the comparison

1. Close the game and project-local RML. Run `earthworks-mod\launch-prototype.cmd`, choose **Earthworks Development**, and launch the current **0.5.0-dedicated-facility** plugin. The launcher stages both local test buildings.
2. Load a disposable vanilla-compatible save with no rail construction projects waiting or in progress. Place **Earthworks Rail Office Comparison (test)** near the regular Earthworks test facility. Where possible, make both accessible to the same nearby housing or worker bus stop by footpath, and let workers attempt to reach them.
3. Do not buy or assign a rail construction trainset solely for this comparison. If the rail office does not attract workers without one, record that. Do not assign rail construction or register this comparison as an Earthworks station.
4. Open each building's vanilla panel. Record whether it displays worker capacity and actual worker occupation as separate values. Note any worker arrival/departure behavior.
5. Open **Earthworks → Stage 4 | Read-only workforce inspector → Find buildings**. Select each facility by its displayed ID and origin. After workers have had time to arrive, record the inspector's **Occupant list** count for both. The inspector samples the native building people list; it does not certify productivity.
6. If useful, close Earthworks while using native controls, then reopen and compare the vanilla panel with the inspector again. Avoid terrain-edit buttons; do not use the rail comparison for ordinary construction.

## Report

- Does the rail-office vanilla panel show a worker limit, actual occupation, or both?
- Do workers travel to either facility without assigned vehicles/jobs? What counts do both panels show?
- Does the rail office require a track builder or a rail assignment before workers arrive?
- Any warning, unexpected auto-assignment, or other difference?

The rail office has native rail-only construction and material behavior. Even if this comparison explains the panel discrepancy, it does not prove that its machinery or work system fits Earthworks.
