---
name: wrsr-building-authoring
description: Create, stage, and troubleshoot custom building assets for Workers & Resources: Soviet Republic using Blender, ModelViewer, native building scripts, and Workshop WIP files.
---

# Workers & Resources building authoring

Use this skill when creating or changing a custom building model, material, construction script, Workshop item, or local test package for Workers & Resources: Soviet Republic (WRSR).

## Start with the project state

For this repository, read `AGENTS.md`, `IMPLEMENTATION_PLAN.md`, `docs/DECISIONS.md`, and the Stage 12 section of `docs/STAGE_REVIEWS.md` before changing the Earthworks building. Read the detailed [Earthworks Stage 12 case study](references/earthworks-stage12-case-study.md) for the real asset layout, current values, proven material route, failure history, and remaining checks.

Separate three kinds of information in notes and proposals:

- **User-approved design**: fixed requirements such as the compact dispatch office, open two-bay canopy, decorative visible tank, and the currently approved single-front road arrangement.
- **Prototype settings**: temporary values that are useful for testing but are not balance decisions, such as the current five worker places and construction coefficients.
- **Unverified behavior**: matters such as vehicle pathing, save/reload, workforce scaling, and final palette acceptance that still need the user's in-game review.

Ask before changing a consequential visual or gameplay choice that is not already settled. Preserve superseded decisions in the project record when they explain why the current design differs.

## Author the model and game assets

1. **Keep source assets authoritative.** Edit `assets/building/generate_earthworks_facility.py` for geometry and Blender colors. Keep generated Blender/OBJ work under `assets/building/generated/`; the review render is written to `docs/mockups/`. Do not hand-edit ignored files in `build/` or the live `workshop_wip` item; change the source or staging script and regenerate them.
2. **Use stable material and node names.** The generator groups exported geometry by its first material, names each group `mesh_<material-name>`, triangulates it, merges near-duplicate vertices and recalculates normals before OBJ export. Keep names synchronized among Blender materials, OBJ/NMF mesh nodes, native MTL submaterials, `building.ini` construction-node declarations, and runtime materials.
3. **Convert through ModelViewer.** Import the generated OBJ with the game's ModelViewer and inspect it with object type set to Building. ModelViewer's NMF save dialog is restricted to `GAME_DIR/media_soviet`; saving directly to the mod repository (for example, `assets/building/native/`) fails with “file is outside of GAME_DIR/media_soviet”. Navigate to the object folder under `media_soviet/workshop_wip/...`, enter the filename stem without `.nmf` because the dialog appends it (`model.nmf` becomes `model.nmf.nmf`; enter `model`), save, and verify the reported and actual filename. Then copy the saved NMF into `assets/building/native/` with filesystem tools. Verify the copied file before staging. ModelViewer previews are useful, but they do not prove that the game uses the same runtime renderconfig/material binding.
4. **Provide the complete WIP item.** Keep the Workshop item root and `$OBJECT_BUILDING` object directory consistent. The Earthworks stage script writes `workshopconfig.ini`, `building.ini`, `renderconfig.ini`, the preview images, the NMF, the runtime MTL, and its texture files. Follow that script rather than copying a donor building's unrelated geometry files.
5. **Match construction phases to the NMF.** Every construction node in `building.ini` must be a real NMF mesh name. Assign the intended nodes to native construction phases, and attach the vehicle work station to a real node. Keep phase ordering consistent with a working vanilla script; in this project, construction phases precede `$RESOURCE_SOURCE_*` declarations.
6. **Bind textures explicitly.** In this project, MTL diffuse-color constants and path-only changes did not produce the intended colors on the completed in-game building. The confirmed route uses a local MTL referenced from `renderconfig.ini`, `$TEXTURE_MTL 0` for custom-color submaterials, and BC1/DXT1 Workshop-local DDS files. For large surfaces where the player requested vanilla texture matching, the current pass instead references the native firestation/garage atlases with `$TEXTURE 0` and UVs targeting the relevant atlas regions. Preserve these distinct routes unless in-game evidence justifies changing them; atlas UV mapping is pending a fresh user-run review.

## Stage, inspect, and review

- Close the game and project-local RML before staging. `launch-prototype.cmd` checks that state, invokes `scripts/Stage-Diagnostics.ps1` (which builds/stages the plugin and invokes the building stager), then opens the project-local RML. Do not launch a second RML instance over a live session.
- `scripts/Stage-EarthworksBuilding.ps1` checks the pinned game baseline and a project ownership marker before copying into `media_soviet/workshop_wip/9100000012`. Preserve these guards; never overwrite an unmarked Workshop item.
- The script deliberately does not copy a donor `building.bbox` or `building.fire`. The game generates those for the current NMF. If the model hash changes, the staging script removes stale generated bounds; allow a full game restart for regenerated geometry to take effect before evaluating construction node costs.
- Use a fresh/disposable save and a newly placed building when checking changed assets. Inspect the completed model for materials: placement ghosts and construction-phase tints are not evidence that the finished render has the intended colors.
- In-game acceptance belongs to the user in this repository. Do not claim success from a build, ModelViewer preview, static inspection, or plugin log. Provide concise user-run instructions and record only outcomes the user reports. Do not edit a user save or perform gameplay tests without explicit authorization.
- After each implementation stage or approved decision, make a local commit. Do not push to origin unless the user explicitly asks.

## Current project checkpoint

Stage 12.1 has a custom Blender source, ModelViewer NMF, two-slot local building item, native construction phases, local compressed runtime textures, and a muted palette staged in commit `0d3f0db`. The user confirmed that the previous high-contrast texture-binding diagnostic displayed in-game colors. The muted palette has not yet been accepted in game; scale, road snapping, both bay routes, save/reload, and demolition remain part of Stage 12.1 review. Stage 12.2 (native facility UI) and 12.3 (facility/Project integration) are not yet complete. Do not report Stage 12 as accepted.
