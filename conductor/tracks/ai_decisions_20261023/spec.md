# Spec: Decision models for the Covenant

## Overview
Halo's AI runs actors, squads and encounters with its own logic: perception, firing positions, morale, retreats and berserking. This track adds a pluggable decision layer above it, the **commander**. A few times a second, it chooses each Covenant squad's orders from the options Halo's AI already has, using a local decision model. The first model is [kev-4b](https://huggingface.co/jaredpalmer/kev-4b), a decision model built on Qwen3.5-4B-Base under Apache-2.0. Given a state document and typed questions, it returns calibrated probabilities over the options in one forward pass. The stock AI remains the fast controller that runs every tick, so the game never waits for a model. Without a model, play is the stock game.

## Functional Requirements

### The interface
1. **The decision model trait** (Rust, in `campaign`): it takes a state and a set of typed questions (`choice` over named options, `score` in a range) and returns a probability for each option, or a timeout. Implementations:
   - **Stock**, Halo's own logic: the default, and the fallback;
   - **System One client:** a loopback HTTP client for a System One-style endpoint (`POST /v1/systemone`), as kev-4b is served;
   - **In-process MLX:** the user's weights loaded with MLX, through a small bridge in the app;
   - a **scripted test model** for tests.
2. **The state** for each squad is built from what its actors perceive, never from what they cannot know. It is rendered as labelled text:
   - the actors: type, rank, health, shield, weapon, ammunition and morale;
   - their positions against the encounter's firing positions and cover;
   - the targets they know, with how long ago each was seen;
   - recent events: the leader killed, a grenade, a vehicle arriving, allies lost;
   - the encounter's other squads;
   - the difficulty.
3. **The questions** offer only orders the scenario allows:
   - advance, hold, or fall back to a firing-position group;
   - flank (choosing a group), regroup on the leader, or charge or berserk (Elites);
   - flee when the leader dies (Grunts, as Halo already does);
   - throw grenades, take or leave a vehicle, or call the encounter's next squad where the scenario provides one.

   A model cannot create actors, positions or weapons.
4. **The scripts always win:** command lists, `ai_migrate`, `ai_magically_see` and other scripted orders override the commander, and cutscenes and scripted moments are not touched.
5. **The Covenant's character:** each species and rank has its own state fields and question sets:
   - Elites lead and charge;
   - Grunts panic without leaders;
   - Jackals hold with shields;
   - Hunters pair up.

   A **thought log** in the shell (`ai.thoughts`) shows each decision, its options and their probabilities.
6. **Difficulty:** Legendary takes the most likely option. Easier difficulties sample from the probabilities, or blend them with the stock choice. Below a confidence threshold, the stock choice is kept.

### Running the models
7. **Only the host runs a model**; clients see the results through the existing network state.
8. **Off the tick:** decisions are asked on a worker thread at 2–5 Hz a squad, each with a deadline (250 ms by default). A late answer is dropped and the stock choice stands.
9. **Recorded decisions:** each decision the simulation acts on is an input, recorded in replays and saves. A replay repeats exactly without the model.
10. **Settings** (`ai.commander`): off (the default, the stock game), System One endpoint, or MLX with a weights path. The cadence, the deadline and the GPU budget can also be set. The engine makes no network calls except to loopback, and downloads nothing itself. The user fetches the weights, whose licence (Apache-2.0) the documentation names.
11. **Machines with less than 16 GB of memory:** the MLX backend is refused there unless forced, because kev-4b in bf16 needs about 8 GB.

### Measuring the model
12. **The evaluation harness** plays headless encounters from a10, b30 and c20 against a scripted bot player, with the stock AI and with each model. It reports the time to clear, the player's damage taken, the AI's survival and the variety of orders.
13. **A dataset export** writes (state, stock choice, outcome) records, for training a later model.

## Non-Functional Requirements
- With the commander on, the frame's p99 rises by at most 1 ms on the M1 Max. The model's GPU work is paced so it does not take frames from the renderer.
- No network version change: decisions run only on the host.

## Acceptance Criteria
- The a10 and b30 encounters play with kev-4b as commander. The thought log shows sensible orders, and the scripts still drive every scripted moment.
- With the model stopped mid-fight, play continues on stock AI with no stall.
- A replay of a commanded fight repeats the world's hash without the model.
- A manual check that the Covenant feel like the Covenant: Grunts flee, Elites lead.

## Out of Scope
- Shipping or downloading model weights.
- Generated dialogue or chat.
- Cloud models.
- Multiplayer bots.
- Fine-tuning a model (only the export is in scope).
- The Flood and the Sentinels. The interface allows them, and they are a later addition.
