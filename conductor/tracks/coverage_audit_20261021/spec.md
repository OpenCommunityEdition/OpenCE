# Spec: Coverage of every map, a cutscene audit and a review

## Overview
When the earlier tracks are done, this track plays every map to find what is still wrong: objects out of place, objects not drawn, missing effects and sounds, scripts that stall, and cutscenes that do not match the original. It fixes what it finds, plays again, and ends with a review of the whole engine.

## Functional Requirements
1. **Coverage runs:** a test driver script (`.ocs`) for each campaign mission and each multiplayer map, run headless. A pilot plays each mission to its end, or flies through each map's clusters, and records:
   - objects whose placement differs from the reference trace (the placement checks of the dev tooling track);
   - objects that should be drawn and are not (in view, in the cluster, not hidden by a script);
   - shaders, effects, sounds or animations that fall back or are missing;
   - script functions that are unwired, and threads that stall or fault;
   - frames over the budget.
2. **Frames side by side:** at fixed points of each map, a frame from the engine and one from the reference app, for a person to compare. They are kept locally in `build/`, never committed.
3. **The cutscene audit:** each cutscene of the campaign is played in the engine and in the reference app, and compared: the camera's path, the actors' animations and positions, the dialogue's timing, the music, the effects and the fades. Differences are listed per cutscene.
4. **Iterating:** each finding becomes a fix in the right crate, with a test, and the coverage runs again until the lists are empty or every remaining item is agreed with the user.
5. **The review:** a review of the whole engine against the product guidelines and the tech stack: correctness, safety of the readers, performance, the documentation.

## Acceptance Criteria
- Every campaign mission plays to its end under the pilot, with no missing draws, no misplaced objects, no unwired functions and no stalled threads.
- Every cutscene's audit is empty, or each remaining difference is agreed with the user.
- Every multiplayer map is covered.
- The review's findings are fixed or agreed.

## Out of Scope
- Custom Edition maps beyond a sample set (they are covered by the import tests).
