# Stage 4 — remaining feasibility research

Updated 2 November 2026. This note records source inspection only. No game session or in-game test was run for this research.

## Confirmed so far

- The user accepted the 0.5.2 vehicle filter with: “working great now”. The registered Earthworks facility accepts bulldozers, excavators, dumpers and buses, while the excluded vehicle classes tested by the user were refused. The exact test cases were not itemized.
- The user's Stage 4a comparison passed except that they could not find vehicle fuel information. The regular construction-office prototype remains selected; the rail-office alternative is deferred.
- The inspector's vehicle links and parked-list membership are observations, not a native “idle and available to Earthworks” signal. The pinned RepublicMP source describes native depot dispatch as multiple contextual checks. No general reservation API or fuel-field binding is currently verified.

## Vehicle script evidence

The installed base-game `script.ini` files provide construction skill tokens for specific vehicle types. Examples include:

| Vehicle type | Script token | Value |
|---|---|---:|
| Small bulldozer | `$SKILL_CONSTRUCTION_BULLDOZER` | 9 |
| Large bulldozer | `$SKILL_CONSTRUCTION_BULLDOZER` | 58 |
| EO-302B excavator | `$SKILL_CONSTRUCTION_GROUNDWORKS` | 22 |
| E-10011 excavator | `$SKILL_CONSTRUCTION_GROUNDWORKS` | 33 |
| Large Caterpillar earthmover | `$SKILL_CONSTRUCTION_GROUNDWORKS` | 37 |

The community vehicle scripting guide identifies these as bulldozing and groundworks skill values. The official game's Q&A says native raise/lower requires excavators and native leveling requires bulldozers when construction is not paid for with money. This confirms useful equipment classes and per-type skill metadata in content scripts. It does not establish the runtime field offset, how skill numbers convert into work per simulation time, or whether a parked machine can be reserved for a remote planned project.

## Fuel and range evidence

The official Construction Office reference lists 30 tonnes of internal fuel storage and describes machines driving to construction sites or being transported. The official Q&A says native terrain work can use excavators or bulldozers in a depot or Construction Office within a maximum 1,000 metres. These references describe native construction behavior, not the Earthworks building's operating radius or a remote, stationary-work model.

The inspector currently has no verified vehicle-fuel field. It should continue to report that fuel is unavailable rather than show an inferred value. The user's inability to locate the fuel reading remains the only recorded in-game evidence for this item.

## Native terrain-edit boundary

The verified `C3D_TERRAIN` exports (`EditRaiseLower`, `EditLevel`, `EditToHeight`, and related calls) mutate terrain immediately. Their signatures accept terrain-edit parameters and return no queued-project or worker-dispatch handle. The current prototype safely applies bounded calls at the game-thread boundary and checks height readbacks. This supports a technical distinction: Earthworks must provide its own project scheduling and progress model, then issue bounded terrain edits; calling a native terrain-edit helper alone will not make assigned facility workers or machines perform the work.

This does not prove that the native UI has no other construction workflow for terraforming. It means no such autonomous job interface has been identified in the verified bindings so far. The native cost-query export (`EditRaiseLowerAngleLimitGetCost`) also does not itself show where costs are charged or how fuel is accounted.

## What remains unproven

- A numeric productivity formula for workers, bulldozers and excavators, or a validated source for project duration estimates.
- Whether Earthworks should require workers in addition to machines, or how either should affect progress.
- A reliable test for a machine being free from native jobs and other uses.
- A safe exclusive reservation mechanism and how it interacts with native orders.
- The relationship between a dedicated building's parked machine, stored fuel and remote project work.
- Fuel accounting for the first autonomous version. D09 remains open; fuel must not be silently omitted or charged using a guessed formula.

## Next technical step

Before Stage 7 office assignment or Stage 8 terrain execution, continue read-only tracing of the pinned executable's native terraforming and construction work paths. Identify whether parsed vehicle skills, resource use and dispatch state can be observed or reused through version-checked interfaces. If no native job interface exists, estimate a minimal Earthworks-owned progress model for the user's review without implementing a rate. If a safe availability/reservation path cannot be established, prepare a small user-run diagnostic iteration rather than implementing a guessed rule. Do not launch the game or run in-game tests on the user's behalf.

## References

- [Official game Q&A — terrain work and Construction Office](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Questions_and_answers)
- [Official Construction Office reference](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Construction_office)
- [Community vehicle scripting guide — vehicle skill tokens](https://steamcommunity.com/sharedfiles/filedetails/?id=1861143159)
- Local native layout findings: `external/RepublicMP/docs/findings.md`, `external/RepublicMP/determinism/state_hash.h`, and the Stage 4 section in `docs/NATIVE_BINDINGS.md`.
- Native terrain-edit signatures and checked use: `docs/NATIVE_BINDINGS.md`, plus `tools/downloads/terrain-raise.asm`, `terrain-level.asm` and `terrain-toheight.asm`.
