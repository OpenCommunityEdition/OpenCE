# `actor_moving.obj` — opus5 fresh-graphs lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_moving.c`; target `build/split/source/ai/actor_moving.obj`
  (January 2002 Xbox debug build).
- Compiler: VC7 `CL.Exe` 13.00.9254 from `xbox/bin/vc7`, repository flags
  (`/O2 /Oy- /DDEBUG /Dxbox` + include set from `build.ninja`); measured with
  `tools/campaign/gate.py --source ... --forbid-emitted-symbol _point_from_line3d`.
- Names: January map atlas (`scratch/atlas/actor_moving.obj.txt`, commit 47d9dd0da renamed the
  seven private owners), January assert strings in `.rdata`, HCEA per-function reconstructions
  (`halocea-review/src/blam/ai/actor_path_refresh.c`, `actor_destination_update.c`,
  `actor_move_*.c`) for semantics only, Ghidra decompilation as a hypothesis generator.
- Worker report with every shape and measurement: `scratch/workers/actor_moving.md`
  (candidate files `scratch/workers/actor_moving/*.c|obj`).

## 2. Validated baseline and result

| | exact | residual | unwritten | exact padded code bytes |
|---|---:|---:|---:|---:|
| lane HEAD | 20 | 6 | 10 | 2,848 / 20,352 |
| this lane (final real-file gate) | **21** | 14 | 1 | **3,568 / 20,352** |

Data: all 69 data/literal owners of the target are byte-identical in the final object (baseline 26;
the new owners are the string/float literals of the newly written functions).

Newly strict EXACT: `_actor_move_calculate_free` (720 padded, 717 meaningful, 53 relocations,
normalized sha16 `32046024d371d892`), emitted by its genuine January caller
`_actor_move_calculate_movement`.

Residual table (final object vs January):

| function | target | ours | class | first divergence |
|---|---|---|---|---|
| `_actor_path_refresh` | 1440/52 `b6b01d01` | 1440/52 `89b79760` | layout tie | three endpoint-check jcc choose the epilogue after the failure clear instead of the one after the inner clear |
| `_actor_destination_update` | 976/24 `4c6c5fda` | 976/24 `df9604dc` | x87/register tie | negated projection kept on FPU stack in January; ecx/edx swaps |
| `_actor_move_calculate_movement` | 2752/99 `970ee1ef` | 2752/99 `8eb4b04d` | frame/register | January homes the face-actor flag in `[ebp-1]` (frame 0x5c vs 0x58) |
| `_actor_move_calculate_controlled_by_aiming` | 640/16 `1832fd5e` | 640/16 `fbdbba5f` | scheduling | `*global_zero_vector3d` copy: load-all-then-store vs interleaved |
| `_actor_move_avoidance_setup` | 576/18 `3060b44f` | 576/18 `a1eeed08` | owner-blocked | exact only with `distance2d` (new SELECT_ANY owners) |
| `_actor_move_test_avoidance_vector` | 752/16 `271a1af1` | 544/17 `724a80ba` | inline boundary | January inlines `actor_move_transform_avoidance_vector` twice |
| `_actor_move_vector_avoidance_find_direction` | 240/2 `92f92b7c` | 240/2 `452690e2` | canonicalisation | dot product term order k,j,i vs i,j,k |
| `_actor_move_vector_avoidance` | 4144/135 `a8a8010c` | 3936/133 `b6b809ea` | inline boundary | January inlines normalize3d (5 sites) and one transform site |
| `_actor_move_update` | 3136 | 3088 | inline/spill | January inlines actor_path_has_path; crouch spilled |
| `_actor_move_try_evasion_direction` | 416 | 400 | register | attempt_count in `[ebp-4]` in January |
| `_actor_get_stopping_distances` | 464 | 464 | canonicalisation | biped dot i-term operand order |
| `_actor_move_try_evasion_vector`, `_actor_move_initialize` | — | — | unchanged | see older ledgers |
| `_actor_aim_jump` | parked | unchanged (256/4/`4087deef`) | parked | — |
| `_midpoint3d` | 64/3 | unwritten | header proposal | see §5 |

## 3. Accepted controls (retained source)

- `actor_path_refresh`: January case order raw / move-position / firing-position / prop, a
  separate `path_available` flag, `struct actor_debug_info *debug_info` bound after
  `distance3d`, `have_previous_destination` set before the point copy, asserts 2943/3004;
  `distance3d` imported with the file's existing rename + external-prototype pattern because
  January calls `_distance3d` out of line.
- `actor_destination_update`: `do { } while (step_reached)` step loop (January's loop is not
  rotated), char step index stored as `step_index + 1`, path pointer scoped to the valid block,
  assert 180 `final_step`, `ai_debug.print_unfinished_paths` error, tau-ceti guard, directional
  flying `(reverse ? -1 : 1) * 3` fallback, inlined `actor_path_clear` in the final else.
- `actor_move_calculate_movement` (now `static`, matching January storage class; no header
  declares it): single `actor_move_calculate_free` call site through a facing-vector pointer
  (January cross-jumps the call), explicit rotation-axis arithmetic, `acos`, console oversteer
  prints, asserts 1432/1532/1634/1649/1777.
- `actor_move_calculate_free` (EXACT) and `actor_move_calculate_controlled_by_aiming`:
  January private ABIs reproduced; 2D dots written as arithmetic (no new `_dot_product2d`
  COMDAT); `valid_real_normal2d` prototype from `ai/actor_looking.h`; normal2d/realcmp asserts
  expanded as `match_vassert` + `csprintf` (actors.c precedent); local `actor_facing` enum
  (HCEA DB enum values) for the four candidate directions.
- Avoidance graph: `actor_move_vector_avoidance` (static, ecx ABI), `actor_move_avoidance_setup`,
  `actor_move_test_avoidance_vector`, `actor_move_vector_avoidance_find_direction`; enum renamed
  to `VECTOR_AVOIDANCE_NUMBER_OF_DIRECTIONS` (January assert text); `object_get_bounding_sphere`
  imported like `normalize3d`; `PATH_EXTERNAL_FLEE_ROUTINES` opt-in for
  `path_input_set_target_object`; `main/console.h` for `console_printf`.

All candidates: 20 baseline exact functions preserved, `_point_from_line3d` guard passes, parked
`_actor_aim_jump` measurements unchanged, owner census adds only January code owners and literal
COMDATs that January references undefined, `tools/fake_match_scan.py` 0 leads, `/W3 /Zs` adds no
implicit declarations.

## 4. Experiment matrix (summary; full list in the worker report)

- path_refresh: v1 HCEA order (sha) → v2 January order (1440/52 sha, layout tie) → v3 else-chain
  (1456/53) → v4 nested if (= v2) → v5 early returns (1456/54) → v6 top-scope distance (1472/54).
- destination_update: d1 while/break (944) → d2 do/while (960) → d3 scoped path + point_from_line2d
  (976 sha) → d4a named `t` (976 sha) → d4b no step pointers (992).
- calculate_movement: c1 two free calls (2784/100) → c2 single call (2752/99) → c3 flag scope, c4
  store order (same).
- controlled_by_aiming: a/b/c/d/e/f/g (see report); best e (6-instruction scheduling diff).
- calculate_free: s1 sha → s2 EXACT → s3 EXACT without `_dot_product2d` (landed).
- avoidance_setup: p1 sha → p2 je/jp → p3 `MAX` + `distance2d` EXACT (probe and genuine caller, exact
  22) → owner-free spellings (sqrt components, magnitude2d, set_real_vector2d, vector_from_points3d
  cast) all sha.
- test_avoidance_vector: call form 544/17; explicit expansion 768/16; `__inline`/point_from_line3d
  transform spellings never inline (VC7 inlines 6-statement bodies here, not 9-statement ones).
- find_direction: ternary as real expression fixes ABI/frame; five dot spellings keep i,j,k order.
- midpoint3d: header body with `return result;` (scratch-only shadow) → EXACT.
- move_update: `actor_path_has_path()` call (3104/74), drop reload (3088/72).
- try_evasion_direction: for-loop (400), named actor (400).
- get_stopping_distances: argument swap (reverses all terms), explicit expression, three scratch
  associativity diagnostics (no match).

## 5. Orchestrator proposals

1. `source/math/real_math.h` `midpoint3d`: add `return result;` (pointer-returning inline falls off
   its end). Paired .c change: `actor_move_try_evasion_vector` uses
   `midpoint3d(&actor->input.position.head_position, &actor->input.position.body_position, &actor_midpoint)`
   (January computes all three components head+body). Evidence: atlas mangled signature
   `?midpoint3d@@YAPATreal_point3d@@PBT1@0PAT1@@Z` (`i`), scratch m2 exact. Consumers calling
   midpoint3d: `source/units/bipeds.c`, `source/ai/actor_moving.c`.
2. Accept `_distance2d` / `_vector_from_points2d` header-inline COMDAT copies in actor_moving.obj
   (already present in other base objects) → `_actor_move_avoidance_setup` EXACT (+576).
3. `source/ai/actor_definitions.h`: publish `_actor_definition_flags2_pathfinding_ignores_danger_bit = 4`
   (currently a local enum in actor_firing_position.c and actor_moving.c).
4. `source/ai/path.h`: move `path_input_set_target_object` out of the
   `PATH_EXTERNAL_FLEE_ROUTINES` opt-in (actor_moving.c is a second consumer).
5. `source/ai/ai_debug.h`: rename the placeholder `actor_debug_info` fields written by
   `actor_move_vector_avoidance` (list in the worker report).

## 6. Do-not-repeat list

- Do not retry `return` placement / else-chaining for path_refresh's exit block (6 forms).
- Do not retry dot-product argument order, explicit k+j+i sums, cross_product3d()->i, or local
  dot_product3d associativity shadows for the term-order ties.
- Do not try to make VC7 auto-inline `actor_move_transform_avoidance_vector` via `__inline`,
  point_from_line3d-inline bodies, or explicit duplication.
- Owner-free replacements for `distance2d` in avoidance_setup are all non-exact.
- Named/CSE/inline-parameter spellings of the negated projection in destination_update.

## 7. Reopen criteria

- Orchestrator accepts proposals 1 or 2 (mechanically proven exact).
- A same-compiler mechanism for per-call-site inlining (transform, normalize3d, actor_path_has_path)
  would reopen test_avoidance_vector, vector_avoidance and move_update.
- Evidence for January's commutative operand canonicalisation would reopen find_direction and
  get_stopping_distances.

## 8. Disposition

`actor_moving.obj` is still active, not Matching: 21 / 36 strict exact. Only
`_actor_move_calculate_free` receives new exact credit. All other new bodies are complete,
coherent fuzzy reconstructions with zero credit.
