**Earthworks Planning — RML development setup**

Implementation design: [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md).

Development proceeds in small stages, each ending in an in-game check and user review before the next stage. Important gameplay/UX decisions are discussed before dependent implementation. See [decision register](docs/DECISIONS.md) and [stage review log](docs/STAGE_REVIEWS.md). The first planning interaction is confirmed: select an area and enter or sample a target height.

Republic Mod Loader (RML) is the primary host, using its Tesmio API 4 compatibility layer. Current plugin build: **0.11.0-stage10a-player-ui**. The player interface is split into Plan and Projects tabs; the workforce inspector and terrain probes are compiled only when `EARTHWORKS_DEVELOPER_UI=ON`. The Stage 9 scheduler supports multiple manually assigned projects, priority controls, concurrent use of observed parked machine candidates, and overlap prevention. Started projects share a facility's observed workers in priority order; projects without capacity wait, and paused/review-blocked work is skipped. Project order and active state are saved with the world. Productivity remains provisional at 10 m3 per simulation hour per manual-rate unit. Native machine reservation, fuel use, machine animations, and workers from outside the facility remain unimplemented or unverified. Machine names are an initial classifier; unusual modded names may need later support. A recent audit found the current test building inherited 16 vehicle parking points rather than the two intended for Stage 9; correcting the facility model/config is the Stage 10b gate. See [Stage 10a user test instructions](docs/STAGE10A_TESTING.md). In-game checks are user-run unless explicitly authorized otherwise.

**Required tools**

- Git for Windows.
- Visual Studio 2022 with the components in `.vsconfig`: native desktop development, x64/x86 MSVC, CMake/Ninja and Windows SDK 10.0.22621.
- RML 1.0.1 and the compatibility SDK, pinned in `dependencies.lock.json` and restored by `scripts/Fetch-Dependencies.ps1`.

The pinned RML repository includes its release runtime, so Go and Python are not needed to build this C++ plugin. Use Visual Studio's native debugger initially; add Ghidra when concrete engine investigation requires it.

**Build**

From this folder, run `build.cmd`, or run `scripts/Build.ps1` in PowerShell. The script discovers Visual Studio and initializes its x64 development environment for that process; no global PATH changes are needed. CMake produces `build/windows-x64/bin/EarthworksPlanning.dll` and debug symbols with the player UI (no technical inspector or terrain-probe controls). To build the separate developer UI, run `scripts/Build-DeveloperUI.ps1`; it uses a separate output directory and verifies the x64 DLL and Tesmio exports.

To restore dependencies on another checkout, run `scripts/Fetch-Dependencies.ps1` first. It validates pinned RML, Tesmio SDK and RepublicMP source commits, SDK header hashes and RML runtime/asset hashes. Dependencies and generated outputs are ignored by Git. The CMD wrappers permit these local scripts to run using a process-only execution-policy setting; they do not change the machine's PowerShell policy.

**Stage and launch a diagnostic game session**

Run `scripts/Stage-Diagnostics.ps1` after building to copy the diagnostic DLL, any available PDB, and optional RML metadata into `tools/runtime/rml/rml/plugins`. Staging checks the game hashes against `docs/game-baseline.json` and verifies pinned RML files, then refreshes the isolated `Earthworks Development` diagnostic preset. This preset selects the diagnostic plugin and excludes Workshop and other Development payloads. Your subscribed RML configuration is untouched.

Close the project-local RML window before restaging a new build. The launch script reuses an already-open development launcher instead of replacing its live configuration or opening another copy.

With Steam open and both the game and project-local RML closed, run `launch-prototype.cmd` (`launch-diagnostics.cmd` is an equivalent legacy wrapper). It refreshes the preset, stages the local Workshop building, and opens RML visibly. Confirm `Earthworks Development` and **Earthworks Planning**, version **0.11.0-stage10a-player-ui**, then launch the game from RML. The expected classification is Tesmio API 4, hosted by RML's built-in compatibility layer.

After a game launch, `tools/runtime/rml/rml/logs/rml-runtime.log` and `tools/runtime/rml/rml/state/last_runtime_report.json` identify the plugin version, lifecycle results and native hooks. Inspector snapshots requested with **Log inspector snapshot** use this log prefix:

```text
earthworks  stage04
```

The diagnostic preset enables RML's Detailed Diagnostic Logging. Opening the launcher alone does not establish in-game compatibility. The original on-disk game binaries remain unchanged.

The setup task does not launch the game automatically or load a save. Use a disposable save for the later terrain experiments. Ordinary Steam launches continue to use the normal executable.

Use a disposable vanilla-compatible world: this profile disables Workshop content, so a normal republic that needs those assets is unsuitable. Stage 4a compares a staffed workplace and machinery at a construction office. These are diagnostic specimens, not a chosen production office design. Follow the Stage 4a checklist and report observations for review.

`scripts/Verify-DiagnosticSession.ps1` is historical **Stage 1 only**: it expects zero hooks and must not be used to certify this prototype. Builds verify x64 architecture and exports. `workforce_checks.exe` exercises guarded reads, independent links and identity handling with synthetic memory only; it does not establish in-game field meanings.

**Files and dependency provenance**

- `.vsconfig`: component list for Visual Studio Installer.
- `dependencies.lock.json`: pinned RML and compatibility SDK versions/checksums.
- `assets/rml/`: diagnostic preset and plugin metadata templates.
- `docs/game-baseline.json`: installed Steam build, PE timestamps and SHA-256 hashes before native integration.
- `docs/SETUP_STATUS.md`: results of local setup and build verification.
- `THIRD_PARTY_NOTICES.md`: dependency provenance and distribution considerations.

Build/dependency scripts write inside this project. The original Tesmio runtime staging folder from the first setup is unused by all current scripts. Neither the game's original executables/content nor the subscribed RML installation is overwritten.
