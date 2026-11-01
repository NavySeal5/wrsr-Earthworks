# Stage 4d — vehicle allowlist test

Build: **0.5.1-vehicle-allowlist**. This is a test iteration for the temporary Earthworks Development Facility in Workshop item `9100000012` (5 worker places, 2 vehicle stations). It keeps the native construction-office shell and filters vehicles only while this exact facility is registered in the current loaded world.

The first classifier uses internal type names and native categories. It recognizes `bulldozer`/`buldozer` and `excavator`/`exvacator` in road-service category 2; `dumper`, `dump_truck`, `dumptruck`, `gravel_`, and `bus`/`bust` in road category 1. Unreadable or unmatched types are rejected. Mod vehicles with other names may be rejected even if they have the corresponding skill.

## Start

1. Close the game and project-local RML. Start Steam.
2. Run `earthworks-mod\launch-prototype.cmd` from the game folder.
3. Confirm profile **Earthworks Development**, plugin **Earthworks Planning - Dedicated Facility Prototype**, version **0.5.1-vehicle-allowlist**. Launch from RML.
4. Load the disposable `Earthworks_Test_S01` world. Make sure the local Earthworks Development Facility item is available and placed. Do not register an ordinary construction office as the Earthworks station.
5. Open Earthworks, use **Find buildings**, select the test facility, then register it. The message should say named bulldozers, excavators, dumpers and buses only.

## Check accepted and excluded types

The test building has two vehicle stations, so sell a vehicle before trying more types if both stations are occupied. Avoid buying a new test vehicle with autopurchase if you are unsure whether the selected building is the facility.

1. From the registered facility's native vehicle-purchase UI, try one base-game example from each class:
   - bulldozer: Stalinec bulldozer
   - excavator: EO-302B excavator
   - dumper: IFA W50 gravel/dump truck
   - bus: Ikarus 66 bus
2. Record whether each appears, can be selected, and is successfully purchased into the facility. The expected result is that the native purchase path allows each type.
3. Try one unrelated road cargo truck and one unrelated road-service vehicle (for example, a fire engine). The expected result is that the facility refuses them. If a forbidden vehicle remains selectable but purchase is refused, record that distinction.
4. If the test profile offers a way to transfer an already-owned vehicle into a construction office, attempt one forbidden type and one allowed type. Report whether transfer respects the same filter; do not move a vehicle from an ordinary office if it would disrupt that office's setup.
5. Use the same purchase checks at an ordinary construction office. Its behavior should remain unchanged.
6. Close and reopen Earthworks and confirm the filter remains active while the test facility is registered. If you reload the world, registration is session-only: register the facility again before repeating the filter checks.

## Report

Please send the exact vehicle names tested and, for each, whether it appeared in the list, could be selected, was accepted by the station, or could be transferred in. Include any unexpected effect at an ordinary construction office. Do not treat the classifier as accepted until these in-game checks are reviewed. The test modifies only the disposable save; sell test vehicles before saving if you want the original fixture unchanged.

The user performs all in-game checks. No gameplay test was run by the agent for this build.
