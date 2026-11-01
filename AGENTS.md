# Earthworks Planning collaboration instructions

These instructions record the user's explicit workflow preferences for this project.

- Latest user instruction: "Dont do the tests yourself, just provide me with test instructions". The user performs all in-game tests from now on. Do not launch/control the game, trigger terrain edits, save/reload test worlds or perform gameplay verification on the user's behalf. Provide an identifiable build and clear test instructions, then use the user's reported results for stage acceptance. Read-only source/log inspection, implementation, builds and appropriate non-game checks remain allowed within authorized work; do not imply these are user in-game acceptance.

- Read IMPLEMENTATION_PLAN.md, docs/DECISIONS.md and docs/STAGE_REVIEWS.md before implementing a stage. The revised staged plan supersedes docs/archive/.
- RML is the primary host. The current diagnostic uses its Tesmio API 4 compatibility layer; do not revert to a standalone TesmioLoader launch.
- Implement one authorized stage at a time. Every stage ends with an identifiable build, a short in-game checklist, observed evidence and a pause for user review.
- Do not start implementing the next stage until the user explicitly accepts the current stage and authorizes progression. A single reply such as "works, continue" is sufficient; do not ask twice for the same authorization.
- Current explicit exception: the user subsequently requested "Continue until the first prototype of the terraforming planning is finished". Continue implementation and in-game checks through that planning preview without pausing for intermediate stage acceptance. Important gameplay/UX questions still require user input. Office/execution work can be deferred until after this read-only planning prototype; resume review gates after delivering it.
- Compilation, static checks, RML discovery and agent-only tests do not constitute the user's in-game acceptance. Silence and elapsed time are not acceptance.
- Ask the user before making important gameplay or UX decisions, including office design/fallbacks, terrain interaction/edge behaviour, staffing and machinery rules, range, costs, work rates, cancellation, persistence interaction and scheduling. Show concrete options and tradeoffs. Ask when relevant rather than presenting all future questions at once.
- The user has already chosen area selection with entered or sampled target height for the first planning prototype. Do not re-ask that settled question unless new evidence changes the scope.
- Routine technical choices, read-only investigation and repairs within an authorized stage can proceed without repeated confirmation. Continue independent work while an optional design question is pending, but do not implement the dependent choice without an answer.
- Keep proposed gameplay values and temporary diagnostic controls clearly labelled. Do not turn an unapproved prototype setting or engineering fallback into a production rule.
- Record user decisions and stage acceptance explicitly in the two logs, distinguishing expected outcomes from observed results. Never claim a check passed when it was not run.
- Use an agreed disposable, vanilla-compatible test world for the initial RML profile, which disables Workshop content. Do not load a normal republic requiring disabled assets as a diagnostic shortcut.
- During plan-review-only work, edit documentation and records; do not start game tests or implement another stage without the user's instruction.

Current progress and the next authorized action belong in docs/STAGE_REVIEWS.md rather than being inferred from this file.
