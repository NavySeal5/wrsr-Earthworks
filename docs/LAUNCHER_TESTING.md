# Earthworks launcher cursor and overlap checks

Close the game and TesmioLoader. Install build `0.16.65-cursor-occlusion` with `scripts\Install-TesmioPlugin.ps1` from the source checkout. Open the game through TesmioLoader and load a disposable save. In `tesmioloader.log`, check for `native cursor draw signatures matched 30/30`.

1. With the normal pointer, move over the Earthworks button. The button stays visible at the lower-left. Click it to open and close the Earthworks panel.
2. Select a road tool and move over the Earthworks button. Repeat with a rail, power-line, or pipe tool. The button stays visible. Click the button and confirm that it opens Earthworks without placing infrastructure.
3. Where available, repeat the hover check with the free-move pointer, radiation meter, and a vehicle stop tool.
4. Move a vanilla panel so that it overlaps only one edge of the button. The full button disappears. Click a control on the overlapping panel. The control responds, and Earthworks stays closed. Move the panel away. The button returns to its original position.
