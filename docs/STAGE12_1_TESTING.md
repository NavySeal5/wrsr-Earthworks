# Stage 12.1 — model and access check

This is a user-run check. The agent has not launched the game or edited a save. The prototype is staged as **Earthworks Facility (prototype)**, Workshop ID `9100000012`, with the custom model, preview icon, two parking entries and provisional frontage routing.

## Start

1. Close the game and the project-local RML window. Keep Steam open.
2. Run `launch-prototype.cmd` from the `earthworks-mod` folder and start the game from RML. If `building.bbox` or `building.fire` is absent, let the game reach the main menu once so it can generate the files, then close game and RML fully and launch again. The staging script preserves generated bbox/fire data while the NMF model is unchanged.
3. Use a fresh or disposable save; do not use a normal republic save for this test. The RML Development checkbox controls an optional DLL payload, not whether the in-game local Workshop building is loaded.

## Place and inspect

1. In the construction-office category, find **Earthworks Facility (prototype)**. Confirm its preview shows the canopy, dispatch office and tank.
2. Place and rotate it on level ground. Check the ghost preview, footprint, scale from normal zoom, ground contact, office/tank placement, and whether the two canopy bays are open toward the road-facing edge. The dispatch walls and flush parking apron use UV-mapped regions from the vanilla firestation atlas; the office roof uses another firestation atlas region; the canopy roof uses a vanilla garage metal panel region. Remaining colored parts use compressed Workshop-local DDS textures through `earthworks_facility_material.mtl`. The parking surface is now a thin ground-level concrete texture plane instead of the former raised slab.
3. The generated `building.ini` follows the native construction-office phase sequence, using the custom model's ten NMF mesh nodes: Groundworks for the foundation and bay markings, Brick Laying for the dispatch office, and Rooftop Building for the canopy, tank and roof. Costs now use the installed native office coefficients: Groundworks work 0.0 and ground_asphalt 1.5; Brick Laying work 1.0, wall_brick 0.7 and wall_wood 0.3; Rooftop Building work 1.0 and roof_woodsteel 0.8. The `COST_RESOURCE_AUTO` numbers are coefficients applied by the game to construction geometry, not literal total resource units; actual totals depend on the model nodes. First confirm the build-menu cost summary shows nonzero, reasonable values. Then check that the construction window receives those materials and that a Groundworks vehicle can use the station. Complete the phases and inspect the placed building; the green ghost tint does not show the authored palette. If progress remains at 0%, record the missing resource or vehicle shown by the construction window. See [construction cost notes](STAGE12_CONSTRUCTION_COSTS.md) for tuning details.
4. Open the building window. Confirm the facility loads without missing-model/material warnings and the normal Earthworks controls still recognize it automatically. If the material still looks wrong, close the game and send the latest `log.html` entries mentioning `blankdiffuse`, `material.mtl`, or the Earthworks model.
5. After construction finishes, inspect the **finished** facility. The garage canopy should show a metal panel texture; the dispatch office walls and parking apron should read as vanilla-style concrete; the office roof should be dark gray; the framing dark, glazing blue-gray, and safety details restrained red. Check the parking plane sits flush to surrounding grade. Construction overlays are not a reliable view of finished material colors.

## Check road access and parking

The first config places both bays with their approach heading toward negative Z and puts the straight road connector across that frontage. Treat this as a hypothesis to check; if the road connection lands on another side, record which side the game expects.

1. Connect a road along the open fronts of the bays. Check that a road connection snaps and the building does not require a second/rear connection.
2. Purchase or assign one allowed machine (bulldozer or excavator) to the facility. Watch it drive from the road into a bay and park without passing through the office, tank, columns or roof.
3. Repeat with a second allowed machine in the other slot. Check both possible arrival orders, their final orientations, and whether one machine blocks the other's route.
4. Send each machine back out. Confirm it can exit to the frontage road without getting trapped. If a vehicle reverses, blocks the other bay, or selects an unexpected road, note the road side and vehicle type.
5. Try one dumper and one bus only if convenient. This pass focuses on model and parking paths; the accepted vehicle allowlist was established earlier and is still enforced by the plugin.

## Persistence and cleanup

1. Save after the building is complete and vehicles are parked, then reload. Confirm the model and parked vehicles remain in place.
2. Clear the two bays and demolish the facility. Confirm no route marker, footprint fragment or model part remains.

## Report

For this focused materials check, report whether the finished facility has the expected muted palette and include a screenshot. If the game log reports missing files, include any lines containing `earthworks_`, `earthworks_facility_material.mtl`, or `model.nmf`. The road-facing side, chosen parking slots and vehicle entry/exit can be checked in the same run if convenient. A screenshot of the native building panel is useful. The five-worker value and route coordinates are provisional; do not treat the workforce rule as validated by this check.
