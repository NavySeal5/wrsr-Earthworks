# Stage 4 — remaining feasibility research

Updated 2 November 2026. This note records source inspection only. No game session or in-game test was run for this research.

## Confirmed so far

- The user accepted the 0.5.2 vehicle filter with: “working great now”. The registered Earthworks facility accepts bulldozers, excavators, dumpers and buses, while the excluded vehicle classes tested by the user were refused. The exact test cases were not itemized.
- The user's Stage 4a comparison passed except that they could not find vehicle fuel information. The regular construction-office prototype remains selected; the rail-office alternative is deferred.
- The inspector's vehicle links and parked-list membership are observations, not a native “idle and available to Earthworks” signal. Pinned RepublicMP sources show a raw mechanism job-state field (`vehicle+0x698`) and a route-stop vector (`vehicle+0x680`) used by native checks, but the depot route also checks capability, depot/type compatibility, ownership/home and other state. No general reservation API or fuel-field binding is verified.
- The user wants 10 workers to let one machine reach the speed of player-performed manual terraforming. Partial staffing scales machine throughput proportionally; zero workers stops machines. Workers may contribute without machines. With no machines, worker progress should mirror vanilla manual terrain work from nearby buildings or worker passenger stations. When machines operate, workers only enable/operate them and do not add a separate labor-only output. The station the user recalled is the Labour Hall (also described as the worker pickup hall). Native rate calibration and multi-machine worker allocation remain open.

## Vehicle script evidence

The installed base-game `script.ini` files provide construction skill tokens for specific vehicle types. Examples include:

| Vehicle type | Script token | Value |
|---|---|---:|
| Small bulldozer | `$SKILL_CONSTRUCTION_BULLDOZER` | 9 |
| Large bulldozer | `$SKILL_CONSTRUCTION_BULLDOZER` | 58 |
| EO-302B excavator | `$SKILL_CONSTRUCTION_GROUNDWORKS` | 22 |
| E-10011 excavator | `$SKILL_CONSTRUCTION_GROUNDWORKS` | 33 |
| Large Caterpillar earthmover | `$SKILL_CONSTRUCTION_GROUNDWORKS` | 37 |

The community vehicle scripting guide identifies these as bulldozing and groundworks skill values. The official game's Q&A describes excavator use for native raise/lower and bulldozer use for leveling when construction is not paid for with money. Official Community Report #102 adds manual terrain flattening when machines are unavailable: workers from nearby buildings and worker passenger stations can supply the labor. It also introduces Labour Halls as specialized worker-passenger stations. This supports the user's worker-only baseline, but does not establish labor output per simulation time, whether all terrain actions are supported by labor, how vehicle skill values convert into work per simulation time, or whether parked machines can be reserved for remote projects.

### Productivity relation: what static evidence supports

A June 2024 Steam discussion gives a concrete community interpretation of construction vehicle speed/level ratings: for non-crane mechanisms, the rating represents how many workers at 100% productivity would be needed to match the machine's output; it gives a KT50 bulldozer as equivalent to 22 workers. This is useful as a *candidate unit convention* for ordinary construction work, but it is a player explanation rather than developer documentation, and does not specifically establish the native terrain tool's rate calculation or the `$SKILL_CONSTRUCTION_GROUNDWORKS` token's exact use in terraforming.

The pinned 1.1.1.9 native-code trace confirms that the terrain brush routine (`0x3A8A70`) is called every frame while the tool is selected and the mouse button is held. It reads cursor position, brush size, target height, frame-time-based sound timers, and per-stroke state, then processes affected terrain cells and construction costs. The inspected routine contains no readable worker-count or vehicle-skill symbol, and its direct inputs do not expose a clear “worker-equivalent output” parameter. Its helper calls may conceal some game logic, so this is not proof that no workforce or mechanism modifier exists elsewhere. It does show why a single point-height change over ten real-time seconds cannot be converted into a worker-equivalent rate without knowing frame rate, the brush's spatial profile, terrain slope and tool mode, and the exact nearby labor/mechanism state.

The user's YAG DA-0.25 sample was approximately +4.69 m at one sampled point over ten seconds with the default brush. Since the user could not isolate a worker count and it appeared to draw on city-wide labor, this is an observation of a mixed native-tool setup, not a machine-only rate. The YAG's exact script skill value is also not in the installed base-game vehicle scripts checked so far. Therefore the current static evidence supports the *shape* of a plausible rule—machine output may be represented in worker-equivalent units—but it does not establish a numeric excavator-to-worker ratio for terrain planning.

For continued implementation, the user authorized a provisional value and later balancing. The first model uses **22 worker-equivalent points as one manual-rate unit**: worker-only progress is `workers / 22`; a machine with rating `R` and `W` assigned workers contributes `(R / 22) × min(W / 10, 1)`. A missing machine rating temporarily defaults to 22. Thus one 22-rated machine with 10 workers is set to one manual-rate unit; at partial staffing it scales linearly, and workers do not add another labor-only output alongside it. This is an Earthworks starting rule informed by the community rating explanation and local script values, not a claim that vanilla terrain code uses this formula. Actual volume per simulation time and multi-machine worker allocation remain for later balancing/integration.

## Fuel and range evidence

The official Construction Office reference lists 30 tonnes of internal fuel storage and describes machines driving to construction sites or being transported. The official Q&A says native terrain work can use excavators or bulldozers in a depot or Construction Office within a maximum 1,000 metres. These references describe native construction behavior, not the Earthworks building's operating radius or a remote, stationary-work model.

Community discussion reports an 800 m range for labor and around 1,750 m for machines, while the official Q&A gives an older 1,000 m machine distance. Treat those figures as unverified and potentially version-dependent. The official Community Report describes labor-based manual flattening but does not give a range or rate. The user's Earthworks project range remains open under D08.

The inspector currently has no verified vehicle-fuel field. It should continue to report that fuel is unavailable rather than show an inferred value. The user's inability to locate the fuel reading remains the only recorded in-game evidence for this item.

## Native terrain-edit boundary

The verified `C3D_TERRAIN` exports (`EditRaiseLower`, `EditLevel`, `EditToHeight`, and related calls) mutate terrain immediately. Their signatures accept terrain-edit parameters and return no queued-project or worker-dispatch handle. The current prototype safely applies bounded calls at the game-thread boundary and checks height readbacks. This supports a technical distinction: Earthworks must provide its own project scheduling and progress model, then issue bounded terrain edits; calling a native terrain-edit helper alone will not make assigned facility workers or machines perform the work.

This does not prove that the native UI has no other construction workflow for terraforming. It means no such autonomous job interface has been identified in the verified bindings so far. The native cost-query export (`EditRaiseLowerAngleLimitGetCost`) also does not itself show where costs are charged or how fuel is accounted.

## What remains unproven

- A numeric productivity formula for workers, bulldozers and excavators, or a validated source for project duration estimates.
- The native labor-only rate to mirror.
- How to allocate workers among multiple active machines while retaining the accepted 10-worker/manual-rate and proportional-scaling rule.
- A reliable test for a machine being free from native jobs and other uses.
- A safe exclusive reservation mechanism and how it interacts with native orders.
- The relationship between a dedicated building's parked machine, stored fuel and remote project work.
- Later fuel and quarried-stone accounting; Stage 8's prototype cost rules are now set by D39 to add no fuel, monetary or material costs.

## Next technical step

The Stage 8 readiness probe reports raw vehicle job-state and route-stop readings for bulldozers and excavators, and build 0.8.1 adds a guarded read-only trace at `C3D_TERRAIN::EditRaiseLower`. The user subsequently reported visually seeing the excavator used by vanilla Raise terrain, even though the screenshot still showed unchanged H/A/P links, job state and route stops, and the probe count was zero. Thus unchanged diagnostics do not establish that the machine was idle. A first disassembly pass found the EXE tool dispatcher at `0x312A1E` calling the brush routine at `0x3A8A70`; the latter receives the world pointer and an integer tool mode, not an explicit office or vehicle. It iterates a world-owned pointer vector and calls helpers, but the vector's type and any mechanism-selection logic have not yet been identified. The zero `EditRaiseLower` count therefore remains unexplained; no alternate writer is confirmed. Fuel use and exclusive availability remain unverified. Continue tracing the brush helper calls and world-object vector before designing reservation or autonomous execution. See [Stage 8 readiness notes](STAGE8_READINESS_TESTING.md).

For a user-run native rate comparison, see [Stage 4 productivity instructions](STAGE4E_PRODUCTIVITY_TESTING.md). These instructions do not require a new DLL build.

## References

- [Official game Q&A — terrain work and Construction Office](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Questions_and_answers)
- [Official Report for the Community #102 — worker-based manual terrain flattening and Labour Halls](https://www.sovietrepublic.net/post/report-for-the-community-102)
- [Community discussion clarifying Labour Hall and worker-station terrain work](https://steamcommunity.com/app/784150/discussions/0/686366529729637690/)
- [Official Construction Office reference](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Construction_office)
- [Community vehicle scripting guide — vehicle skill tokens](https://steamcommunity.com/sharedfiles/filedetails/?id=1861143159)
- [Steam community discussion — claimed worker-equivalent interpretation of vehicle speed/level ratings](https://steamcommunity.com/app/784150/discussions/0/4352247346365386302/)
- Local native layout findings: `external/RepublicMP/docs/findings.md`, `external/RepublicMP/determinism/state_hash.h`, and the Stage 4 section in `docs/NATIVE_BINDINGS.md`.
- Native terrain-edit signatures and checked use: `docs/NATIVE_BINDINGS.md`, plus `tools/downloads/terrain-raise.asm`, `terrain-level.asm` and `terrain-toheight.asm`.
