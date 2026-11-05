---
name: wrsr-building-authoring
description: Design, model, configure, and package custom building assets for Workers & Resources: Soviet Republic, from an approved concept through user-run in-game review.
---

# Workers & Resources building authoring

Use this skill for any WRSR mod that creates or changes a building. Follow repository instructions first; do not assume every mod uses the same model generator, staging script, item layout, or test policy.

## Work from concept to testable asset

### 1. Establish what the building is for

Find out the rough building type and gameplay purpose. Use the user's description when it already answers this. Identify the intended player, setting, and any related game systems or vanilla assets that should guide the design. Inspect the target mod's instructions and current source before proposing implementation details.

Ask early which areas inside and around the building footprint should use each native ground/floor type. Have the user map areas such as the building footprint, parking/forecourt, pedestrian approach, loading/service apron, and utility corridors to a surface type, or explicitly leave them as natural ground. Do not assume a paved apron or cover the whole footprint by default; refine the map with the user if the building concept changes.

### Native floor and connection areas

Never model a floor or paved ground area as a mesh. Use the game's native building-area directives for these surfaces. Reserve meshes for the building itself, props, and a foundation/plinth only when the design needs one; a foundation is structural geometry, not a substitute for a floor area.

The Building Editor comparison's `element_script.ini` emitted two distinct surface-property families. Normal `$CONNECTION_*_DEAD` entries draw a narrow, texture-following surface run along a line. `$CONNECTIONS_*_DEAD_SQUARE` entries cover a square/rectangle. Ask which shapes the building needs; do not replace one with the other or approximate either with a mesh.

| Surface type | Line property and observed parameters | Square/rectangle property and observed parameters | Use it for |
|---|---|---|---|
| Road | `$CONNECTION_ROAD_DEAD s x1 y1 z1 x2 y2 z2` (sample `s=1`) | `$CONNECTIONS_ROAD_DEAD_SQUARE x1 y1 x2 y2` | Textured road line; rectangular asphalt, parking courts, and paved forecourts. |
| Airport | `$CONNECTION_AIRPORT_DEAD s x1 y1 z1 x2 y2 z2` (sample `s=-1`) | `$CONNECTIONS_AIRPORT_DEAD_SQUARE x1 y1 x2 y2` | Textured airport/tarmac line or rectangular area. |
| Pedestrian | `$CONNECTION_PEDESTRIAN_DEAD s x1 y1 z1 x2 y2 z2` (sample `s=1`) | `$CONNECTIONS_PEDESTRIAN_DEAD_SQUARE x1 y1 x2 y2` | In the tested game/build, both forms use the gravel ground texture (confirmed by the user's all-floor-types comparison); do not assume this means a paved sidewalk. |
| Water | `$CONNECTION_WATER_DEAD x1 y1 z1 x2 y2 z2` | `$CONNECTIONS_WATER_DEAD_SQUARE p0 x1 y1 x2 y2` | Water line or rectangular water area. The square example has a leading scalar (`p0`) whose meaning is unverified. |
| Mud road | `$CONNECTION_MUDROAD_DEAD s x1 y1 z1 x2 y2 z2` (sample `s=0`) | Not observed in the comparison | Textured mud-road line. |
| Trolley track | `$CONNECTION_TROLLEYTRACK_DEAD x1 y1 z1 x2 y2 z2 x3 y3 z3 x4 y4 z4` | Not observed in the comparison | Trolley-track line geometry; the editor emits four XYZ points. |
| Trolley lines | `$CONNECTION_TROLLEYLINES_DEAD x1 y1 z1 x2 y2 z2 x3 y3 z3 x4 y4 z4` | Not observed in the comparison | Trolley-line utility geometry; the editor emits four XYZ points. |

For the line properties, XYZ triplets are the observed endpoint/point coordinates; the road, airport, pedestrian, and mud-road variants also have a leading scalar (`s`) in the sample (`1`, `-1`, `1`, and `0`). Its meaning is not established, so preserve the editor-generated value instead of guessing. The XY pairs are the two rectangle corners in building-local coordinates. Observed square examples were `5.5147 4.0049 17.5180 17.9735` (road), `-16.5144 0.4822 -1.3613 16.5378` (airport), `-15.0612 -33.3294 -2.0876 -15.7728` (pedestrian), and `0.0000 2.8460 -51.1360 11.4206 -42.6012` (water). These are parameter shapes observed from the editor output, not a complete grammar specification; verify syntax and coordinate ordering there or in a comparable vanilla building before hand-authoring them.

Keep surface geometry separate from access points and construction costs. An ordinary `$CONNECTION_ROAD` access point does not itself create a rectangular asphalt court, and `$COST_RESOURCE_AUTO ground_asphalt` is a construction-cost coefficient, not a surface declaration.

Keep the roles distinct: `$CONNECTION_*_DEAD` entries define line-drawn textured runs, `$CONNECTIONS_*_DEAD_SQUARE` entries define rectangular native surface areas, and ordinary access properties such as `$CONNECTION_ROAD` define network access points. `$COST_RESOURCE_AUTO ground_asphalt` is a construction-cost coefficient and does not itself paint a surface. Do not add a ground mesh as a workaround when native area coverage is missing; inspect the generated Building Editor config and bounds instead.

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
