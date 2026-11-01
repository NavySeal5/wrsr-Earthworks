**Gameplay and UX decision register**

Updated 1 November 2026. This register separates the user's confirmed intent from proposed implementation rules. A recommendation or prototype value is not an accepted decision.

**Confirmed decisions**

| ID | Decision | Evidence |
|---|---|---|
| D01 | Republic Mod Loader is the primary host | User: "Lets stay with RML then" |
| D02 | Plan terrain changes, confirm a project, then carry out work autonomously; workers/machines need not travel to each site | Original concept and user's remote-station clarification |
| D03 | First planning prototype: select an area and enter or sample a target height | User explicitly selected this option during the plan review |
| D04 | Important gameplay/UX choices need user input; each stage has an in-game check and review before progression | User's request to review the plan and work in stages |
| D16 | Continue through the first planning prototype without intermediate acceptance stops; retain in-game verification and gameplay/UX questions | User: "Continue until the first prototype of the terraforming planning is finished" |
| D17 | Rectangular selection uses two opposite-corner clicks; preview uses coloured wireframe with cut/fill shading; retain sharp edges for this prototype | User explicitly selected all three options |
| D18 | Open planning with an on-screen Earthworks button only; no hotkey | User explicitly selected this option |
| D19 | First planning prototype accepted after the supplied in-game checklist, including final menu-transition/reload checks | User, 1 November 2026: "All checks passed, Whats next on the plan"; build 0.2.0-prototype |
| D20 | Proceed with Stage 3's small raised rectangle, followed by a lowering test, in a disposable fixture | User: "Continue then", followed by "continue"; accepts the proposed temporary terrain experiment, not production work rates/costs |
| D21 | All further in-game tests are performed by the user | User: "Dont do the tests yourself, just provide me with test instructions" |
| D22 | Stage 3 accepted; proceed to Stage 4's feasibility diagnostic | User: "Seems to work, next step now", then completed remaining checks: "Both deltas are 0, road planning & building over it felt just like normal" |
| D23 | Stage 4a inspector checks accepted except vehicle fuel could not be found | User: "Can confirm everything except vehicle fuel, did not find anything regarding that"; confirms the checklist without providing numeric readings |
| D25 | Earthworks uses its own building with its own worker and machinery slots; do not draw machines from ordinary construction offices | User: "Earthworks should not use vehicles in construction offices, there should be its own building with worker slots + machine slots" |
| D26 | The Earthworks building must be blocked from ordinary construction jobs | User selected "Block normal jobs (Recommended)" for the dedicated building's dispatch behavior |
| D27 | Stage 4b test building has 5 worker places and 2 machine stations | User selected these temporary prototype values; not a release balance decision |
| D28 | Compare a separate rail-office type to investigate native worker occupation display | Initially requested by the user, then superseded by D29 before testing |
| D29 | Keep the regular construction-office prototype for now; defer the rail-office alternative as overkill | User: "lets just stick with the construcction office prototype for now, the RCO seems overkill" |
| D30 | The Earthworks building must accept only bulldozers, excavators, dumpers, and buses; exclude every other vehicle class | User: "The building should only accept Bulldozers, excavators, dumpers, and busses". After 0.5.1 false negatives were corrected in 0.5.2, the user accepted the result: "working great now". Specific transfer and ordinary-office checks were not itemized |
| D31 | 10 workers let one machine reach the speed of player-performed manual terraforming. Machine throughput scales proportionally with partial staffing and stops with zero workers. Workers can work without machines; worker-only progress should mirror vanilla worker-based terrain work supplied by nearby buildings/passenger stations, including the Labour Hall/worker pickup hall. When machines operate, workers only enable/operate them and do not add separate labor-only output | User, 2 November 2026. Official Community Report #102 confirms worker-based manual terrain flattening from nearby buildings and worker passenger stations, and adds Labour Halls. Native rate calibration and multi-machine worker allocation remain to be resolved before execution implementation |
| D32 | Provisional productivity scale: treat groundworks skill/rating points as worker-equivalent output, using 22 points as one manual-terrain-rate reference. Worker-only output is workers/22 of that reference rate. A machine's output is (rating/22) times the reference rate, scaled by assigned workers/10 and capped at full staffing; workers do not add a second output while operating it. Use the native machine rating when available and 22 as the initial fallback for an eligible machine whose rating cannot be read. Revisit the numbers during balancing | User authorized choosing a value from current evidence and deferring balancing. Evidence: community interpretation says non-crane construction vehicle ratings correspond to worker-equivalent output and gives a 22-rated KT50 bulldozer as 22 workers; installed excavator scripts include groundworks rating 22. This is a provisional normalization, not a verified terraforming formula |

D17/D18 settle the first prototype's selection, preview, sharp edges and opening control. They do not settle released tool scope, office behaviour, project persistence or autonomous execution. The D16 batch authorization ends with the accepted planning prototype; normal review gates resume.

**Open decisions — ask before dependent implementation**

| ID | Topic and useful options to discuss | Ask by | Current status |
|---|---|---|---|
| D05 | Operation representation: dedicated combined building, adapt an existing building, or explicitly linked workplace/yard | Before Stage 7 | Resolved by D25: dedicated building with its own worker and machine slots; capacities and presentation remain open |
| D06 | Area selection controls/shapes, preview style, elevation units, initial flatten/raise/lower scope and how edges join the terrain | Before expanding Stage 5 scope | Prototype resolved by D17/D18 and accepted via D19: flat rectangle, heights in metres, cut/fill wireframe, sharp edges and button only. Expanded tools remain part of D14 |
| D07 | Staffing/productivity and machine eligibility: what counts as available; how native assignments and reservations interact | Before Stage 7 | Provisional rate scale resolved by D32; D31 staffing/output rules retained. Actual terrain units per simulation time, machine availability, allocation across multiple machines and exclusive reservation remain open for balancing/integration |
| D08 | Operating range: distance rule, partially covered plans, capacity/area limits and player assignment | Before Stage 7 | Open. No radius or slot count approved |
| D09 | First autonomous-version accounting: fuel, monetary costs, abstract or balanced cut/fill, and any soil/material requirements | Before Stage 7 | Open. Omitting fuel or abstracting fill is not an approved production rule |
| D10 | Work-rate model and visual execution: worker/machine bottleneck, progression pattern, displayed estimates and desired duration | Before Stage 8; tune in Stage 10 | Provisional relative formula selected by D32: 22 worker-equivalent points define one manual-rate unit; 10 workers fully staff one machine; partial staffing scales linearly. Volume/time, simulation-time conversion, progress presentation and balance remain open |
| D11 | Pausing/cancelling partial work, restoration requests and the workflow after external terrain changes | Before Stage 8 | Open. No free rollback or permanent-partial-work rule has been chosen |
| D12 | Draft persistence and recovery interaction: which drafts survive, how missing/mismatched metadata is explained and recovered | Before Stage 6 | Open. Refusing unsafe automatic terrain writes remains an engineering requirement |
| D13 | Multiple-project rules: sequential/shared work, priorities, blocked jobs, auto-assignment, overlap and reassignment | Before Stage 9 | Open. Single active project and priority queue are earlier proposals |
| D14 | Candidate scope/balance: final rates, capacities/radius, tool set, practical limits, completion feedback and intended gameplay settings | Before Stage 10 | Open. Revisit earlier choices using real in-game results |
| D15 | Public release/distribution and any cloud or other-mod promises | After candidate review | Open. Packaging a local candidate does not authorize publishing |

The earlier values of 20 workers, four machines, 1,000 metres and 5 cm remain unapproved examples. Terrain completion tolerance also depends on measured native precision; present any visible precision/shape tradeoff before adopting it.

**How to request a decision**

Ask a small number of relevant questions before the feature depends on them. Include the observed technical constraints, two or three concrete options where useful, and a recommendation with its tradeoff. For an office fallback, show why the preferred representation is unsupported before proposing a replacement.

Do not ask every future design question before Stage 1. Stages 1–4 can collect evidence using clearly labelled developer controls without deciding the final player experience. Technical experiments do not approve their temporary mechanics for release.

If an answer changes a previous choice, record the new decision and affected stages. Do not erase the reason for the revision or silently proceed using the older rule.

**Decision entry template**

- ID/topic:
- Stage affected:
- Evidence and constraints:
- Options and recommendation presented:
- User's response:
- Accepted behaviour and remaining limits:
- Date and affected files:

Unanswered questions remain open. The first preview, Stage 3 and Stage 4a are accepted. The user chose a dedicated Earthworks building with its own workers and machines (D25), instructed the plugin to block ordinary construction jobs from it (D26), selected temporary 5-worker/2-machine capacities for this test item (D27), retained the regular construction-office type (D29), and specified the sole accepted vehicle classes (D30). The user found false negatives in build 0.5.1; build 0.5.2 corrects the known category and ZiS-585 name cases, and the user reports it works great. Modded vehicles with unusual names may need more robust classification. Release balance and visual design remain open. Stage 4 is still in progress. D31 sets the staffing direction, proportional partial staffing and non-additive worker behavior during machine work; native rate calibration and multi-machine allocation remain open. Fuel was not found during Stage 4a and remains unmeasured. Native dispatch availability and exclusive reservation remain open. The agent must not perform in-game tests for the user or infer future cost/rate/persistence choices.
