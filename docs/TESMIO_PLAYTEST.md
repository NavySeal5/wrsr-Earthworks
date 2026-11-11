# Tesmio playtest setup

## Required files

- Install TesmioLoader in the game directory. The game directory contains `SOVIET64.exe` and `media_soviet`.
- Build the player plugin with `scripts/Build.ps1`.
- Assemble the local mod package with `scripts/Build-CombinedModPackage.ps1`.

The package contains the three facility definitions, `plugins/EarthworksPlanning.dll`, the RML fallback manifest, this guide, and `Install-TesmioPlugin.ps1`. Steam Workshop does not install the DLL into TesmioLoader's plugin folder.

## Install the plugin for TesmioLoader

1. Close the game and TesmioLoader.
2. Open PowerShell in the package folder.
3. Run the installer and provide the game directory:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install-TesmioPlugin.ps1 -GameRoot "<game directory>"
   ```

   Example: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install-TesmioPlugin.ps1 -GameRoot "D:\Games\SovietRepublic"`

4. The installer copies `EarthworksPlanning.dll` to `<game directory>\plugins`. It refuses to replace a different DLL unless `-Force` is used. With `-Force`, it saves a timestamped backup first.
5. Start TesmioLoader, then launch the game through it.

TesmioLoader loads enabled plugins from its `plugins` directory. Its `tesmioloader.ini` must enable `EarthworksPlanning` in the `[plugins]` section. The local development launcher stages and enables it automatically in the project-local Tesmio runtime.

## RML fallback

Use RML only if the Tesmio path is unavailable. Keep the package's `plugins/EarthworksPlanning.rml.json` beside the DLL. Add the package as an RML mod and enable both the mod content and the Earthworks plugin in the RML playlist. Do not install or enable the plugin in both loaders for the same game session.

## Game version guard

The plugin checks the executable timestamp before it installs native hooks. The verified baseline is game version 1.1.1.9, PE timestamp `0x6A3EB6AD`. If the check reports a different timestamp, Earthworks will not initialize and it will install no hooks. Do not remove the guard. Re-verify the native addresses against the new executable before supporting a later game version.

## First in-game check

Use a disposable save. Confirm that Earthworks loads, the Earthworks button opens, all three facility sizes appear in the building menu, and the facility UI can register each building. Report any loader or game log errors. Do not treat a successful loader start as proof that the native hooks work.
