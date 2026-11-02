# Stage 7 — scope, evidence and selected behavior

## Proposed bounded deliverable

Assign one queued plan manually to a selected dedicated Earthworks facility, then show a dry-run summary of its staffing and machinery candidates. Recognize all Earthworks facility instances without per-session registration. Show distance to the plan without declaring operating range or coverage. The dry-run will not change terrain, consume fuel or materials, reserve machines, or estimate completion time. It must label parked vehicle matches as *candidates*, because the current native readings do not prove that a vehicle is idle or exclusively available.

The existing test facility uses 5 worker places and 2 machine slots by user choice (D27). These remain developer test values, not a release balance. The inspector's 150 m building-pick distance is only a click-selection aid, not a project operating radius.

## Evidence and limits

- The current workforce probe can read the selected facility's occupant-list count and linked vehicles. Vehicle rows expose home (`H`), assignment (`A`) and parked-list membership (`P`). `P` means present in a native parked list; it does not prove that the vehicle is idle or reserved for Earthworks.
- The probe does not currently read parsed construction skill, fuel, native job readiness or a reservation handle. The accepted vehicle allowlist includes buses and dumpers, but only bulldozers and excavators should contribute groundworks skill output. Until native skill is read, D32's 22-point fallback is an estimate, not a vehicle-specific value.
- The official Q&A says native raise/lower/level terrain machinery must be in a depot or Construction Office within 1,000 m of the work [Official Q&A](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Question_and_answers). The official citizen guide says maximum walking distance is 480 m on the fastest paths; slower routes reduce it [Official citizens/walking reference](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Units). These measure different native behaviors and do not prove the Earthworks station-to-plan radius, especially for the user's no-travel workplace model.
- Stage 4 has not verified exclusive vehicle reservation or whether the native manual terrain tool can still use a vehicle held at the Earthworks facility. Stage 7 can report candidates without claiming exclusivity; autonomous work must wait until the reservation interaction is resolved.

## Decisions and implementation identity

1. **Range display (D37):** User chose distance only, with no operating range or coverage claim.
2. **Project assignment (D38):** User chose manual assignment of one queued project to a chosen facility.
3. **Facility identity (D36):** User chose automatic recognition of every instance, without per-session registration. The user's in-game inspector screenshot showed the full descriptor asset name `9100000012/EarthworksDevelopmentFacility`. An earlier comparison used only the object folder name and therefore did not match; runtime now compares the bounded full string observed in game. Category/subtype alone is not used.
4. **Assignment persistence:** The user's answer to the session-registration question requested permanent facility identity. The project assignment is stored as the native building ID in companion schema v2 and is resolved on reload only when exactly one current building has both that ID and the Earthworks asset name. Schema v1 remains readable as unassigned. If the ID is missing or ambiguous, the dry-run reports unresolved and does not auto-reassign.

## Stage boundary

The user resolved D37 and D38: show distance only, and assign projects manually. D36 requests automatic recognition of every dedicated Workshop facility instance. The implementation uses the exact descriptor asset name, never the generic construction-office category/subtype. The Stage 7 display is a dry-run: it shows observed occupants and parked allowlisted vehicle candidates plus the provisional D32 one-machine rate; it does not describe candidates as reserved/available, write terrain or imply a completion duration. Reservation, fuel and cost accounting remain blockers before Stage 8.
