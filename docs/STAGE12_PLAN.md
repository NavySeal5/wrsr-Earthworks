# Stage 12 — Purpose-built Earthworks facility

**Status:** The Small model is accepted. Its Road Depot prototype has four vehicle slots (D93). The user authorized a Medium variant (D94); its four-bay Blender source, ModelViewer NMF, separate item/config and staging are complete. The Medium target is eight vehicle slots and one loading/unloading station. User-run in-game review of Medium is pending. Fuel storage is omitted for now because it does not appear in the Road Depot UI; the visible tank is decorative. The user confirmed the excavator/bulldozer restriction works; the native purchase dialog still displays all category buttons, while unsupported purchases are refused. Stage 12.3 is in implementation: assigned facilities report eligible parked-machine counts when all readings are valid and otherwise show an unavailable status. User review of the end-to-end integration remains pending.

## Goal

Replace the temporary construction-office-derived test item with a distinctive, placeable Earthworks facility, then connect its native building behavior and player-facing facility controls to the existing Project/Site workflow. The revised prototype is vehicle-only: its parked eligible machines provide Earthworks capacity/output without a worker requirement. Accept only bulldozers and excavators; reject dumpers, buses and every other vehicle type (D92). Preserve automatic recognition, no ordinary construction jobs, one Project per facility and multiple facilities per Project.

The Small concept is an open two-bay equipment canopy beside a compact dispatch office, with a visible decorative tank. Its four vehicle slots are two parallel positions in each bay. The new Medium concept (visual option A) extends the same canopy to four openings; it has eight slots, one loading/unloading station in front of the office and a road connection on the far-right end pointing outward to the right. Its asphalt/roof resource quantities are doubled; office wall quantities are unchanged. Fuel storage is not configured in either prototype. Do not require workers or count labor toward output. Eligible parked machines give capacity; no eligible parked machine means no work. The machine-rate scale remains provisional under the existing rate decision. The earlier 0.5-worker-per-Strength rule (D87) is superseded by the user's vehicle-only direction; the previous 20-worker value was only a Stage 9 test setting.

## 12.1 — Building design, model and placeable item

### Work

1. Prepare a visual proposal before committing the model. Show a game-scale concept with front/side/top views or an equivalent clear layout, including the canopy, dispatch office, two equipment bays, visible decorative tank, access/road-facing sides, footprint and approximate height. Retain the accepted middle concept; use the proposal to settle implementation details rather than reopen the basic concept.
   - Current single-frontage concept: [STAGE12_FACILITY_CONCEPT_PROPOSAL.png](mockups/STAGE12_FACILITY_CONCEPT_PROPOSAL.png). The user asked to return to this initial road/bay layout after reviewing the through-access alternative. It has one road approach along the open bay fronts; final geometry remains subject to actual in-game routing.
   - Through-access alternative (superseded): [STAGE12_FACILITY_CONCEPT_THROUGH_ACCESS.png](mockups/STAGE12_FACILITY_CONCEPT_THROUGH_ACCESS.png).
2. Confirm the installed game's supported asset/export workflow and package requirements. The current development script copies a native construction-office model/materials and writes a Workshop item; it is a staging aid, not a custom-model pipeline. The game includes ModelViewer; see the [official ModelViewer guide](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Modelviewer) and [official modelling notes](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Modelling). The community workflow uses Blender-exported OBJ imported into ModelViewer to create NMF and material files; see the [community modding guide](https://workers-resources.fandom.com/wiki/General_modding).
3. Build a low-poly model and materials that read clearly at normal camera distance and in the construction thumbnail. Prepare a unique item/asset identity and placeable preview icon.
4. Design the vehicle access together with the parking layout. The current `construction_office3.ini` donor defines each `$VEHICLE_PARKING` slot and ordered `$VEHICLE_PARKING_ADVANCED_POINT_PID` waypoints, plus its road connections. Small has four slots in two bays. Medium has eight slots in four bays and one loading/unloading station in front of the office. Its road connection is at the far-right end and points outward to the right. The actual route may require vehicles to reverse out; confirm that in-game before changing the concept.
5. Configure the native Workshop building as `$TYPE_ROADDEPO` so it uses the depot's vehicle-only panel. Keep each variant's parking count and the accepted vehicle classes, and keep ordinary construction jobs blocked. Omit fuel-storage declarations for now; the mesh tank is decorative.
6. Keep footprint, road/access behavior, build cost, maintenance, and demolition behavior documented from native defaults or clearly marked as provisional. Do not invent release balance values during modeling.

### User-run check and gate

Using a fresh disposable world, place and rotate each item. For Medium, check the four-bay silhouette, road connection at the far right facing outward, footprint fit, loading/unloading station in front of the office, construction animation/cost, save/reload and demolition cleanup. With eligible vehicles, check that eight can be parked (two per bay), then try mixed vehicle lengths and arrival order and ensure the single loading/unloading point is outside the garage. The user reviews each model/item before Stage 12.2.

### Exit criteria

- A unique model and Workshop item load without missing assets.
- Both size variants are legible in game; no major clipping, floating, or unusable access orientation.
- Native identity and slot configuration are recorded and survive save/reload.
- The user accepts the appearance and placement behavior.

## 12.2 — Building's native UI

### Work

1. Inspect the actual native building window produced by the Workshop asset. The current construction-office-derived item provides inherited native controls; confirm which controls and readings remain valid for the new item.
2. Prepare a screenshot/mockup of the native panel before changing its UX. Keep normal player-facing controls clear: worker staffing/occupation, accepted or parked machinery, ordinary-job exclusion and building identity. Do not surface developer diagnostics or claims about fuel services.
3. Reuse the native construction-office-style panel if it presents usable staffing and vehicle controls. If the inherited panel hides information or exposes misleading ordinary-job controls, investigate supported ways to correct that before proposing a custom panel.
4. Keep project selection, Site execution, and Project assignment in the Earthworks panel unless the user approves a different division. Avoid duplicating those controls between the building window and Earthworks UI.

### User-run check and gate

Inspect an empty facility, then buy/park representative allowed vehicles. Verify the native depot panel, four slots (two per bay), vehicle purchase/return, and that the facility has no worker requirement or ordinary construction-assignment controls. Confirm that no fuel controls appear; this is expected because fuel storage is omitted for now. The visible tank is decorative. The user reviews the panel before Stage 12.3.

### Exit criteria

- Building name and native controls are understandable and functional for this facility.
- The panel does not advertise services or ordinary jobs the facility cannot provide.
- Worker and machine state is either shown accurately or its native display limitation is documented and addressed in the Earthworks facility UI.
- The user accepts the building-panel experience.

## 12.3 — Earthworks facility and Project integration

### Work

1. Update automatic facility recognition from the development asset name to the released item's unique descriptor. Confirm every placed instance is detected without per-session registration; ordinary construction offices remain unaffected.
2. Connect the item to the existing facility-management area in the Earthworks Projects view. Preserve the user's direction: facility management remains below **New Site**, shows only facilities assigned to the selected Project with add/remove controls, and contains no flavor text.
3. Show facility name/identity, eligible parked machine counts, and missing/unreadable status in the assigned-facility section, but only when readings are reliable. Do not show workers as a requirement or productivity input. If a vehicle reading is unavailable, label it unavailable rather than displaying a misleading zero. Do not expose technical inspector fields or add per-machine detail without confirmation.
4. Preserve project ownership rules: multiple facilities per Project, no facility shared between Projects, explicit add/remove, reassignment only after removal, and saved assignments bound to stable building identity. A missing facility must remain visible as unavailable or clearly explain why capacity is absent; never silently assign a different building.
5. Verify scheduler capacity aggregation with two facilities assigned to one Project, facility exclusivity across Projects, worker/machine counts and concurrent Sites. Verify removing/demolishing a facility follows existing missing-facility handling and preserves project progress.
6. Add player UI only for demonstrated integration state. Keep the workforce inspector out of the player build; temporary terrain-edit and native UI/terrain observation probes have been removed.

### User-run check and gate

On a fresh test save, place at least two Earthworks facilities and an ordinary construction office. Check automatic detection, add/remove assignment, cross-Project exclusivity, save/reload, eligible-machine readings, concurrent Site allocation without workers, and behavior after one assigned facility is removed. Confirm ordinary offices are unchanged and ordinary construction jobs cannot be assigned to either Earthworks facility. User reviews this as Stage 12's final acceptance.

### Exit criteria

- Released facility instances are recognized reliably after placement and save/reload.
- Facility rows and assignment controls reflect actual Project ownership and usable capacity without technical diagnostics.
- Existing scheduler rules continue to work across multiple facilities; no facility is double-counted or shared between Projects.
- User accepts end-to-end building, native UI and Project integration.

## Deferred to Stage 13 or later

Final machine productivity/balance, facility range, build/maintenance costs, fuel-store capacity and fuel consumption/refuelling rules, quarried-stone accounting, and release settings. Stage 12 uses visibly provisional values where a test needs them. Vehicle reservations, machine animations, active-area construction prevention and non-rectangular Sites also remain outside this stage.

## Open decisions to resolve at the relevant checkpoint

- Finish native ModelViewer conversion and staging for the 12.1 single-frontage model, then ask the user to review the placeable item in game before 12.2.
- For 12.2, decide whether the inherited native construction-office panel is sufficient after seeing it with the new building, or whether a custom player-facing panel is necessary and technically supported.
- For 12.3, show eligible-machine counts per assigned facility when readings are reliable; show a clear unavailable state instead of misleading zeroes. Worker counts are no longer an operating prerequisite (D90).
- Keep the machine-rate scale provisional through Stage 12 and review final productivity during Stage 13.
- `$TYPE_ROADDEPO` is the selected native-panel prototype. Fuel storage, consumption and refuelling are deferred (D91).
