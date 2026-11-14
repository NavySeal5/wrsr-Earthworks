# Known issues and limits

This list describes the 0.16.64 release candidate. It separates selected product limits from checks that have not passed.

## Selected limits

- Fuel storage and refuelling are not included. The visible facility tank is decorative.
- Terrain work does not consume fuel, money, or quarried stone.
- Distance contribution changes estimated and scheduled capacity. It does not set a maximum facility operating radius.
- TesmioLoader needs a separate plugin installation. Steam Workshop does not install the plugin DLL.

## Checks still open

- The user reports active playtesting over several days. Facility costs, terrain editing, Tesmio use, and saving/loading are reported as working. These are user-reported results; no logs or individual checklist results were supplied.
- Medium and Large facility appearance, access, and construction behavior still need review.
- RML fallback support needs a separate session check.
- The report does not identify whether Save As, autosave, corrupt companion data, or mismatched save recovery were tested.
- The maximum supported Site, Project, queue, and facility workload is not set.

See [release readiness](docs/RELEASE_READINESS.md) for the full checklist. Do not treat these open checks as passed.
