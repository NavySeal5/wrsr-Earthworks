# Compatibility

## Recorded target

The current candidate is built for:

- Workers & Resources: Soviet Republic 1.1.1.13, Steam build 24922914.
- TesmioLoader b0.3.7, using Tesmio API 4.
- Republic Mod Loader 1.0.1 as an optional fallback.

This is the recorded build target. The latest candidate has not passed the user-run in-game compatibility checks. Do not read this target as a guarantee for other game or loader versions.

## Loader setup

TesmioLoader is the primary host. Workshop subscription does not install the plugin DLL. Run `Install-TesmioPlugin.ps1` or install the DLL in TesmioLoader's `plugins` folder.

RML remains an optional path. Use one loader per game session. Do not enable Earthworks in both loaders at once.

## Native hooks

The plugin does not reject a game by executable timestamp or whole-file hash. Each native hook checks its target signature before patching. If a signature does not match, the affected hook must stay disabled and report the failure.

## Evidence

The recorded game baseline and binary hashes are in `docs/game-baseline.json`. The user-run installation and gameplay checks are in [release readiness](docs/RELEASE_READINESS.md).
