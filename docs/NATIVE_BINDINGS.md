# Planning prototype native bindings

Supported game: 1.1.1.9, Steam build 23935965. Binary SHA-256 identities are in game-baseline.json. Staging refuses changed binaries; runtime checks the executable PE timestamp, import availability and exact inline prologues before installation. This is a build-specific prototype.

Reference: RepublicMP commit 630775724abd5e3346eb59f9c66e2873128d74a4, game/world.cpp, game/camera.cpp, game/input_block.cpp, determinism/sim_patches.cpp, determinism/determinism.cpp and world/terrain_hook.cpp. Prologues were independently compared with the installed executable using MSVC dumpbin.

| Binding | Location / signature | Use |
|---|---|---|
| Main frame Late | EXE+0x30D1A0, void(int64 world) | Observe after native cursor picking, process queued commands |
| World load | EXE+0x430FC0, void() | Invalidate session draft and queued commands before native loading |
| World | Inline EXE+0x9D4F10 | Cursor +0xF78; day/year/progress +0x590/594/59C |
| Readiness | Road-network pointer EXE+0x9E46B0 and byte EXE+0x993C65 | Gate all planning queries |
| Camera | Pointer EXE+0x9EAD10 | Copy view matrix, projection at +0xC8 |
| Terrain | *( *(EXE+0x9941F0) +0xED8 ) | Read-only height-query object |
| Height | C3DDLL64 CollisionTerrainHeightOverPoint | float(void* terrain, const float* Win64 by-value vector copy) |
| Input | 13 verified C3D_INPUT imports in EXE | Consume native input, suppress only overlay-owned input |
| Left input latches | EXE+0xA54E90/91 | Clear while planning owns left clicks |
| Escape latch | EXE+0xA54C91 | Clear only while text input owns keyboard |
| Device creation | C3DDLL64 IAT D3D11CreateDeviceAndSwapChain | Observe actual swap-chain creation after the original returns |
| Present | IDXGISwapChain vtable slot 8 | Render overlay using copied snapshots |
| Construction assignment filter | Inline EXE+0x161C90, `void(world, office, construction, amount, first, second)` | Block assignment only when `office` resolves to the dedicated Earthworks building type descriptor; otherwise call the original |
| Vehicle station slot filter | Inline EXE+0x3E3F40, `bool(world, building, vehicleType, slotHint, int* slot)` | For the exact dedicated type descriptor, reject names outside the initial bulldozer/excavator/dumper/bus patterns; ordinary construction offices forward unchanged. Installation requires the exact pinned 17-byte instruction-aligned prologue |
| Native save boundary | World save path at `world+0x9F8`; save trigger at `world+0x14D35`; observed through the existing Late frame hook | Detect a native save completing, then write an atomic `earthworks.dat` companion in that save folder. On world load, restore only after its SHA-256 fingerprint matches the saved `heightmap.dds` |

In the accepted 0.2.0 preview, terrain/camera reads have SEH guards and no terrain-write path exists. In 0.3.0 the ordinary planning controls remain read-only, with the additional fixture-restricted native experiment described below. Planning math uses copied samples; all terrain calls occur in the native frame callback. Commands carry a world-generation token; a new load drops stale work and undo history. Stage 6 stores drafts/projects in a versioned companion file with a checksum and terrain fingerprint; no native pointer or reservation is persisted. The UI restores companion data only for an exact saved-heightmap match. Rendering also gates snapshots on the two fixed-address readiness scalars because Late stops running in menus; it does not follow terrain/camera pointers.

The rendering code does not retain a back-buffer reference between frames, allowing native resize handling. The initial temporary swap-chain bootstrap conflicted with Steam overlay initialization; the retained implementation observes the actual game creation instead. Startup crash iterations and the final gameplay evidence are recorded separately in STAGE_REVIEWS.md.

Hook failure leaves the DLL resident with planning disabled, rather than unloading code that may already contain an active detour. Unsupported builds decline in Init before hooks. RML owns the two inline trampolines and import records. The COM Present slot is patched directly and is additional to RML's tracked hook count.

## Stage 3 temporary terrain experiment (0.3.0)

Additional native exports: EditRaiseLower (RVA 0xF8E50), HeightmapTextureOpen (0xF5500), HeightmapTextureClose (0xF5520). Export RVAs and native constants -1 / 60000 are checked before enabling the experiment. No raw terrain buffer writes are used.

Disassembly evidence is retained locally in tools/downloads/terrain-raise.asm, terrain-level.asm, terrain-toheight.asm and terrain-open-close.asm. EditLevel averages nearby heights; EditToHeight samples the second point's current height rather than accepting an arbitrary numeric target. The selected experiment therefore uses EditRaiseLower, with normalized amount (target-current)/heightScale and -1 as its native no-limit sentinel.

The engine converts world X/Z to heightmap texels using origin at terrain+0x890/898, world size at +0x87C/880, and resolution at +0x220/224. Height scale is +0x89C. Radii are in texels. Inner radius 0.20, outer radius 0.45 at an aligned vertex should leave neighboring vertices unchanged; this must be verified in-game and through full saved-file comparison. Native collision writes scale normalized heights by 60000, so the diagnostic displays heightScale/60000 as the expected collision unit. No production completion tolerance is chosen.

The test snaps the edited vertex rectangle inward from the selected area, limits selection to 64 m per side / 256 vertices, and samples a two-vertex guard around it. Those are diagnostic constraints, not approved release limits. The interpolated surface may change in cells adjoining edited vertices; the red border shows that full one-cell envelope rather than claiming an infinitely sharp native discontinuity.

Before capture: exact native save path must be save/4505 - Earthworks_Test_S03, the player must check the empty dry patch and margin, every sample must be above 3 m, variation inside must be <=1 m, and building origins must be at least 300 m away. Paths/trees/underground infrastructure rely on fixture inspection, not a general protection claim. Other saves remain preview-only. Capture performs no edit; explicit raise/lower buttons select targets 1 m above the original maximum / 1 m below the original minimum. Native balances are measured without imposing a production accounting model.

One batch opens the native texture, applies bounded native brushes with internal open/close disabled, and closes it. Those native brushes update height texture, collision arrays and dirty terrain regions themselves; no full-world collision regeneration is introduced. Readbacks stop further editing on unexpected guard changes or native faults. Diagnostic evidence is written to rml/plugins/earthworks-stage03-samples.tsv. Sample copies can be rechecked across a same-process reload, but edits require recapture after loading; no automatic edits or gameplay persistence are implemented.

Prepare-TerrainFixture.ps1 clones S01 into a new S03 folder without overwriting existing saves. It changes only the copied header's same-length display name at byte 4 and embedded save path at byte 260. Original heightmap/collision/header copies and hashes are preserved under tools/fixtures/stage-03-baseline. The accepted 0.2.0 DLL/PDB are retained under tools/releases/0.2.0-prototype.

## Stage 4a read-only inspector (0.4.0)

Provenance: local RepublicMP commit `630775724abd5e3346eb59f9c66e2873128d74a4`, primarily game/world.h, determinism/state_hash.cpp, lockstep/call_home.cpp, lockstep/world_refs.cpp, lockstep/purchase.cpp and lockstep/immigrant_invite.cpp. These community layout references are not user verification of their gameplay semantics.

| Field | Layout | Current interpretation |
|---|---|---|
| Buildings | World = EXE+0x9D4F10; pointer vector +0x11B08 | World-local list, capped at 262144 slots for this diagnostic |
| Building identity | +0x00 unsigned ID, validated alongside vector slot and address | Never persist pointers; invalidate on mismatch/load |
| Building origin/type | +0x350 vector3; type pointer +0x318, category +0x360, subtype +0x364 | Origin, category/subtype and bounded internal asset name at the descriptor start |
| Building asset identity | Type descriptor starts with bounded internal asset name | Exact string `9100000012/EarthworksDevelopmentFacility` identifies every instance of the generated Workshop building, independent of its generic construction-office category/subtype |
| Occupants | Building pointer vector +0xBC0 | Non-null entries and slots; workers at workplace is a hypothesis to compare, residents at homes use this same list |
| Parked vehicles | Building pointer vector +0xC70 | Native membership, not verified idle/usable status |
| World vehicles | World pointer vector +0x12810 | Capped at 65536 slots; faults mark an incomplete scan |
| Vehicle identity/type | ID +0x04; type pointer +0x1708, bounded internal name +0x200, raw category +0x294 | Stage 4d prototype matches internal names plus native road/service category. This is not a native skill-field read and may miss unusually named mod vehicles |
| Vehicle links | Home +0x4F0, owner/assignment +0x4F8 | Pointer equality with selected live building, independent overlapping flags |
| Vehicle position | +0xDE0 vector3 | Horizontal distance from building origin, not operating range or proof of availability |

No new hooks or native calls are added for this inspector. Main-thread sampling occurs at most twice a wall-clock second while selected, including when the panel is closed; an explicit snapshot request can sample immediately. Every followed field uses an SEH-guarded read. Vector bounds and alignment are checked, parked membership is copied/sorted, native pointers never reach the renderer, and published UI data contains only copied values. Reload clears selection/candidates. Each later sample resolves the selected slot/address/ID before following the building. Lost or unreadable identities require reselection; replacement buildings are not automatically followed.

## Stage 7 dedicated facility identity and dry-run

The pinned community [Helicopter Distribution Office implementation](https://github.com/Ultimate-Universe/WRSR-HelicopterDistributionOffice/blob/main/source/helicopter_distribution_office.cpp) documents the same game's building type descriptor pointer at building+0x318 and a global descriptor vector whose 0xBE8-byte entries begin with the internal asset name. The user's in-game inspector snapshot shows the generated Workshop building's full descriptor asset name is `9100000012/EarthworksDevelopmentFacility`, including its Workshop item ID; an earlier check against only the object folder name omitted that prefix and failed. Runtime recognition now compares the exact observed, bounded descriptor string. It does not use category/subtype alone, so ordinary `$TYPE_CONSTRUCTION_OFFICE` buildings remain untouched.

Project assignment stores the stable native building ID in companion schema v2; schema v1 remains readable and migrates unassigned. On reload, the ID is resolved only against a building with the exact Earthworks descriptor name, and duplicate/unreadable matches are refused. The dry-run reports occupant counts, parked allowlisted bulldozer/excavator candidates and distance to the plan centre. It does not prove equipment availability, fuel, skill rating, reservation, operation range or execution time. Relative output uses D32's provisional one-machine reference formula.

## Stage 4d vehicle allowlist prototype

On supported executable 1.1.1.9/build 23935965, the slot selector at EXE+0x3E3F40 is used by the pinned purchase replay as the native test for a vehicle type's depot slot. The plugin detours it and checks the exact session-registered building identity. For that one building, vehicle types are allowed by case-insensitive internal-name matching plus the raw category at type+0x294: named bulldozers, excavators, dumpers and buses must be road (category 1) or road service (category 2). The initial 0.5.1 classifier incorrectly assigned bulldozers/excavators to category 2 and only matched gravel names beginning `gravel_`; user logs exposed these false negatives. Build 0.5.2 accepts both road categories for named earthmovers and matches `gravel` anywhere for dump trucks such as internal type `DLC3_zis150_gravel` (ZiS-585). Other/unreadable/unmatched vehicle types are rejected; other buildings are forwarded unchanged.

This is a conservative name-based classifier, not proof that every modded vehicle name follows these patterns. The community vehicle guide documents construction skill tokens and dumper-deck animation as separate script properties, but the current native binding does not inspect those parsed properties. User in-game validation is required to verify native UI, purchase and vehicle-transfer behavior. [Vehicle modding guide](https://steamcommunity.com/sharedfiles/filedetails/?id=1861143159).

Pick candidates are the five nearest readable building origins within 150 m of the click. This radius is a developer selection aid, not an approved operating range. Up to 48 linked vehicle rows are copied, with total/truncation and incomplete-read information displayed. Vehicles not linked by H/A/P are omitted; no proximity-based ownership is inferred.

Unverified: effective productivity, max staffing, vehicle skills, fuel/readiness, exclusive reservation and interaction with native/manual terraforming. The +0xDE4 building field is produced output, not productivity; +0xDEC is worker/visitor settings, not actual headcount; +0xF18/F30/F48/F60 are destination lists, not current workers. These are deliberately not substituted for missing readings. Stage 4 requires further evidence before office design or autonomous execution.

## Stage 4b assignment filter

The filter checks the 18-byte prologue at EXE+0x161C90 before installation. Pinned RepublicMP `game_calls.cpp` maps construction-office windows, office pick tools and area-office tools to this function with the `WOOFbb` argument shape. The wrapper checks the office argument and forwards every argument unchanged for other offices. The separate native routine at EXE+0x1561C0 is used by the office window's automatic “add construction” controls and is not directly hooked in this build. Whether that route funnels through EXE+0x161C90 must be established in the user's Stage 4b check; ordinary-job exclusion remains unverified until then.
