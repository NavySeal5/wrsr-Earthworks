# Earthworks Planning release readiness

**Status:** Not ready for release. This checklist records the repository state on 14 November 2026. A build or a package stage does not count as an in-game result.

## Confirmed candidate scope

The user confirmed this candidate scope on 14 November 2026. Decision D163 records it. This confirms the candidate features. It does not accept the current build or approve public distribution.

- Earthworks Planning, with named Projects, editable Sites, terrain preview, and scheduled terrain work.
- Small, Medium, and Large Earthworks facilities.
- TesmioLoader as the primary host and Republic Mod Loader (RML) as an optional fallback.
- No fuel storage or refuelling. The facility tank is decorative.
- No fuel, money, or quarried-stone accounting for terrain work. The decision log records these as future work.
- No public upload as part of this candidate review. Decision D15 keeps publishing separate.

The current compatibility reference is Workers & Resources: Soviet Republic 1.1.1.13, Steam build 24922914, and TesmioLoader b0.3.7 with Tesmio API 4. The plugin also has an RML fallback. The plugin does not reject a game by whole-file hash or executable timestamp. Native hooks check their target signatures. This is the recorded target, not a claim that the latest build has passed in-game compatibility checks.

Do not load Earthworks through TesmioLoader and RML in the same game session. Workshop subscription does not install the Tesmio plugin DLL. The player must use the package installer or install the DLL in TesmioLoader's `plugins` folder.

## Current evidence

- Build `0.16.63-distance-based-facility-contribution` is the latest recorded player build. The x64 plugin build and combined local package staging succeeded.
- No automated or in-game tests were run for that build. Its player checks remain open in [the stage review log](STAGE_REVIEWS.md).
- Stage 13 has not been accepted for the current build.
- Stage 12's facility UI is accepted. The Small facility is accepted. The latest Medium and Large model/config changes still need the documented user checks.
- Tesmio migration has no recorded in-game acceptance. The RML path is also not verified as a fallback for this release candidate.
- The existing package is labelled as a playtest package. It does not contain the source models, third-party notices, or debug materials. The final release package must include the source and notices required for distribution.

## User-run release checks

Run these checks with the same identified candidate build. Use a disposable save. Do not use a normal republic that depends on disabled mods. Record observations, logs, and the exact build for each result.

### 1. Install and loader

1. Install through TesmioLoader using the package installer. Confirm the DLL is copied to the intended loader folder and the plugin is enabled.
2. Launch the game through TesmioLoader. Confirm the main menu and a disposable world load. Check loader and game logs for initialization errors or failed native hook signatures.
3. Open and close the Earthworks panel. Confirm that the normal game controls still work around it.
4. If the release will advertise RML support, run a separate session with only RML enabled. Confirm plugin discovery, launch, and the same basic panel workflow.

### 2. Facilities

1. Place and construct each facility size. Confirm appearance, footprint, road access, parking, loading positions, and demolition cleanup.
2. Confirm the facilities appear without a research prerequisite. Check the construction crane during brick-laying and roof phases, and confirm it is not needed during groundworks.
3. Check the displayed material totals for Small, Medium, and Large against the approved construction-cost notes.
4. Confirm that bulldozers and excavators can use the facilities, and that other vehicle types and ordinary construction jobs cannot.
5. Assign facilities to Projects. Save and reload. Confirm assignment identity, exclusivity, and missing-facility behavior.

### 3. Sites and terrain

1. Create and save a flat polygon and a concave polygon. Check point spacing, shape finish, cut/fill estimate, and the footprint boundary.
2. Set different point heights. Confirm that preview, slopes, and ramps match the saved target surface.
3. Edit a saved Site by moving, adding, and removing points. Confirm invalid geometry is rejected with a clear message.
4. Start, pause, resume, review, and complete a small Site. Compare the terrain result with its preview. Confirm edits pause active work and require review before work resumes.
5. Check occupied or protected terrain and overlap rules in the intended gameplay setting. Confirm Earthworks does not change terrain outside the Site's accepted influence area.
6. Save and reload during draft, queued, running, paused, edited, and completed states. Include Save As and autosave if these are part of the intended release workflow. Confirm mismatched or corrupt companion data never resumes terrain work automatically.

### 4. Scheduling, rates, and settings

1. Start more than one Site in a Project. Confirm FIFO order, pause/review skip behavior, and facility pooling.
2. Check a known Speed/Level 21 or 25 vehicle at 1×. Confirm the displayed capacity is 5,250 or 6,250 m³ per in-game day per vehicle. Change the global speed setting and confirm work rate and estimates scale together and persist after restart.
3. With distance contribution off, confirm distance does not reduce capacity. Turn it on and confirm 100% at 3,500 m, 50% at 5,000 m, and 25% at 10,000 m, with linear changes between thresholds. Change the thresholds, confirm their order stays valid, and confirm estimates update while paused and persist after restart.
4. Confirm the high-contrast setting persists and changes display colors only.

### 5. Final package

1. Install the package into a clean test setup. Confirm it contains all required building assets, plugin files, setup instructions, source code, and license notices.
2. Upgrade from the prior playtest package. Confirm the installer enables the intended plugin and does not leave an old DLL or manifest that loads twice.
3. Disable or remove the plugin and confirm the game launches normally. Check how saved Earthworks data behaves when the plugin is disabled, then re-enabled.
4. Run the agreed maximum Site, Project, queue, and facility load. Record any frame-time impact or limit.
5. Review player-facing labels, error messages, controls, and setup instructions.

## Release blockers

- User-run checks above have not been recorded for the current build.
- Stage 12 model, access, construction, and cost reviews are not complete for every facility size.
- Stage 13 geometry and editing are not accepted for the current build.
- Intended gameplay settings and practical workload limits remain open under D14. Fuel use and terrain material accounting are outside the confirmed candidate scope under D163. Operating range limits still need review; D162's optional distance contribution does not define a maximum operating radius.
- The final package needs a source and license review. The current playtest package is not the final distribution package.
- Confirm performance limits and define the supported maximum workload.
- Decide separately whether to publish the candidate publicly.

## Documentation issues to fix before handing off the candidate

- Some older Stage 13 instructions tell the player to launch the project-local RML profile. TesmioLoader is now primary. Update the instructions to match the selected loader and candidate package.
- The top status matrix and the end of `IMPLEMENTATION_PLAN.md` contain old stage and next-action notes. Keep one current status and point old notes to the review history.
- `CMakeLists.txt` reports version 0.16.14 while the plugin and RML manifest report 0.16.63. Resolve this in the versioning and packaging step.
- The package script preserves stale files in its output. Make the release package reproducible and verify its file list before distribution.
