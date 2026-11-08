# Vehicle construction-skill diagnostic

The vanilla vehicle definitions store machine skill values in each vehicle's `script.ini`:

- Bulldozers use `$SKILL_CONSTRUCTION_BULLDOZER`.
- Excavators use `$SKILL_CONSTRUCTION_GROUNDWORKS`.

Examples in the installed game data include KT50 bulldozer = 22, small bulldozer = 9, K12 bulldozer = 36, large bulldozer = 58, Stalinec bulldozer = 28; EO302B excavator = 22, T815 excavator = 15, RMN12215 = 17, WT174 = 26, LH360 = 28, E10011 = 33. This shows that a single static rate per machine discards real differences in the vanilla data. These configured skills are evidence about machine ratings; they do not establish a validated terrain-volume multiplier.

## Diagnostic behavior

The Developer tab's **Log inspector snapshot** action now reports the matching vanilla `media_soviet/vehicles/<internal-type-name>/script.ini` and both construction skill tokens for each linked bulldozer or excavator. It reads only when the user requests a snapshot. It does not write game data, alter the executor, or affect productivity. Internal type names are restricted to plain folder-name characters before constructing the path.

Workshop/modded vehicles whose definitions are not in the vanilla vehicle directory will report unavailable. Their active mod source and parsed native type value have not yet been resolved. Do not silently use a different skill token or substitute the fallback for a missing value until this is decided and validated.

## Open the developer inspector

The normal player build omits technical diagnostics. Build the developer-only UI into the isolated Earthworks Development profile with:

```powershell
.\scripts\Build.ps1 -DeveloperUI
.\scripts\Stage-Diagnostics.ps1 -DeveloperUI
```

Close the game and project-local RML before staging or launching. Run `launch-prototype.cmd`; it stages the developer build and opens the project-local RML. Choose **Earthworks Development** and launch the game from RML. Open the Earthworks panel and select **Developer**, beside **Projects and Sites**. Confirm the RML plugin version is `0.16.39-construction-skill-diagnostic` with DLL hash prefix `5f912b7f`. The subscribed/player build does not include this tab.

## User verification

In-game, open Earthworks → Developer → Workforce inspector, select a facility containing machines with different known skills, then click **Log inspector snapshot**. Send the `earthworks skill-source` lines from the current game's `log.html`. Compare the reported configured skill with each vehicle's native information panel. In particular, check several different excavator and bulldozer models. Do not change productivity until the mapping and behavior are confirmed.
