# Code structure

- `src/core` contains terrain geometry, Site history, persistence encoding, productivity, and global settings. It has no game-window or loader hooks.
- `src/runtime` connects core plans to game state, native terrain hooks, facilities, scheduling, and save events. `runtime.cpp` owns the main game-thread workflow.
- `src/ui` draws the player panel and terrain underlay. Developer-only inspector code is enabled by a separate CMake option.
- `src/plugin` exports the Tesmio API entry points. CMake generates the version header from the root `VERSION` file.
- `assets/building` contains facility models, materials, textures, and generation sources. `scripts/Stage-EarthworksBuilding*.ps1` can prepare package inputs without writing to the game's WIP folder by using `-PackageOnly`.
- `scripts/Build-CombinedModPackage.ps1` builds the local playtest package by default. `scripts/Build-ReleasePackage.ps1` builds a separate versioned candidate and archive.
- `tests` contains core and synthetic data checks. In-game checks remain user-run and are listed in the stage and release review documents.

The two largest integration files are `src/runtime/runtime.cpp` and `src/ui/overlay.cpp`. Keep new geometry and policy logic in `src/core`; keep native calls in `src/runtime`; keep drawing and input handling in `src/ui`. Split a large file only when a cohesive responsibility has a clear interface and a focused review checklist.
