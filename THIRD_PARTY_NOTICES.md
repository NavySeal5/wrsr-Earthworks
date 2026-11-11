The primary runtime is Republic Mod Loader: https://github.com/Ultimate-Universe/WRSR-RepublicModLoader.

Pinned RML release: 1.0.1, source commit `57e567e5cccaa3c38aee7e9d599d2443d2460b10`. Original binaries and the brand asset are copied from the pinned repository's `ModFiles` into the isolated runtime. SHA-256 hashes match installed Workshop item `3787969749` and are recorded in `dependencies.lock.json`. The full GPLv3 licence remains in `external/RepublicModLoader/LICENSE`.

The plugin uses the Tesmio API 4 ABI. TesmioLoader is the primary host; RML can provide the API as a fallback. The SDK headers come from https://github.com/MaxLegend/TesmioLoader, tag `tesmioloader0.3.6`, commit `7d6bb8943ca5f6af817d8e7e6cf5be2c7faa818a`. The full GPLv3 licence is preserved in `external/TesmioLoader/LICENSE`. The project-local prototype launcher uses its ignored TesmioLoader runtime. A player must install TesmioLoader and install the Earthworks DLL in its `plugins` directory; Workshop content alone does not install the DLL there.

The preview prototype adapts native binding addresses, camera projection and input/overlay techniques from [RepublicMP](https://github.com/E3418/RepublicMP), E3418 and contributors, pinned at `630775724abd5e3346eb59f9c66e2873128d74a4`. Adapted runtime/UI files carry `SPDX-License-Identifier: GPL-3.0-only`. The GPLv3 text is preserved in `LICENSES/GPL-3.0.txt` and in the upstream checkout. The local prototype is not published; any distribution of the combined plugin must comply with GPLv3, including corresponding source and notices.

Dear ImGui is built from that pinned checkout's `third_party/imgui` directory. Copyright (c) 2014–2026 Omar Cornut and Dear ImGui contributors; MIT licence preserved in `LICENSES/Dear-ImGui-MIT.txt`. Its Win32 and Direct3D 11 backends are included. Multiplayer, networking and RepublicMP simulation patches are not compiled into this plugin.

Visual Studio, MSVC, the Windows SDK and Git remain separately installed tools under their respective licences; they are not bundled into a mod release by these scripts.
