# Earthworks Planning playtest package

The playtest package contains Earthworks Planning and the Small, Medium, and Large facilities. The packaging script writes a clean local package to `build/playtest/EarthworksPlanning` and stages the development item at `media_soviet/workshop_wip/9100000015`.

The playtest package contains runtime building files, the Tesmio plugin DLL, the RML fallback manifest, the Tesmio plugin installer, and setup instructions. It does not include source models, debug symbols, or diagnostic probes.

## Primary setup: TesmioLoader

TesmioLoader must be installed in the game directory. Workshop subscription alone does not install the DLL into TesmioLoader. Follow [Tesmio playtest setup](TESMIO_PLAYTEST.md) to install the DLL in the loader's `plugins` folder and enable it.

The local development launcher stages the plugin into the project-local Tesmio runtime and starts the Tesmio launcher. Run `launch-prototype.cmd` only after building the player plugin. The launcher does not start a game session. Use a disposable save for the in-game check.

## Optional fallback: RML

The package includes `plugins/EarthworksPlanning.rml.json` for RML. Add the package to RML and enable both its content and plugin in the RML playlist. Do not enable both RML and Tesmio for the same game session.

## Rebuild the package

1. Close the game and the project-local loader.
2. Build the player plugin with `scripts/Build.ps1`.
3. Run `scripts/Build-CombinedModPackage.ps1`.

The playtest script refreshes only its generated package folder and copies its files into the local WIP item. It does not upload to Steam Workshop. It does not remove older files from the WIP item.

To prepare the existing release Workshop item, use `scripts/Build-ReleasePackage.ps1`. It writes release files directly into the Workshop WIP folder whose item ID is stored in `assets/workshop/workshopconfig.ini`. It preserves the current Workshop title, owner, description, tags, and visibility. It does not create a ZIP or upload the item. See the root README for details.

The plugin does not gate loading on the game executable timestamp or whole-file hash. Native hooks check their target signatures before patching.

No in-game test was performed as part of preparing these instructions. Player-run checks are required before longer playtesting.
