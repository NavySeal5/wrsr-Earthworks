# Tesmio playtest setup

## Required files

- Install TesmioLoader in the game directory. The game directory contains `SOVIET64.exe` and `media_soviet`.
- Build the player plugin with `scripts/Build.ps1`.
- Assemble the local mod package with `scripts/Build-CombinedModPackage.ps1`.

The package contains the three facility definitions, `plugins/EarthworksPlanning.dll`, the RML fallback manifest, this guide, and `Install-TesmioPlugin.ps1`. Steam Workshop does not install the DLL into TesmioLoader's plugin folder.

## Install the plugin for TesmioLoader

1. Close the game and TesmioLoader.
2. Open PowerShell in the package folder.
3. Run the installer. It searches Steam libraries automatically. If needed, it asks for the game folder. You can also pass `-GameRoot`:

   ```powershell
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install-TesmioPlugin.ps1
   ```

   Example override: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Install-TesmioPlugin.ps1 -GameRoot "D:\Games\SovietRepublic"`

4. The installer copies `EarthworksPlanning.dll` to `<game directory>\plugins` and enables `EarthworksPlanning=1` in `tesmioloader.ini`.
5. Start TesmioLoader, then launch the game through it.

The local development launcher stages and enables the plugin automatically in its project-local Tesmio runtime.

## RML fallback

Use RML only if the Tesmio path is unavailable. Keep the package's `plugins/EarthworksPlanning.rml.json` beside the DLL. Add the package as an RML mod and enable both the mod content and the Earthworks plugin in the RML playlist. Do not install or enable the plugin in both loaders for the same game session.

## First in-game check

Use a disposable save. Confirm that Earthworks loads, the Earthworks button opens, all three facility sizes appear in the building menu, and the facility UI can register each building. Report any loader or game log errors. Do not treat a successful loader start as proof that the native hooks work.
