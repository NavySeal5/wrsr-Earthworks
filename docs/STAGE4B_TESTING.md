**Stage 4b — dedicated Earthworks facility and ordinary-job exclusion**

Build: **0.5.0-dedicated-facility**. The launch script stages a local Workshop item with ID `9100000012` beside the RML plugin. The test building uses the native `construction_office3` model and configuration as a temporary shell, with **5 worker places** and **2 vehicle station placements**. It remains visibly labeled as a test item; the values and model are not release decisions.

The user chose a dedicated building and asked the plugin to block ordinary construction jobs from it. In this iteration, selecting **Register selected building as Earthworks station** registers that exact building instance for the current loaded world. The plugin intercepts the native office assignment routine for that instance. Registration clears when the world reloads. Do not register an ordinary construction office.

**Launch and place the facility**

1. Close both the game and project-local RML. Keep Steam open. Run `earthworks-mod\launch-prototype.cmd`. Confirm **Earthworks Planning - Dedicated Facility Prototype**, version **0.5.0-dedicated-facility**, in `Earthworks Development`; launch from RML.
2. Load a disposable vanilla-compatible save. In the construction-office building category, find **Earthworks Development Facility (test)** and place it. Confirm the native panel shows 5 worker places and 2 vehicle station placements. It uses donor construction-office art so the item is easy to replace later.
3. Confirm you can put workers and two test machines in its own slots using native controls. Do not transfer vehicles from an ordinary construction office; purchase/assign test vehicles directly to the facility if the native menu allows it. Report if the worker or machine slots do not appear as expected.

**Register and check the exclusion**

4. Open **Earthworks → Stage 4 | Read-only workforce inspector → Find buildings**. Click near the test facility and choose it from the nearby origins. Check its position and magenta marker. Click **Register selected building as Earthworks station**. Confirm the selected ID is marked registered.
5. Using native construction controls, attempt to assign this facility to an ordinary road or building project. The project should remain unassigned to this facility, and **blocked native assignments** should increase. Try the ordinary office pick workflow and, if available, the area-office assignment tool. Also try the office window's automatic **add construction** control if available; this route uses a different native routine and is not directly hooked by this build. If it assigns a job to the Earthworks station, stop the test and report it; the exclusion needs another implementation pass.
6. Assign a separate ordinary construction office to a disposable project. It should still assign normally and should not increment the Earthworks blocked count.
7. Close and reopen Earthworks; the registered marker and blocked count should remain for this loaded session. Save/reload: registration should clear. Re-select the facility and register it again. Verify ordinary assignments to it remain blocked after re-registration.
8. Confirm normal building controls/camera and the existing area/height preview still work. Avoid the Stage 3 raise/lower buttons; terrain mutation is not part of this test.

**Report back**

- Do the 5 worker places and 2 vehicle stations appear and accept the intended occupants?
- Which ordinary job-assignment routes did you try, and did the blocked count rise while the facility stayed off the project?
- Did a normal construction office continue to work?
- Did registration clear on reload and work again after re-registering?
- Any crash, message, odd native panel behavior, or slot-count mismatch?

The assignment filter covers the native function at EXE RVA `0x161C90`, which the pinned RepublicMP call map identifies for construction-office window, office pick and area-office assignment paths. The automatic add-construction control uses EXE RVA `0x1561C0` and is not directly hooked; the in-game check must establish whether it passes through the filter. If any route bypasses the filter, report it and stop using the test facility for normal jobs until that path is handled.

The station is session-registered; persistent or multiple station registration has not been designed. This iteration does not execute terrain work, claim machine skills/fuel/readiness, or establish productivity, cost, operating range or release capacities. No in-game test was performed by the agent.
