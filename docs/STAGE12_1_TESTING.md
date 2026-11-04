# Stage 12.1 — model and access check

This is a user-run check. The agent has not launched the game or edited a save. The prototype is staged as **Earthworks Facility (prototype)**, Workshop ID `9100000012`, with the custom model, preview icon, two parking entries and provisional frontage routing.

## Start

1. Close the game and the project-local RML window. Keep Steam open.
2. Run `launch-prototype.cmd` from the `earthworks-mod` folder. The launcher stages the current plugin and Earthworks Workshop item, then opens the isolated RML. Start the game from that RML window.
3. Use a fresh or disposable save. Do not use a normal republic save for this test.

## Place and inspect

1. In the construction-office category, find **Earthworks Facility (prototype)**. Confirm its preview shows the canopy, dispatch office and tank.
2. Place and rotate it on level ground. Check the ghost preview, footprint, scale from normal zoom, ground contact, office/tank placement, and whether the two canopy bays are open toward the road-facing edge. This iteration uses an intentionally high-contrast diagnostic palette: cream office, blue roof, dark blue structure, red accents, blue glass, and green tank. These are temporary test colors, not the proposed final art palette. For the material-path diagnostic, the MTL is copied both to the Workshop item root and beside `model.nmf`, and the renderconfig references the beside-model copy as `MATERIAL material.mtl`, matching installed Workshop building items.
3. The generated `building.ini` assigns all ten NMF mesh nodes to one small Groundworks phase, with a work station attached to `mesh_foundation_concrete`. The construction phase is listed before the `RESOURCE_SOURCE` entries, following the native construction-office layout. For this diagnostic it requires **1.0 ground_asphalt, 1.0 wall_brick, 1.0 wall_wood, 1.0 roof_woodsteel**, and 1.0 work. These temporary test values are not the final facility balance. First confirm the build-menu cost summary shows nonzero values. Then check that the construction window receives those materials and that a Groundworks vehicle can use the station. Complete the phase and inspect the placed building; the green ghost tint does not show the authored palette. If progress remains at 0%, record the missing resource or vehicle shown by the construction window.
4. Open the building window. Confirm the facility loads without missing-model/material warnings and the normal Earthworks controls still recognize it automatically. If the material still looks wrong, close the game and send the latest `log.html` entries mentioning `blankdiffuse`, `material.mtl`, or the Earthworks model.

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

Please report the road-facing side the game used, which slots each vehicle chose, whether both could enter and exit, and any clipping, warnings or stuck vehicles. A screenshot of the placed building and the native panel is useful. The five-worker value and route coordinates are provisional; do not treat the workforce rule as validated by this check.
