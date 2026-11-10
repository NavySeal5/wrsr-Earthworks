# Earthworks settings check

Build `0.16.58-global-settings-site-speed` and launch the game with the prototype launcher.

1. Open Earthworks. Select the gear icon in the panel header.
2. Confirm that the Settings panel shows **Site construction speed** and **High contrast colors**.
3. Set Site construction speed to `2.0x`. Check that the assigned capacity and estimated completion time update. Start a Site and confirm that it advances faster than at `1.0x`.
4. Enable **High contrast colors**. Confirm that Site outlines and point markers use the brighter palette. Disable it and confirm the regular palette returns.
5. Close the game and relaunch it. Confirm that both settings keep their values.
6. Load a different save. Confirm that both settings still use the same global values.

The global settings file is `%LOCALAPPDATA%\EarthworksPlanning\settings.ini`. Site progress remains in the republic save data.
