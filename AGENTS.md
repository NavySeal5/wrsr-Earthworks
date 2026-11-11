# Earthworks Planning collaboration instructions

These instructions record the user's explicit workflow preferences for this project.

- Latest user instruction: "Dont do the tests yourself, just provide me with test instructions". The user performs all in-game tests from now on. Do not launch/control the game, trigger terrain edits, save/reload test worlds or perform gameplay verification on the user's behalf. Provide an identifiable build and clear test instructions, then use the user's reported results for stage acceptance. Read-only source/log inspection, implementation, builds and appropriate non-game checks remain allowed within authorized work; do not imply these are user in-game acceptance.

- Read IMPLEMENTATION_PLAN.md, docs/DECISIONS.md and docs/STAGE_REVIEWS.md before implementing a stage. The revised staged plan supersedes docs/archive/.
- TesmioLoader is the primary host. RML remains an optional fallback. The plugin uses Tesmio API 4 under either host. Do not load the plugin through both hosts in one game session.
- TesmioLoader does not load the plugin from a Workshop package path. Use `Install-TesmioPlugin.ps1` or copy the DLL to the game directory's `plugins` folder. Do not claim that Workshop subscription installs the Tesmio plugin automatically.
- Keep the native executable timestamp guard. The verified baseline is game 1.1.1.9, timestamp `0x6A3EB6AD`. If the installed executable differs, report that the plugin declines initialization. Do not update hook addresses until they are verified for that executable.
- Implement one authorized stage at a time. Every stage ends with an identifiable build, a short in-game checklist, observed evidence and a pause for user review.
- Do not start implementing the next stage until the user explicitly accepts the current stage and authorizes progression. A single reply such as "works, continue" is sufficient; do not ask twice for the same authorization.
- Current explicit exception: the user subsequently requested "Continue until the first prototype of the terraforming planning is finished". Continue implementation and in-game checks through that planning preview without pausing for intermediate stage acceptance. Important gameplay/UX questions still require user input. Office/execution work can be deferred until after this read-only planning prototype; resume review gates after delivering it.
- Latest Stage 11 instruction: after moving the former Stage 10b UI work to Stage 11, the user requested implementation continue until all agreed UI elements exist in a first version. This authorizes implementation through the Site editor without waiting between UI checkpoints; it does not constitute in-game acceptance or authorize agent gameplay testing. Provide one combined user-run review checklist when the first complete UI version is ready.
- Compilation, static checks, RML discovery and agent-only tests do not constitute the user's in-game acceptance. Silence and elapsed time are not acceptance.
- Ask the user before making important gameplay or UX decisions, including office design/fallbacks, terrain interaction/edge behaviour, staffing and machinery rules, range, costs, work rates, cancellation, persistence interaction and scheduling. Show concrete options and tradeoffs. Ask when relevant rather than presenting all future questions at once.
- The user has already chosen area selection with entered or sampled target height for the first planning prototype. Do not re-ask that settled question unless new evidence changes the scope.
- Routine technical choices, read-only investigation and repairs within an authorized stage can proceed without repeated confirmation. Continue independent work while an optional design question is pending, but do not implement the dependent choice without an answer.
- Keep proposed gameplay values and temporary diagnostic controls clearly labelled. Do not turn an unapproved prototype setting or engineering fallback into a production rule.
- Record user decisions and stage acceptance explicitly in the two logs, distinguishing expected outcomes from observed results. Never claim a check passed when it was not run.
- Use a disposable save for player-run checks. Do not load a normal republic with missing assets. The user performs in-game tests unless they explicitly ask the agent to do them.
- During plan-review-only work, edit documentation and records; do not start game tests or implement another stage without the user's instruction.

Current progress and the next authorized action belong in docs/STAGE_REVIEWS.md rather than being inferred from this file.
