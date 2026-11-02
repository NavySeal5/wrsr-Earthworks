# Stage 7 — facility assignment dry-run

Build: **0.7.3-facility-identity**. The game reports the dedicated Workshop building's type descriptor asset name as `9100000012/EarthworksDevelopmentFacility`; this build recognizes that exact string. It also widens the Earthworks panel, allows it to be resized, and wraps long inspector text. It no longer requires the inspector's session registration button. Only the dry-run is implemented; no autonomous terrain work is enabled.

## Initial save/load attempt — 2026-11-02

- The local RML Development project `9100000012` was disabled in the active profile. It was enabled and verified in RML; the dashboard showed **1 Development project(s) active**. The RML local runtime configuration now has `developmentItems["9100000012"] = true`.
- The game was relaunched through RML and loading save **A** (`save\autosave2`) still showed **Missing Workshop item(s)**. The prompt was canceled. No subscription was started, no save was loaded or altered, and the persistence check remains incomplete.
- The Earthworks Development Facility files are present locally, but enabling this local Development project did not satisfy the game's missing Workshop content check. The missing item(s) have not yet been identified.
- Next: inspect which Workshop item IDs the save expects, then repeat the save/reload check in a disposable save where required content is available. Do not overwrite save A.

## Clean-save retry — 2026-11-02

- The user re-saved the world in a cleaner state. It then loaded through RML with no missing-Workshop prompt, and the Earthworks panel opened successfully. The panel reported that saved Earthworks drafts/projects were restored, but there was no queued project to assign.
- Created a 9.9 × 28.2 m test footprint (280 m²), sampled the ground at 6.77 m, and queued project 1. The UI reported **Preview ready. Actual terrain is unchanged** and later confirmed the project was queued.
- The inspector distinguished ordinary construction office ID 15 from Earthworks facility ID 14 (`9100000012/EarthworksDevelopmentFacility`). Project 1 was manually assigned to facility 14. Dry-run showed 0 observed occupants, 0 parked bulldozer/excavator candidates, and provisional rate 0.00. No terrain work ran.
- The user saved the assigned state to a separate slot and reloaded it. Their screenshot shows queued project 1 still assigned to Earthworks facility ID 14, with the inspector recognizing the dedicated facility asset name. The dry-run remains at 0 observed occupants, 0 machine candidates and rate 0.00; the terrain is unchanged. Save/reload persistence is confirmed, and the original clean save was kept separate.

Stage 7 is accepted. The dry-run does not verify actual machine availability, exclusive reservation, work radius, fuel or production balancing; these remain out of scope and prerequisites for autonomous execution.

## Test in a disposable save

1. Close the game and project-local RML. Run `scripts/Stage-Diagnostics.ps1`, then run `launch-prototype.cmd`, confirm **Earthworks Planning - Dedicated Facility Prototype**, version **0.7.3-facility-identity**, and launch through RML.
2. Load a disposable republic containing the Earthworks Development Facility (test). Do not register it; no registration button should appear.
3. Place two Earthworks facilities if convenient. Confirm the inspector labels each placed copy as an automatically recognized Earthworks facility. The candidate row should say **Earthworks facility** and show asset name `9100000012/EarthworksDevelopmentFacility`. Find an ordinary native construction office too; it must be labelled **Other building**.
4. Inspect the ordinary office. Confirm its vehicle purchasing and normal construction controls still behave as before. Inspect an Earthworks facility and confirm ordinary construction assignments are blocked there. Check the allowed vehicle classes (bulldozers, excavators, dumpers, buses) and at least one excluded type.
5. Create and confirm a small terrain plan. Confirm the queued grey wireframe remains visible and the ground does not change.
6. In the Stage 7 panel, click **Find Earthworks facility**, then click near one Earthworks facility on the map. In the inspector below, choose the explicitly labelled Earthworks facility result. Return to the Stage 7 panel and click **Assign selected facility** (or click **Assign this facility now** in the inspector). Confirm the selected facility ID is shown. The distance is to the plan centre and must be presented as distance only, with no in-range/coverage claim.
7. Observe the dry-run counts. Occupants are observed list entries. Parked bulldozer/excavator matches are called candidates, not available or reserved machines. The provisional rate has no completion-time estimate. Change staffing or candidate equipment and confirm only the displayed observations/rate change; terrain must remain unchanged.
8. Save the republic after assignment, return to the main menu and reload that same save. Confirm the queued project and assigned facility ID return, resolve to the correct Earthworks building, and the current read-only counts refresh. If the ID cannot resolve, report the message and do not reassign automatically.
9. If safe in the disposable world, demolish the assigned facility. Confirm the assignment is shown as unresolved and no other facility is silently substituted. Rebuild or choose another facility and assign it manually.
10. Confirm the Earthworks panel opens wider than before, long inspector asset names and status messages wrap instead of running off the right edge, and the panel can be resized or moved during the session.

## Report back

Please report whether identity labels distinguished each Earthworks facility from ordinary construction offices, whether vehicle/job filters affected only Earthworks facilities, whether assignment and ID survived save/reload, what the dry-run showed for occupants/candidates/distance, and whether any terrain or native control changed unexpectedly. Include the RML log if identity resolution or reload failed.

This test does not validate vehicle idleness, availability, native-work competition, facility operating radius, fuel, actual terrain rates or reservation. Those remain blockers before autonomous work.
