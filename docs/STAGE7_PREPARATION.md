# Stage 7 — office assignment dry-run preparation

## Proposed bounded deliverable

Assign one queued plan to the session-registered dedicated Earthworks facility, then show a dry-run summary of its staffing and machinery candidates. The dry-run will not change terrain, consume fuel or materials, reserve machines, or estimate completion time. It must label parked vehicle matches as *candidates*, because the current native readings do not prove that a vehicle is idle or exclusively available.

The existing test facility uses 5 worker places and 2 machine slots by user choice (D27). These remain developer test values, not a release balance. The inspector's 150 m building-pick distance is only a click-selection aid, not a project operating radius.

## Evidence and limits

- The current workforce probe can read the selected facility's occupant-list count and linked vehicles. Vehicle rows expose home (`H`), assignment (`A`) and parked-list membership (`P`). `P` means present in a native parked list; it does not prove that the vehicle is idle or reserved for Earthworks.
- The probe does not currently read parsed construction skill, fuel, native job readiness or a reservation handle. The accepted vehicle allowlist includes buses and dumpers, but only bulldozers and excavators should contribute groundworks skill output. Until native skill is read, D32's 22-point fallback is an estimate, not a vehicle-specific value.
- The official Q&A says native raise/lower/level terrain machinery must be in a depot or Construction Office within 1,000 m of the work [Official Q&A](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Question_and_answers). The official citizen guide says maximum walking distance is 480 m on the fastest paths; slower routes reduce it [Official citizens/walking reference](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Units). These measure different native behaviors and do not prove the Earthworks station-to-plan radius, especially for the user's no-travel workplace model.
- Stage 4 has not verified exclusive vehicle reservation or whether the native manual terrain tool can still use a vehicle held at the Earthworks facility. Stage 7 can report candidates without claiming exclusivity; autonomous work must wait until the reservation interaction is resolved.

## Decisions needed before implementation

1. **Range rule:** Choose a single radius for all facility contributions, separate worker-only and machine-assisted limits, or defer range display until tested. If a plan crosses the chosen limit, decide whether to mark the whole plan out of range or work only the covered part. Recommendation: use a single provisional 1,000 m limit for machine-assisted dry-runs, because that is the official native terrain-machine distance; label worker-only coverage unverified and ask the user to validate before autonomous use.
2. **Assignment:** Choose manual assignment of a selected queued project to the currently registered facility, or automatic assignment to the closest eligible registered facility. Recommendation: manual assignment for Stage 7's one-facility dry-run so the player controls which dedicated station's capacity the panel displays.
3. **Assignment persistence:** Registration currently resets on world reload and no stable facility link has been verified. Choose whether the dry-run assignment is session-only, clearing on reload while leaving the queued project intact, or whether to investigate a saved facility link. Recommendation: keep assignment session-only for this prototype.

## Stage boundary

Even after those choices, the Stage 7 display is a dry-run. It may show actual observed occupants and parked allowlisted vehicle candidates, plus the provisional D32 rate. It must not describe candidates as reserved/available, write terrain, or imply a completion duration. Stage 7 user checks should verify assignment display, range status and changing counts without terrain edits. Reservation, fuel and cost accounting remain explicit blockers before Stage 8.
