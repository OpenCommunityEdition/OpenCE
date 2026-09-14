# `effects.obj` Opus5 150K house-clean lane, wave w2: park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/effects/effects.c`, target `build/split/source/effects/effects.obj`.
- Compiler VC7 13.00.9254 (XDK 3911). Repository flags unchanged. No header, config or flag edits.
- Lane `opus/150k-house-clean-20260914`, base `12f7375d4`. Worker `w2:effects`.
- All five effects residuals are parked and locked. Every experiment therefore ran on scratch copies under
  `scratch/workers/effects/`. **The production file is unchanged** (`git diff --stat -- source/effects/effects.c` is empty).
- Worker notes with per-function blocks: `scratch/workers/effects.md`.
- Harness: `scratch/workers/effects/try.py` replaces named bodies in a copy of the real file, gates the whole TU with
  `--forbid-emitted-symbol _point_from_line3d`, and prints only the rows that changed.
- Evidence used:
  - the January target COFF (disassembly, relocations, jump tables);
  - the earlier ledgers `effects_obj_opus_reconciliation_20260904.md` and `effects_obj_opus5_100k_waveD_20260914.md`, plus the wave D worker notes and bodies;
  - `config/parked.json`;
  - the later HCEA per-function sources (semantics only);
  - exact January code elsewhere in the tree (`projectiles.c`, `units.c`), used for statement-order corroboration.

## Baseline (real file)

`== exact 36  residual 5  unwritten 0  (of 41 listed)`. The point guard passes and there is no park drift.

| residual (parked) | target | base | tag |
|---|---|---|---|
| `_effect_allowed_by_environment` | 128/10 | 112/10 | size |
| `_effect_evaluate_function_integral` | 176/17 | 176/16 | relocs |
| `_effect_generate_part` | 992/27 | 976/28 | size, relocs |
| `_effect_generate_particles` | 1888/46 | 2048/55 | size, relocs |
| `_effect_get_node_matrix` | 80/3 | 80/3 | sha |

## Results

### Park-reopen proposals (strict EXACT in scratch)

The candidate is `scratch/workers/effects/cand_final.c` (sha256 `f3df56f4dd15eb70…`). It is the current real file with only these two bodies replaced.

**Candidate checks:**
- Gate `== exact 38 residual 3`. Only the two rows below change, and all 36 baseline-exact rows stay exact.
- The point guard passes.
- `scratch/parkcheck.py`: only these two parks drift, both to the target hash. The other three parks are at base.
- Owner census vs `build/base` and `build/split`: identical.
- `tools/fake_match_scan.py`: 0 leads.

1. **`_effect_get_node_matrix`**: 80 bytes, 3 relocations, sha `0d95929225eaea99`.
   - **Evidence.** January's NONE arm is `or eax,-1; push eax`. The NONE constant enters a register phi, which means a node-index variable. A direct `NONE` or `node_designator` argument is constant-propagated to `push -1`. Both object tails load `object_index` before the value, so the tail is one call statement that VC7 duplicated late. The first-person arm falls through, so it is the `&&` test first.
   - **Source.** Declare `short node_index;`. Test `if (node_designator != NONE && TEST_FLAG(node_designator, first_person))` and return the first-person matrix. Then `if (node_designator == NONE) node_index = NONE; else node_index = designator & mask;` and finally `return object_get_node_matrix(effect->object_index, node_index);`.
   - **Park criterion.** "A natural same-compiler source shape that controls the arm order": MET.
   - **Shapes tried.**
     - Nested if: `push -1`, unchanged.
     - NONE-first early return: 64 bytes.
     - `&&` plus the file's designator/`&=` idiom: 64/2.
     - `&&` plus a `?:` local: movsx/ECX, and generate_parts breaks.
     - if/else assignment: EXACT.
2. **`_effect_evaluate_function_integral`**: 176 bytes, 17 relocations, sha `d68c4243e967a0ca`.
   - **Evidence.** January keeps the `fraction == -1` zero return inline and unmerged. The end case's `0.0` and `1.0` behave as phi arms of a result variable: the `1.0` arm merges with case start's `1.0` before return duplication, and the `0.0` arm stays separate. That is the single-exit law applied to the switch. The end case is `test ah,5; jp one`, which is `fraction < 1.0f`. The default path (`add esp,14h; fld [ebp+0Ch]`, shared with case constant) returns the input fraction after the fatal assert. HCEA notes a result local for an out-of-range function.
   - **Source.** Declare `real integral;`. Keep the early `-1` return. Assign `integral` in each case. The default arm is `match_vassert(..., FALSE, NULL); integral = fraction; break;`. End with `return integral;`.
   - **Safety.** There is no uninitialised read: the fallback is the value January's machine code actually returns on that path.
   - **Park criterion.** "A natural shape that explains the selective zero-tail separation": MET.
   - **Shapes tried.** In the resumed run:
     - `integral = fraction` after the check: 208 bytes.
     - if/else `-1` plus `?:`: 176/16.
     - Reusing the parameter as the accumulator: 176/17, sha only.
     - Uninitialised result, as a scratch probe only: EXACT.
     - The w4 form above: EXACT.
     - The same with default falling through into a trailing constant case: EXACT, but not preferred because it breaks enum case order.

     The earlier cut-off run tried e3a (init at declaration, 224), e4 (switch nested under `!= -1`, 176/16), e5 (end case `?:`, 192/18) and f2r (end case `<` returns, 176/16).

### Owner-blocked (no admissible exact spelling)

- **`_effect_allowed_by_environment`**
  - **January bytes read an undefined slot.** The `push ecx` frame slot is never written on any path, and the default arm returns `mov al,[ebp-1]`. Any safe definition compiles to 112 bytes with no frame.
  - **New measurement:** the integral default-fallback lever does not transfer. `allowed = FALSE` after the assert is 112 bytes and is inlined into both callers; generate_parts regresses to 688/24.
- **`_effect_generate_particles`**
  - **Env inlining.** Every safe environment helper is inlined at the depth-3 call site. This was re-measured with the two reopens applied: 2048/55 or 1984/55. Only the UB env stays out of line.
  - **Probes.** Scratch probes used the UB env as evidence only. From wave D's gp_n1 body, a named `real tint_fraction` for the tint alpha (x87 phi law) reproduces January's `[ebp-4]` alpha temp: 1888/46, sha only.
  - **Remaining residual under the probe:**
    - inner-loop trip-count hoisting and guard elimination: January hoists a movzx trip copy to `[ebp-0xcc]` with no per-instance guard;
    - attached-arm store scheduling;
    - random-range register permutations;
    - the inlined node-matrix designator load form.
  - **Rejected probes.** A `long` index gives movsx and keeps the guard. An if/else `NONE` node_index in the attached arm materialises `mov eax,-1`, whereas January stores the designator register. An if-block instead of `continue` is identical.
- **`_effect_generate_part`**: four new January-evidenced corrections leave **only** the csprintf destination.
  - **Light and sound arms.** `mov eax,0FFFFFFFFh` in an if/else: the same node_index if/else assignment as the node_matrix reopen.
  - **Decal arm.** Upper and lower bounds are homed in two real slots, and `[ebp-8]` is reused for the result. These are the parameters of the inlined real_math.h `__inline real_local_random_range(lower, upper)`. This recovers the park's "missing four-byte local", and frame `0xb8` is now exact.
  - **Object arm.** The angular-velocity arguments are pushed before the seed call, so the seed is a direct argument.
  - **Damage arm.** `+0x28` is written before `+0x1c`, which is `damage.epicenter = *world_point; damage.origin = *world_point;`. That statement order matches exact January `projectiles.c` (lines 1640-1641 and 1702-1703) and `units.c` (lines 11394-11395).
  - **Result.** `scratch/workers/effects/gpart_p4.txt` compiles to 1008/28, and alndiff differs only by `push offset temporary` / `add esp,10h`. The scratch probe with January's malformed three-argument csprintf (`gpart_p4ub.txt`, inadmissible) is STRICT EXACT at 992/27.

## Rejected / do-not-repeat (this wave)

- node_matrix:
  - a nested-if NONE arm passing `node_designator`, which VC7 constant-propagates;
  - a NONE-first early return;
  - `&&` plus a `?:` local.
- integral:
  - result init at declaration or after the `-1` check;
  - if/else `-1` with a `?:` end case;
  - reusing the parameter as the accumulator;
  - the uninitialised result (UB, a probe only).
- env: `allowed = FALSE` after the assert, which inlines into both callers.
- generate_particles:
  - if/else `NONE` node_index in the attached arm;
  - a `long` inner index;
  - an `if (count_delta > 0)` block.
- generate_part: nothing rejected; the p1 to p4 corrections are cumulative.

## Orchestrator proposals

1. **Park-reopen:** `_effect_get_node_matrix` and `_effect_evaluate_function_integral` from `scratch/workers/effects/cand_final.c`, as one object commit. The parked entries for both must be removed.
2. **Optional park re-baseline, zero credit:** `_effect_generate_part` body from `gpart_p4.txt`. The combined candidate `scratch/workers/effects/cand_rebase_part.c` gives exact 38, only three rows change, and the fake scan has 0 leads. Its park base would become 1008/28 sha `7d64ae2978c7bd10`.
3. **Owner rulings (unchanged questions, now quantified):**
   - **(a) Read reachable only if `system_exit(-1)` returns.** This unlocks env (128) and is the only way env stays out of line in generate_particles.
   - **(b) January's malformed csprintf on the fatal assert path.** This is the only remaining difference in generate_part (992).

## Reopen criteria

- env: owner ruling (a), or a safe spelling that VC7 declines to inline at loop depth 3 while generate_parts stays exact.
- generate_particles: ruling (a), plus source shapes for the trip-count hoisting and attached-arm scheduling. Scratch base: `gp_g3.txt`.
- generate_part: ruling (b). Candidate: `gpart_p4ub.txt`.

## Final real-file gate

`== exact 36 residual 5 unwritten 0`, identical to the baseline row by row. The point guard passes, park drift is 0/5, the owner census is identical, and the fake scan has 0 leads. `newly_exact` is empty because the two reopens are proposals for the orchestrator.
