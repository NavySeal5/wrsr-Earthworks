# Earthworks Planning playtest package

The combined package contains Earthworks Planning and the Small, Medium, and Large facilities. The packaging script writes a portable package to `build/release/EarthworksPlanning` and stages a local development item at `media_soviet/workshop_wip/9100000015`.

The package keeps the runtime building files, the Tesmio plugin DLL, the RML fallback manifest, the Tesmio plugin installer, and its setup guide. It does not include source models, debug symbols, or diagnostic probes.

## Primary setup: TesmioLoader

TesmioLoader must be installed in the game directory. Workshop subscription alone does not install the DLL into TesmioLoader. Follow [Tesmio playtest setup](TESMIO_PLAYTEST.md) to install the DLL in the loader's `plugins` folder and enable it.

The local development launcher stages the plugin into the project-local Tesmio runtime and starts the Tesmio launcher. Run `launch-prototype.cmd` only after building the player plugin. The launcher does not start a game session. Use a disposable save for the in-game check.

## Optional fallback: RML

The package includes `plugins/EarthworksPlanning.rml.json` for RML. Add the package to RML and enable both its content and plugin in the RML playlist. Do not enable both RML and Tesmio for the same game session.

## Rebuild the package

1. Close the game and the project-local loader.
2. Build the player plugin with `scripts/Build.ps1`.
3. Run `scripts/Build-CombinedModPackage.ps1`.

The packaging script preserves existing files and does not upload to Steam Workshop. It checks the package source and creates or refreshes the portable package and local WIP item.

## Compatibility

The current native hook baseline is game 1.1.1.9, PE timestamp `0x6A3EB6AD`. If the game has a different timestamp, Earthworks will decline initialization and install no hooks. Do not remove the guard. Re-verify hook addresses before supporting a newer executable.

No in-game test was performed as part of preparing these instructions. Player-run checks are required before longer playtesting.
