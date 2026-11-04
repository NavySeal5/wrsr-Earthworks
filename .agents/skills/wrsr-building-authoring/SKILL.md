---
name: wrsr-building-authoring
description: Design, model, configure, and package custom building assets for Workers & Resources: Soviet Republic, from an approved concept through user-run in-game review.
---

# Workers & Resources building authoring

Use this skill for any WRSR mod that creates or changes a building. Follow repository instructions first; do not assume every mod uses the same model generator, staging script, item layout, or test policy.

## Work from concept to testable asset

### 1. Establish what the building is for

Find out the rough building type and gameplay purpose. Use the user's description when it already answers this. Identify the intended player, setting, and any related game systems or vanilla assets that should guide the design. Inspect the target mod's instructions and current source before proposing implementation details.

### 2. Propose and refine the visual concept

Prepare one or more visual proposals that communicate the overall form, layout, access, and distinctive features. Use mockups, image generation, sketches, or another available visual tool that fits the task. Present the proposals and revise them with the user's feedback until the user selects a concept. Do not treat a generated image as an implementation specification until the user approves it.

### 3. Resolve building properties before modeling

Translate the approved concept into a concise spec. Cover the properties that matter for the building, such as:

- scale, footprint, height, orientation, and ground contact;
- visual style, materials, texture sources, and colors;
- building type, capacity, workforce, service or production behavior;
- construction phases, work requirements, and material costs;
- vehicle parking count/types and their approach, parking, and exit paths;
- pedestrian access, entrances, and road connections;
- placement preview, icon, and player-facing building UI needs.

Ask the user about consequential gameplay or UX choices that remain open. Do not ask them to re-confirm settled requirements. Clearly label assumptions and temporary prototype values.

### 4. Generate the assets from editable sources

Once the concept and dependent properties are clear, create the model and related game files using the tools available in the project. This may include Blender or another modeler, scripts, image/texture tools, ModelViewer, and the game's native asset/config format. Prefer reproducible editable sources and generators over hand-editing generated or live staging files. Inspect installed vanilla assets and authoritative modding references for formats, material paths, construction nodes, connection points, and parking-route conventions; do not guess based only on a ModelViewer preview.

Keep mesh, material, and construction-node names synchronized across model exports, native material files, construction configuration, render configuration, and staging scripts. Only copy donor configuration fields that are understood and relevant; do not copy a donor's bounds, fire, or route data onto unrelated geometry without verifying how they are generated and used.

### 5. Use Computer Use only with permission

Before controlling ModelViewer or another desktop application, ask the user for Computer Use permission unless permission for that specific activity is already active in the conversation. If permission is not granted, continue the file-based work that does not depend on desktop control and give the user clear manual instructions for the blocked steps, such as how to start ModelViewer, import the generated OBJ, choose the Building object type, save the NMF, and return the resulting file. Do not imply that a conversion or desktop action was completed when it was not.

Use paths relative to the mod root (`MOD_ROOT`) for repository assets and scripts, and paths relative to the game install (`GAME_ROOT`) for native assets and Workshop staging. Do not put machine-specific absolute paths in this skill or its reusable instructions.

**ModelViewer path and filename note:** On the installation used to develop the Earthworks case study, the NMF save dialog only accepted locations under `GAME_ROOT/media_soviet`; a repository destination outside that tree produced “file is outside of GAME_DIR/media_soviet”. Save to a temporary or WIP location under `GAME_ROOT/media_soviet`, then copy the file into the repository using filesystem tools. The dialog appended `.nmf`: entering `model.nmf` created `model.nmf.nmf`, so enter the filename stem (`model`) and verify the actual output filename. If another installation behaves differently, follow its observed dialog behavior while keeping paths relative to the appropriate root.

### 6. Package and review in stages

Follow the repository's build and staging scripts, preserve ownership and baseline guards, and stage a clearly identifiable candidate. Provide a concise checklist for the user to inspect in game. Check the completed building, not only placement ghosts or construction overlays, for materials, scale, ground contact, access, parking behavior, and construction progression. Treat static inspection, script completion, builds, and ModelViewer appearance as technical evidence, not in-game acceptance. Follow the repository's instructions about who performs in-game checks, how to record results, and when to commit or publish.

## Earthworks example

The [Earthworks Stage 12 case study](references/earthworks-stage12-case-study.md) records one mod's approved design, source layout, construction configuration, ModelViewer lessons, texture-binding diagnosis, and corrections. Treat its values and conventions as evidence from that project, not defaults for other buildings or mods.

## Modding references

- [Official General modding guide](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/General_modding)
- [Official Modelling guide](https://wiki.hoodedhorse.com/Workers_Resources_Soviet_Republic/Modelling)
