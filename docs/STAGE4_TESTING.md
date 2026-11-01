**Stage 4a — first read-only workforce and vehicle comparison**

Build: **0.4.0-worker-probe**. DLL SHA-256: `af750fe923e6e2cc4a4a81ce4ce31c8369830ad72371f09d323a1a871ce51730`.

This iteration observes native data. It does not reserve vehicles, assign jobs, change staffing or perform autonomous terrain work. The separate Stage 3 terrain-test buttons remain available only in S03 and are not part of this checklist.

**Launch and fixture**

1. Close both the game and the project-local RML window. Keep Steam open. Run `earthworks-mod\launch-prototype.cmd` and select **Earthworks Development**. Confirm **Earthworks Planning - Workforce Inspector**, version **0.4.0-worker-probe**, before launching from RML. Reusing an already-open launcher does not stage the new DLL.
2. Use a disposable **vanilla-compatible** save with a staffed workplace and a construction office holding a bulldozer or excavator. The profile disables Workshop content. If S03 is empty, add the test buildings there, or use a disposable vanilla copy saved under a separate name such as Earthworks_Test_S04. Keep the usual native means of supplying workers; this probe does not supply them. The workplace and office are separate diagnostic specimens, not a decision to make the final mod use two buildings.

**Staffing comparison**

3. Open **Earthworks → Stage 4 | Read-only workforce inspector → Find buildings**, then click near the centre of the workplace. Choose its ID from the nearest origins shown. The pick radius is 150 m for this diagnostic only. Check the coordinates and magenta origin marker to confirm the right building. If several overlap, select a less crowded specimen. The numeric category/subtype are raw diagnostic values, not named building types.
4. Note the native workers-present value and inspector **Occupant list** value. Close Earthworks to operate native controls and let workers arrive/leave; reopen Earthworks and compare again after about one second. Repeat for at least two different actual headcounts, ideally including zero. The inspector continues refreshing while its panel is closed. Use **Log inspector snapshot** for each comparison and note the corresponding native value. Worker limits are not actual headcounts. At residential buildings the same list can represent residents; don't use a house to certify worker availability. Productivity and capacity explicitly remain unverified.

**Vehicle comparison**

5. Use **Find buildings** again to inspect the construction office. Match the displayed internal vehicle names/IDs with its native inventory. Record a snapshot with a bulldozer/excavator parked. Native list slots and readable linked vehicle counts are shown separately. The list includes all linked vehicle types; the inspector does not classify eligible earthmoving machines.
6. Close Earthworks and use normal native controls to send one machine to an ordinary construction job. Reopen while it is travelling/working; record the H/A/P flags and distance. Repeat after its return. If practical, move one machine to another office, then sell a test vehicle and check that the old office's links update. Report which changes you actually checked. **H** is a home link; **A** is an assignment/owner link; **P** is parked-list membership. The flags overlap and must not be added together. Neither proximity nor P alone proves that a machine is idle or usable.

**Lifecycle and normal controls**

7. With a building selected, save and reload the disposable world. The inspector must return to an unselected state; select the building again. On a separate expendable building, select it, close Earthworks, demolish it, then reopen: the inspector must invalidate the selection. A replacement must require a fresh selection. If demolition is not practical, report this step as not tested.
8. Confirm the normal building controls and camera work after closing Earthworks, and planning still allows two-corner selection and a height preview. The inspector should report an unreadable field as **UNAVAILABLE** or a vehicle scan as **INCOMPLETE**, rather than claiming zero available workers/machines. Up to 48 readable linked vehicle rows are shown; truncation is explicitly reported.

**Report back**

- Workplace type, two or more native worker counts and corresponding occupant readings.
- Machine type and H/A/P flags when parked, away and returned; any transfer/sale observations.
- Reload cleared selection? Demolition invalidated it? Any unavailable/incomplete readings or control problems?

Snapshots are in `tools/runtime/rml/rml/logs/rml-runtime.log`, prefixed `earthworks  stage04`. No automatic log file containing raw engine pointers is created. No in-game test of this build has been performed by the agent.

After review, continue Stage 4 by resolving the meaning of these readings, effective productivity/skills and a possible equipment reservation mechanism. Native/manual competition for a machine remains unproven and blocks autonomous execution. Only after that evidence is available should we ask the user to choose the office model and availability rules.
