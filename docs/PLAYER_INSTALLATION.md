# Earthworks Planning candidate

This is a release-candidate package. Use a disposable save for review. It is not a public release.

The package is built for Workers & Resources: Soviet Republic 1.1.1.13 (Steam build 24922914). In-game compatibility checks are still pending.

## TesmioLoader installation

TesmioLoader is the primary host. Steam Workshop does not install the plugin DLL into TesmioLoader.

1. Close the game and TesmioLoader.
2. Install TesmioLoader in the game folder.
3. Run `Install-TesmioPlugin.ps1` from this package. It detects the Steam library or asks for the folder.
4. Start TesmioLoader, then start the game through it.

The installer copies the DLL and enables Earthworks in TesmioLoader. Do not load Earthworks through RML in the same session.

## Optional RML installation

Use RML only if you do not use TesmioLoader for this session. Add this package as an RML mod, then enable both its content and the plugin in the RML playlist. Do not enable both loaders at once.

## Candidate limits

- Fuel storage and refuelling are not included. The facility tank is decorative.
- Terrain work does not consume fuel, money, or quarried stone.
- The plugin checks each native hook signature before it installs that hook. A failed signature disables the affected hook.
- Test the candidate in a disposable save. Report loader logs and game behavior with the package version shown in `package-manifest.json`.

## Source and notices

The `SOURCE` folder contains the corresponding project source, build files, dependency pins, and authored building assets. `THIRD_PARTY_NOTICES.md` lists third-party sources and licenses. License texts are in `LICENSES`.
