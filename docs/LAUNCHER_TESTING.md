# Earthworks launcher checks

Close the game and TesmioLoader. Install build `0.16.67-panel-shortcut` with `scripts\Install-TesmioPlugin.ps1` from the source checkout. Open the game through TesmioLoader and load a disposable save.

1. Move a game panel over the launcher. The launcher stays visible above the panel. Click its label to open and close Earthworks. The game control under the launcher keeps its current state.
2. Hover over the six-dot grip. Check for the move tooltip. Drag the grip across the game panel. The launcher moves and the Earthworks panel keeps its current state. The game panel accepts clicks in the area the launcher cleared.
3. Select a road tool. Click the Earthworks label, then drag the grip. Check that both actions leave the road plan empty.
4. Place the launcher near a screen edge. Change the window size or resolution. Check that the full launcher stays accessible. Return to the original window size and restart the game. Check that the launcher returns to the position you saved.
5. Close the Earthworks panel. Press **Ctrl + comma**. The main panel opens. Hold the shortcut, then press it again. The panel stays open.
6. Minimize Earthworks and press **Ctrl + comma**. The panel expands. Repeat with a road tool selected and check that the shortcut opens the panel without changing the selected tool.
7. Press comma by itself in an Earthworks text field. Check that it enters a comma. Return to the main menu and check that **Ctrl + comma** has no effect.
