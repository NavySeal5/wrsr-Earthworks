# Earthworks launcher position and overlap check

Build `0.16.60-fixed-launcher-under-native-ui` and launch the game with the prototype launcher.

1. Open a world. Confirm the Earthworks button stays at the lower-left position, to the right of the game menu and above the bottom toolbar.
2. Open or move a vanilla panel over the button. Confirm the vanilla panel appears in front and covers the overlapping part of the button. Confirm the button does not move.
3. Click the covered area. Confirm the vanilla panel receives the click. Close the vanilla panel and confirm the Earthworks button is visible in its original position.
4. Click the visible Earthworks button. Confirm it opens and closes the Earthworks panel. Confirm the in-game cursor remains visible over the button.

The button is drawn before the native interface, so vanilla panels cover it by draw order. The game was not launched for this check by the developer.
