# Plan: Decision models for the Covenant

## Phase 1: The decision interface
- [ ] Task: The trait and the stock model
    - [ ] Write tests: the stock model returns Halo's own choice with probability one; a scripted model's answers are applied; a timeout keeps the stock choice
    - [ ] The `DecisionModel` trait, the state and question types in `campaign`
- [ ] Task: The state from perception
    - [ ] Write tests: a squad's state lists only targets its actors perceive; events (leader killed, grenade, losses) appear; the text rendering is stable
    - [ ] The state builder for each squad
- [ ] Task: The questions and the orders
    - [ ] Write tests: each option maps to an order Halo's AI has; options the scenario lacks are never offered; scripted orders override; cutscenes are untouched
    - [ ] The question sets for each species and rank; applying a chosen order to the squad
- [ ] Task: Conductor - User Manual Verification 'Phase 1: The decision interface' (Protocol in workflow.md) (deferred to the end)

## Phase 2: Running the models
- [ ] Task: The worker and its deadline
    - [ ] Write tests: a slow model never stalls the tick; a late answer is dropped; the cadence per squad holds
    - [ ] The worker thread, the cadence and the deadline
- [ ] Task: Recording decisions
    - [ ] Write tests: a replay of a commanded fight repeats its hash without the model; saves keep pending decisions
    - [ ] Decisions as simulation inputs in replays and saves
- [ ] Task: The System One client
    - [ ] Write tests: requests and responses against a fake loopback server; a non-loopback address is refused; malformed answers fall back
    - [ ] The HTTP client
- [ ] Task: The MLX backend
    - [ ] Write tests: the bridge loads a tiny test model and scores options; the memory check refuses under 16 GB unless forced; the GPU budget paces the model
    - [ ] The MLX bridge in the app, behind the FFI
- [ ] Task: Difficulty and settings
    - [ ] Write tests: Legendary takes the most likely option; easier difficulties sample or blend; the confidence threshold keeps the stock choice
    - [ ] `ai.commander` and its settings; the licence and setup in `docs/ai.md`
- [ ] Task: Conductor - User Manual Verification 'Phase 2: Running the models' (Protocol in workflow.md) (deferred to the end)

## Phase 3: The Covenant's thought, measured
- [ ] Task: The thought log
    - [ ] Write tests: `ai.thoughts` shows each decision with its options and probabilities
    - [ ] The shell command and overlay
- [ ] Task: The evaluation harness
    - [ ] Write tests: a headless encounter against the bot player reports time to clear, damage taken, survival and order variety (marked, needs `assets/maps`)
    - [ ] The harness for a10, b30 and c20; the dataset export
- [ ] Task: kev-4b in play
    - [ ] Write tests: the frame's p99 rises by at most 1 ms with the commander on; stopping the model mid-fight falls back without a stall
    - [ ] Tune the state and questions with kev-4b; the results in `docs/ai.md`
- [ ] Task: Conductor - User Manual Verification 'Phase 3: The Covenant's thought, measured' (Protocol in workflow.md) (deferred to the end)
