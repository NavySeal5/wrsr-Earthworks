# Earthworks launcher placement check

Build `0.16.59-launcher-avoids-native-ui` and launch the game with the prototype launcher.

1. Open Earthworks in the normal map view. Confirm its launcher stays in a clear area.
2. Open native panels and switch build categories. Confirm the launcher moves when needed and does not cover their controls.
3. Move the mouse over and around the launcher. Confirm the in-game cursor remains visible and the button opens the Earthworks panel.
4. If a screen layout leaves no clear area, confirm no launcher is drawn over native UI. Close the native panels and check that the launcher returns.

The launcher placement uses the game's captured UI rectangles when available. The game was not launched for this check by the developer.
