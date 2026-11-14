# Known issues and limits

This list describes the 0.16.64 release candidate. It separates selected product limits from checks that have not passed.

## Selected limits

- Fuel storage and refuelling are not included. The visible facility tank is decorative.
- Terrain work does not consume fuel, money, or quarried stone.
- Distance contribution changes estimated and scheduled capacity. It does not set a maximum facility operating radius.
- TesmioLoader needs a separate plugin installation. Steam Workshop does not install the plugin DLL.

## Checks still open

- No user-run in-game check is recorded for version 0.16.64.
- The latest Medium and Large facility placement, access, construction, and cost changes need review.
- Polygon editing and terrain execution need review on the current candidate.
- The Tesmio migration has no in-game acceptance record for the candidate. RML fallback support also needs a separate session check.
- Save As, autosave, corrupt companion data, and mismatched save recovery were not all reported as tested.
- The maximum supported Site, Project, queue, and facility workload is not set.

See [release readiness](docs/RELEASE_READINESS.md) for the full checklist. Do not treat these open checks as passed.
