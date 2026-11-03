# Stage 12 — Purpose-built Earthworks facility

**Status:** Planning. The user split this stage into three parts: the building model/design, the building's own UI, and integration with Earthworks facility management and Projects. Do not begin implementation until the plan and any required visual proposals are reviewed.

## Goal

Replace the temporary construction-office-derived test item with a distinctive, placeable Earthworks facility, then connect its native building behavior and player-facing facility controls to the existing Project/Site workflow. Preserve the accepted Earthworks rules: its own workforce and machine slots; only bulldozers, excavators, dumpers and buses; no ordinary construction jobs; automatic recognition; each facility assigned to at most one Project; multiple facilities may serve one Project.

The selected concept is an open two-bay equipment canopy beside a compact dispatch office, with a visible fuel tank. The tank is decorative in Stage 12. Do not add refuelling, fuel accounting, quarried-stone accounting or balance changes here. The previous test item used 20 workers for a Stage 9 test and accidentally inherited 16 vehicle parking points. The intended Stage 12 prototype is two machine slots; 20 workers may remain a clearly labelled test value. Neither worker capacity nor throughput is the final balance decision.

## 12.1 — Building design, model and placeable item

### Work

1. Prepare a visual proposal before committing the model. Show a game-scale concept with front/side/top views or an equivalent clear layout, including the canopy, dispatch office, two equipment bays, visible decorative tank, access/road-facing side, footprint and approximate height. Retain the accepted middle concept; use the proposal to settle implementation details rather than reopen the basic concept.
2. Confirm the installed game's supported asset/export workflow and package requirements. The current development script copies a native construction-office model/materials and writes a Workshop item; it is a staging aid, not a custom-model pipeline.
3. Build a low-poly model and materials that read clearly at normal camera distance and in the construction thumbnail. Prepare a unique item/asset identity and placeable preview icon.
4. Configure the native Workshop building foundation from the verified office type, with exactly two vehicle parking positions and the accepted vehicle classes. Keep ordinary construction jobs blocked. Use a provisional worker count only for testing; reserve final capacity for Stage 13.
5. Keep footprint, road/access behavior, build cost, maintenance, and demolition behavior documented from native defaults or clearly marked as provisional. Do not invent release balance values during modeling.

### User-run check and gate

Using a fresh disposable world, place and rotate the item. Check silhouette and scale at street and normal play zoom, footprint fit, access orientation, preview thumbnail, construction animation/cost, save/reload and demolition cleanup. Temporarily test the two parking slots and recognized vehicle classes if the native item can be loaded without the final plugin integration. The user reviews the visual result before Stage 12.2.

### Exit criteria

- A unique model and Workshop item load without missing assets.
- The two-bay identity is legible in game; no major clipping, floating, or unusable access orientation.
- Native identity and slot configuration are recorded and survive save/reload.
- The user accepts the appearance and placement behavior.

## 12.2 — Building's native UI

### Work

1. Inspect the actual native building window produced by the Workshop asset. The current construction-office-derived item provides inherited native controls; confirm which controls and readings remain valid for the new item.
2. Prepare a screenshot/mockup of the native panel before changing its UX. Keep normal player-facing controls clear: worker staffing/occupation, accepted or parked machinery, ordinary-job exclusion and building identity. Do not surface developer diagnostics or claims about fuel services.
3. Reuse the native construction-office-style panel if it presents usable staffing and vehicle controls. If the inherited panel hides information or exposes misleading ordinary-job controls, investigate supported ways to correct that before proposing a custom panel.
4. Keep project selection, Site execution, and Project assignment in the Earthworks panel unless the user approves a different division. Avoid duplicating those controls between the building window and Earthworks UI.

### User-run check and gate

Inspect an empty facility, then staff it and park representative allowed vehicles. Verify the native panel's labels, counts, interaction, and ordinary construction controls. Confirm its worker display behavior explicitly; the earlier Workshop-derived prototype showed workers arriving while the vanilla UI omitted worker occupation. Verify the player is not led to expect refuelling from the decorative tank. The user reviews the panel before Stage 12.3.

### Exit criteria

- Building name and native controls are understandable and functional for this facility.
- The panel does not advertise services or ordinary jobs the facility cannot provide.
- Worker and machine state is either shown accurately or its native display limitation is documented and addressed in the Earthworks facility UI.
- The user accepts the building-panel experience.

## 12.3 — Earthworks facility and Project integration

### Work

1. Update automatic facility recognition from the development asset name to the released item's unique descriptor. Confirm every placed instance is detected without per-session registration; ordinary construction offices remain unaffected.
2. Connect the item to the existing facility-management area in the Earthworks Projects view. Preserve the user's direction: facility management remains below **New Site**, shows only facilities assigned to the selected Project with add/remove controls, and contains no flavor text.
3. Show useful player-facing facility information only where readings are reliable. Candidate row information: facility name/identity, available worker count, eligible parked machine counts, and missing/unreadable status. Do not expose technical inspector fields. Confirm before implementing any extra status or per-machine information beyond this scope.
4. Preserve project ownership rules: multiple facilities per Project, no facility shared between Projects, explicit add/remove, reassignment only after removal, and saved assignments bound to stable building identity. A missing facility must remain visible as unavailable or clearly explain why capacity is absent; never silently assign a different building.
5. Verify scheduler capacity aggregation with two facilities assigned to one Project, facility exclusivity across Projects, worker/machine counts and concurrent Sites. Verify removing/demolishing a facility follows existing missing-facility handling and preserves project progress.
6. Add player UI only for demonstrated integration state. Keep developer inspector and terrain probes out of the player build.

### User-run check and gate

On a fresh test save, place at least two Earthworks facilities and an ordinary construction office. Check automatic detection, add/remove assignment, cross-Project exclusivity, save/reload, worker/machine readings, concurrent Site allocation, and behavior after one assigned facility is removed. Confirm ordinary offices are unchanged and normal construction jobs cannot be assigned to either Earthworks facility. User reviews this as Stage 12's final acceptance.

### Exit criteria

- Released facility instances are recognized reliably after placement and save/reload.
- Facility rows and assignment controls reflect actual Project ownership and usable capacity without technical diagnostics.
- Existing scheduler rules continue to work across multiple facilities; no facility is double-counted or shared between Projects.
- User accepts end-to-end building, native UI and Project integration.

## Deferred to Stage 13 or later

Final worker capacity, staffing/productivity balance, facility range, build/maintenance costs, fuel consumption/refuelling, quarried-stone accounting, and release settings. Stage 12 uses visibly provisional values where a test needs them. Vehicle reservations, machine animations, active-area construction prevention and non-rectangular Sites also remain outside this stage.

## Open decisions to resolve at the relevant checkpoint

- Review the 12.1 visual proposal, especially building orientation/access and visible scale, before model production.
- For 12.2, decide whether the inherited native construction-office panel is sufficient after seeing it with the new building, or whether a custom player-facing panel is necessary and technically supported.
- For 12.3, confirm whether the existing assigned-facility list should show live worker and eligible-machine counts per facility. Recommendation: show these counts only if the native reads are reliable; otherwise show a clear unavailable state instead of misleading zeroes.
- Do not set final worker capacity or throughput in Stage 12; make those decisions during Stage 13 balance review.
