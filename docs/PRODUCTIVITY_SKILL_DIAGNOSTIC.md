# Vehicle Speed/Level productivity source

Earthworks uses the same number shown in the vehicle information panel as its machine level. It does not substitute another construction skill when that value differs.

| Vehicle | `script.ini` value used | Rate units |
| --- | --- | --- |
| Bulldozer | `$SKILL_CONSTRUCTION_BULLDOZER` | displayed Speed/Level directly |
| Excavator | `$SKILL_GRAVELMINING` | displayed Speed/Level directly |

For example, the M-III P 1.5 displays level 21, so it contributes 21 rate units. The Stalinets S-65 displays level 25, so it contributes 25. Its rate is not calculated from the separate Groundworks value. That value can still appear in the developer log for comparison, but it does not drive productivity.

Each displayed Speed/Level point contributes 350 m³ per in-game day at the default 1× setting. This uses 60 simulation hours per in-game day, or 350/60 m³ per point per simulation hour. The global Site speed multiplier scales the rate. A level-21 machine therefore contributes 7,350 m³/day; a level-25 machine contributes 8,750 m³/day. A missing class-specific displayed-level value produces unavailable capacity. Earthworks does not silently fall back to Groundworks or a fixed rate.

## Developer diagnostic

The Developer tab's **Log inspector snapshot** reports the parsed bulldozer level, excavator level, Groundworks value, selected displayed level, selected field, derived rate units, and source `script.ini` for each linked bulldozer or excavator. The raw aligned-integer scan remains read-only and informational; it does not drive productivity.

The lookup supports vanilla and DLC vehicle folders. Workshop or modded vehicles whose scripts are outside those folders can report unavailable until their definitions are resolved. Do not substitute a different field for these vehicles without confirming that field matches the displayed Speed/Level value.

## Open the developer inspector

The normal player build omits technical diagnostics. Build the developer-only UI into the isolated Earthworks Development profile with:

```powershell
.\scripts\Build.ps1 -DeveloperUI
.\scripts\Stage-Diagnostics.ps1 -DeveloperUI
```

Close the game and TesmioLoader before staging. Run `launch-prototype.cmd`, choose **Earthworks Development**, and launch the game from TesmioLoader. Open the Earthworks panel and select **Developer**, beside **Projects and Sites**. The subscribed/player build does not include this tab.

## Reference snapshot

The user's snapshot showed:

- M-III P 1.5 excavator: displayed Speed/Level 21, `$SKILL_GRAVELMINING 21`, and separate `$SKILL_CONSTRUCTION_GROUNDWORKS 15`. Earthworks uses 21, giving 7,350 m³/day at 1×.
- Stalinets S-65 bulldozer: displayed Speed/Level 25 and `$SKILL_CONSTRUCTION_BULLDOZER 25`. Earthworks uses 25, giving 8,750 m³/day at 1×.

The raw scan found no varying runtime integer field. It is not needed for these supported definitions because the configured fields above match the displayed values.
