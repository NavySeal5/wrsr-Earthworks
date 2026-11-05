# Stage 12.1 — model and access check

This is a user-run check. The agent has not launched the game or edited a save. The prototype is staged as **Earthworks Facility (prototype)**, Workshop ID `9100000012`, with the custom model, preview icon, two parking entries and provisional frontage routing.

The user has accepted the current building model. If checking the current staged config, focus on the new airport-area extension and the parking anchors moved 5 m toward the rear; a fresh placement is required to see these changes.

## Start

1. Close the game and the project-local RML window. Keep Steam open.
2. Run `launch-prototype.cmd` from the `earthworks-mod` folder and start the game from RML. If `building.bbox` or `building.fire` is absent, let the game reach the main menu once so it can generate the files, then close game and RML fully and launch again. The staging script preserves generated bbox/fire data while the NMF model is unchanged.
3. Use a fresh or disposable save; do not use a normal republic save for this test. The RML Development checkbox controls an optional DLL payload, not whether the in-game local Workshop building is loaded.

## Place and inspect

1. In the Roads/Depots category, find **Earthworks Facility (prototype)**. Confirm its preview shows the canopy, dispatch office and tank.
2. Place and rotate it on level ground. Check the ghost preview, footprint, scale from normal zoom, ground contact, office/tank placement, and whether the two canopy bays are open toward the road-facing edge. Dispatch walls use pale concrete from the vanilla construction-office atlas, and the office roof remains unchanged. The canopy roof and tank use local BC1 DDS crops from the construction-office warehouse atlas: the finer, muted gray roof panel and weathered gray-green tank shell. Their imported mesh colors are neutral white so they do not darken the baked textures. Remaining colored parts use compressed Workshop-local DDS textures through `earthworks_facility_material.mtl`. The NMF has no parking or forecourt ground plane: native `$CONNECTIONS_ROAD_DEAD_SQUARE` metadata provides asphalt over the full garage court and turning apron, two adjoining `$CONNECTIONS_AIRPORT_DEAD_SQUARE` rectangles provide the office apron and the marked extension beside it, and `$CONNECTIONS_PEDESTRIAN_DEAD_SQUARE` provides the adjoining gravel-like tank-side patch. All are separate from the `$CONNECTION_ROAD` access point.
3. The generated `building.ini` follows the native construction-office phase sequence across the ten model nodes: Groundworks for the foundation, Brick Laying for the dispatch office, and Rooftop Building for the canopy, tank and roofs. Parking markings are not mesh nodes; the asphalt forecourt uses native building-area metadata. Costs use the installed native office coefficients: Groundworks work 0.0 and ground_asphalt 1.5; Brick Laying work 1.0, wall_brick 0.7 and wall_wood 0.3; Rooftop Building work 1.0 and roof_woodsteel 0.8. The `COST_RESOURCE_AUTO` numbers are coefficients applied by the game to construction geometry, not literal total resource units; actual totals depend on the model nodes. First confirm the build-menu cost summary shows nonzero, reasonable values. Then check that the construction window receives those materials and that a Groundworks vehicle can use the station. Complete the phases and inspect the placed building; the green ghost tint does not show the authored palette. If progress remains at 0%, record the missing resource or vehicle shown by the construction window. See [construction cost notes](STAGE12_CONSTRUCTION_COSTS.md) for tuning details.
4. Open the building window. Confirm the facility loads without missing-model/material warnings and the normal Earthworks controls still recognize it automatically. If the material still looks wrong, close the game and send the latest `log.html` entries mentioning `blankdiffuse`, `material.mtl`, or the Earthworks model.
5. After construction finishes, inspect the **finished** facility. The canopy has no separate modeled ribs; it uses the unmodified corrugated roof texture from the vanilla construction-office warehouse atlas. Compare it directly with the vanilla CO roofing. The tank should look like lighter weathered gray-green steel instead of near-black. The dispatch office walls should be light concrete; the office roof remains dark gray; the framing dark, glazing blue-gray, and safety details restrained red. The two central parking squares and all mesh parking bay markings have been removed; native asphalt now covers the full two-bay court and apron, concrete covers the office-side apron, and the pedestrian-square surface should appear as gravel along the tank-side strip. Construction overlays are not a reliable view of finished material colors.
6. Check that the entry steps rise toward the office door, with the lowest tread furthest away and the highest nearest the threshold.
7. Park a longer eligible vehicle in each bay. The two parking anchors have been moved 5 m toward the rear while retaining their heading toward the opening; confirm the vehicle body stays under the canopy and can enter/leave without hitting the columns.

## Check road access and parking

The updated config points both parking slots toward positive Z and anchors the road connector in the lower-left forecourt. After the previous 2 m adjustment, the user requested another 3 m movement toward the forecourt. Its second point sets the connector direction outward toward the road shown by the user. The paved apron extends farther in front of both bays to leave turning room. Verify the connector side and route in game; the preview model alone cannot confirm native routing.

1. Connect a road to the lower-left access stub beside the garage bays. Check that the stub points left toward the road, stays away from the office door, and the building does not require a second/rear connection.
2. Purchase or assign one allowed machine (bulldozer or excavator) to the facility. Watch it drive from the road into a bay and park without passing through the office, tank, columns or roof.
3. Repeat with a second allowed machine in the other slot. Check both possible arrival orders, their final orientations, and whether one machine blocks the other's route.
4. Send each machine back out. Confirm it can exit to the frontage road without getting trapped. If a vehicle reverses, blocks the other bay, or selects an unexpected road, note the road side and vehicle type.
5. Try one dumper and one bus only if convenient. This pass focuses on model and parking paths; the accepted vehicle allowlist was established earlier and is still enforced by the plugin.

## Persistence and cleanup

1. Save after the building is complete and vehicles are parked, then reload. Confirm the model and parked vehicles remain in place.
2. Clear the two bays and demolish the facility. Confirm no route marker, footprint fragment or model part remains.

## Report

For the original 12.1 model review, report whether the finished facility has the expected muted palette and include a screenshot. If the game log reports missing files, include any lines containing `earthworks_`, `earthworks_facility_material.mtl`, or `model.nmf`. Confirm the office door/steps remain clear of road access, both bays receive and release a machine, and the extended apron provides enough maneuvering room. A screenshot of the native building panel is useful. The road-depot UI/vehicle-only update is covered below.

## Stage 12.2 — Road Depot panel and vehicle-only work

The current prototype uses `$TYPE_ROADDEPO` and two vehicle bays. Fuel storage is omitted; the mesh tank remains decorative. No workers should be required for Earthworks output. The game was not tested by the agent.

1. Close the game and project-local RML, then run `launch-prototype.cmd` and start the game from RML. Use a fresh test save or new placement; existing placed instances will retain their old native type.
2. Find **Earthworks Facility (prototype)** in the Roads/Depots construction category. Check its build-menu card and construction phases still load and the item can be placed.
3. Open its native window before buying vehicles. Confirm it shows a depot/vehicle-management panel, with two usable parking slots and no worker or ordinary construction-assignment controls.
4. Buy one allowed machine and one allowed vehicle type that should be rejected (for example, a road vehicle that is not a dumper or bus). Confirm allowed types can be bought, disallowed types remain blocked by Earthworks, and the allowed vehicle can reach and park in a bay. Return it and repeat in the other bay if practical.
5. Confirm there are no fuel-storage controls in the native panel. This is expected: the prototype has no fuel storage configured, and its visible tank is decorative.
6. Create a Site with a noticeable target-height change, assign this facility to the Project, and explicitly start it. Confirm the Site waits with no eligible machine parked, then runs when an eligible machine is parked, even when no workers are assigned to the building. Check the displayed machine count/progress and confirm a non-Earthworks office's normal controls are unaffected.
7. Save/reload once and confirm the facility is still recognized, its two slots and parked vehicle remain, and the Site resumes only under the normal explicit-start/pause rules.

Report: screenshot of the native facility panel; eligible and rejected purchase examples; how each bay routes; and whether a running Site progresses with a machine but no workers. Fuel functionality is deferred. These checks are for prototype behavior, not final balance acceptance.
