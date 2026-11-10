# Stage 9 — Site priority and facility scheduling

These are user-run gameplay checks. The agent must not launch or operate the game. Use a disposable save or fresh test republic.

## Setup

1. Use two or more non-overlapping Sites in one named Project. Keep them away from roads, buildings, utilities and other infrastructure.
2. Assign at least one Earthworks facility with two or more eligible parked machines to that named Project. Facility assignments are exclusive to one named Project; machines are pooled among that Project's Sites.
3. Start both Sites explicitly. Arrange one Site above the other in the displayed priority order. Starting a Site does not start other queued Sites.

## FIFO allocation

1. With the simulation running, verify that only the highest-priority eligible started Site reports **Running** and receives all available capacity. Lower-priority started Sites should report **Waiting** and show no allocated machines/capacity.
2. Let the first Site make progress. Verify the lower-priority Site remains at the same progress until the first Site completes.
3. After completion, verify the next started Site receives the full available machine pool and begins advancing.
4. Pause the top Site. Verify that the next eligible started Site receives the pool. Resume the higher-priority Site and verify that it again receives the pool, with lower-priority started Sites waiting.
5. If the top Site needs terrain review, verify it is skipped while blocked and the next eligible started Site can work. Complete review and resume it; it should regain the pool according to the displayed order.
6. Reorder the Sites and verify the new order controls which started Site gets the pool. Save/reload and confirm the order persists and allocations are recalculated.

## Queue and project boundaries

1. Leave a lower-priority Site queued without pressing **Start**. Confirm it remains queued and does not begin automatically when earlier Sites complete.
2. Assign a different facility to another named Project. Confirm its machines do not change capacity or scheduling inside the first Project.
3. Try to confirm a Site that overlaps a queued, waiting, running or paused Site. The mod should reject it. A completed or cancelled footprint may be reused after replanning against current terrain.

## Report back

Report which checks you completed, which Site was at the top of the list, the displayed statuses/capacity, and whether order and progress survived save/reload. Do not use the disposable fixture or terrain-test buttons in normal republic saves.
