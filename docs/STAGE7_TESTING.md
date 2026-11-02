# Stage 7 — facility assignment dry-run

Build: **0.7.1-assignment-ux**. This build automatically recognizes buildings whose type descriptor asset name is exactly `EarthworksDevelopmentFacility`. It no longer requires the inspector's session registration button. Only the dry-run is implemented; no autonomous terrain work is enabled.

## Test in a disposable save

1. Close the game and project-local RML. Run `launch-prototype.cmd`, confirm **Earthworks Planning - Dedicated Facility Prototype**, version **0.7.1-assignment-ux**, and launch through RML.
2. Load a disposable republic containing the Earthworks Development Facility (test). Do not register it; no registration button should appear.
3. Place two Earthworks facilities if convenient. Confirm the inspector labels each placed copy as an automatically recognized Earthworks facility. Find an ordinary native construction office too; it must be labelled **Other building**.
4. Inspect the ordinary office. Confirm its vehicle purchasing and normal construction controls still behave as before. Inspect an Earthworks facility and confirm ordinary construction assignments are blocked there. Check the allowed vehicle classes (bulldozers, excavators, dumpers, buses) and at least one excluded type.
5. Create and confirm a small terrain plan. Confirm the queued grey wireframe remains visible and the ground does not change.
6. In the Stage 7 panel, click **Find Earthworks facility**, then click near one Earthworks facility on the map. In the inspector below, choose the explicitly labelled Earthworks facility result. Return to the Stage 7 panel and click **Assign selected facility** (or click **Assign this facility now** in the inspector). Confirm the selected facility ID is shown. The distance is to the plan centre and must be presented as distance only, with no in-range/coverage claim.
7. Observe the dry-run counts. Occupants are observed list entries. Parked bulldozer/excavator matches are called candidates, not available or reserved machines. The provisional rate has no completion-time estimate. Change staffing or candidate equipment and confirm only the displayed observations/rate change; terrain must remain unchanged.
8. Save the republic after assignment, return to the main menu and reload that same save. Confirm the queued project and assigned facility ID return, resolve to the correct Earthworks building, and the current read-only counts refresh. If the ID cannot resolve, report the message and do not reassign automatically.
9. If safe in the disposable world, demolish the assigned facility. Confirm the assignment is shown as unresolved and no other facility is silently substituted. Rebuild or choose another facility and assign it manually.

## Report back

Please report whether identity labels distinguished each Earthworks facility from ordinary construction offices, whether vehicle/job filters affected only Earthworks facilities, whether assignment and ID survived save/reload, what the dry-run showed for occupants/candidates/distance, and whether any terrain or native control changed unexpectedly. Include the RML log if identity resolution or reload failed.

This test does not validate vehicle idleness, availability, native-work competition, facility operating radius, fuel, actual terrain rates or reservation. Those remain blockers before autonomous work.
