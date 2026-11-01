**Local development setup — 1 November 2026**

The initial toolchain is installed and the diagnostic plugin builds successfully. RML is now the selected primary host, with the existing plugin ABI retained through RML's Tesmio API 4 compatibility layer. Gameplay implementation has not started.

| Component | Verified local version/status |
|---|---|
| Git for Windows | 2.43.0.windows.1; already installed |
| Visual Studio | Existing Community 2022 installation extended with native desktop/C++ components |
| MSVC x64 | Compiler 19.38.33130; toolset 14.38.33130 |
| Windows SDK | 10.0.22621.0, including resource compiler |
| CMake | 3.27.2-msvc1, bundled with Visual Studio |
| Ninja | 1.11.0, bundled with Visual Studio |
| TesmioLoader SDK | API 4; release tag tesmioloader0.3.6; commit 7d6bb8943ca5f6af817d8e7e6cf5be2c7faa818a |
| Republic Mod Loader | 1.0.1; source commit 57e567e5cccaa3c38aee7e9d599d2443d2460b10; pinned binaries match the installed Workshop item |
| Plugin host interface | Tesmio API 4 through RML's compatibility host; standalone TesmioLoader runtime unused |
| Source control | Project-local Git repository initialized on main; no remote or commit configured |
| Installer result | Success, exit code 0; no reboot required |

The Visual Studio component selections are recorded in `../.vsconfig`. Dependency versions and checksums are recorded in `../dependencies.lock.json`. Source headers and their upstream licence are available in the ignored `../external/TesmioLoader` checkout. No compiler or SDK was reimplemented or substituted.

**Build verification**

The project's `build.cmd` successfully configured CMake, compiled the diagnostic plugin and linked `build/windows-x64/bin/EarthworksPlanning.dll`. Warnings are treated as errors. The output was inspected for AMD64 architecture and the exports `TsmPluginApiVersion`, `TsmPluginInit` and `TsmPluginStart`. Debug symbols were generated for native debugging.

Windows also successfully loaded and unloaded the built DLL in a separate PowerShell process, and its exported API-version function returned 4. This verifies basic dynamic loading and ABI availability; it does not establish in-game compatibility.

The first sandbox-restricted compiler probe stalled; a normal build outside that restricted sandbox completed successfully. Future agent builds may need the same execution permission. Building from the user's own terminal does not require administrator privileges after tool installation.

**Runtime staging**

The isolated runtime is `tools/runtime/rml`, beneath this project. Staging copies the diagnostic DLL, PDB and optional RML metadata into `rml/plugins`. A separate `Earthworks Development` playlist selects the diagnostic plugin and excludes Workshop and other Development payloads. The preset enables Detailed Diagnostic Logging. The subscribed RML installation and its configuration are untouched. Staging refreshes this project-local test preset on each launch-script invocation.

Both original game binary hashes still match `game-baseline.json`: Steam build 23935965, SOVIET64.exe PE timestamp 0x6A3EB6AD and C3DDLL64.dll timestamp 0x6A3E75BC. Original game executables, content and saves have not been replaced.

The setup does not install a global DLL hook, startup service or automatic launcher replacement. `launch-diagnostics.cmd` explicitly opens the staged upstream launcher when run by the user.

**RML discovery verified**

The project-local RML 1.0.1 launcher was opened and inspected. Its dashboard recognized game version 1.1.1.9 and the recorded SHA-256. The active playlist was Earthworks Development, with one plugin enabled and zero Workshop items enabled. The Mod Manager displayed Earthworks Planning - Setup Diagnostic, ID `earthworks.planning`, as enabled; its details identified the Tesmio interface, Tesmio compatibility load path and AMD64/x86-64 PE32+ binary. No game launch was performed. The development launcher was left open on this plugin's details.

Dependency restoration, runtime staging, PowerShell syntax and JSON configuration validation passed after switching hosts. The staged DLL matches the previously built diagnostic binary. The launcher and runtime are the pinned RML binaries; current scripts do not start the standalone Tesmio launcher.

**Next checkpoint**

Stage 1's technical/in-game check has completed: RML initialized the diagnostic; a fresh vanilla-compatible world opened, was saved as Earthworks_Test_S01 with user assistance, reloaded successfully and exited normally with code 0. The session report contains zero hook failures/audit issues. See [STAGE_REVIEWS.md](STAGE_REVIEWS.md) for actual observations, limitations and preserved evidence. User acceptance remains pending.

After the user accepts Stage 1 and authorizes progression, Stage 2 adds read-only world/terrain observations. Workforce/machinery feasibility is Stage 4. Each stage has its own in-game review boundary. Ghidra and additional analysis tools can be installed when a concrete undocumented function requires them; the installed Visual Studio native debugger is the initial debugging environment.
