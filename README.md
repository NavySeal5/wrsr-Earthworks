# Earthworks Planning

Earthworks Planning adds editable terrain Sites, named Projects, and dedicated Small, Medium, and Large facilities to *Workers & Resources: Soviet Republic*.

For player setup, start with [the installation guide](docs/PLAYER_INSTALLATION.md). Read the [compatibility notes](COMPATIBILITY.md), [known issues](KNOWN_ISSUES.md), and [change log](CHANGELOG.md) before reviewing the candidate. See the [documentation index](docs/INDEX.md) for developer notes and stage records.

## Loader support

TesmioLoader is the primary loader. Republic Mod Loader (RML) remains an optional fallback. Do not load the plugin through both loaders in the same game session.

TesmioLoader does not install the plugin DLL from Steam Workshop. The combined package includes `Install-TesmioPlugin.ps1`, which finds the Steam game folder and installs/enables the plugin. See [Tesmio playtest setup](docs/TESMIO_PLAYTEST.md).

The RML fallback uses `plugins/EarthworksPlanning.rml.json` beside the DLL. RML users must enable the plugin and Workshop content in the RML playlist.

The plugin loads through TesmioLoader API 4 without checking the game executable timestamp or whole-file hash. Each native hook still checks its target signature before patching.

## Version

`VERSION` is the single source for the plugin, CMake project, RML manifest, and package name. Increase it once for each candidate build.

## Development

Read [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md), [decision register](docs/DECISIONS.md), and [stage review log](docs/STAGE_REVIEWS.md) before changing gameplay or UX behavior. The project uses staged implementation and user-run in-game reviews. Do not perform in-game tests unless the user explicitly asks.

Required build tools are Git for Windows and Visual Studio 2022 with the components in `.vsconfig`. Run `scripts/Fetch-Dependencies.ps1` to restore the pinned Tesmio API 4 SDK, RML fallback source, and RepublicMP reference source. This script does not install a game loader.

Build the player plugin with:

```powershell
./scripts/Build.ps1
```

Build the developer UI separately with `scripts/Build-DeveloperUI.ps1`. Build outputs and fetched dependencies are ignored by Git.

## Release candidate package

Run `scripts/Build-ReleasePackage.ps1`. It builds the player plugin, prepares the three facility package inputs without changing the game's Workshop WIP folders, and creates a versioned folder and ZIP under `build/release`. The package includes player install instructions, corresponding source, license texts, and third-party notices. It fails if output for that version already exists. It does not upload to Workshop.

The release builder reads native donor configuration from the installed game's `media_soviet` data. It writes generated facility inputs under `build` and does not install the plugin or change TesmioLoader settings.

## Local Tesmio development launch

Build the developer UI with `scripts/Build-DeveloperUI.ps1`. With Steam running, close the game and the project-local TesmioLoader window. Run `launch-prototype.cmd`. It stages the developer plugin and facility assets, enables Earthworks in the isolated Tesmio configuration, then opens the project-local launcher. It does not start a game session or load a save. Errors pause the command window.

Use `launch-rml-fallback.cmd` to open the retained RML diagnostic path. `launch-diagnostics.cmd` remains a legacy alias for that fallback.

A successful loader start does not prove that native hooks work. Use a disposable save and report the observed game and loader logs.
