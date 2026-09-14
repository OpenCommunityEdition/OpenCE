# `players.obj` Opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/game/players.c`; target `build/split/source/game/players.obj`.
- Lane worktree `opus5-150k-house-clean-20260914`, branch `opus/150k-house-clean-20260914`, base `12f7375d4`.
- Compiler: repository XDK 3911 `CL.Exe` (VC7 13.00.9254). Flags unchanged, and no `--cflag` was used.
- **The real `players.c` is unchanged** (`git diff --stat -- source/game/players.c` is empty). No header, config, park entry or tool was edited.
  - Every exact result in this wave belongs to a function parked in `config/parked.json`.
  - Those results are scratch-only park-reopen proposals.
- Evidence used, in authority order:
  1. January target COFF bytes, relocations and frames.
  2. Ghidra `scratch/ghidra/out/players.obj.decomp.c`, as hypotheses only.
  3. The January map atlas `scratch/atlas/players.obj.txt`.
  4. HCEA `game/*`, `units/unit_should_autopick_weapon.c` and `game/players_reconnect_to_structure_bsp.c`, for semantics and topology only.
  5. Owner prototypes in `objects.h`, `scenario.h` and `bipeds.h`.
- Prior ledgers read in full:
  - `players_obj.md`
  - `players_obj_fable_exact_packet_reconciliation_20260913.md`
  - `players_obj_january_safe_harvest_20260902.md`
  - `players_obj_jonas_residual_freshness_audit_20260830.md`
  - `players_obj_jonas_source_credibility_cleanup_20260830.md`
  - `players_obj_opus5_house_clean_20260913.md`
  - `players_obj_opus5_100k_20260914.md`
  - `docs/players_matching_notes.md`
  - the 100K consolidated handoff
- `branch_sweep`: 77 blobs, and none is better than current. The best historical blob reaches 56 exact.
- Worker notes: `scratch/workers/players.md`. Candidates and objects: `scratch/workers/players/`.

## 2. Gates

| Gate (real file, `--forbid-emitted-symbol _point_from_line3d`) | Result |
|---|---|
| Baseline | `== exact 57  residual 12  unwritten 1  (of 70 listed)`, guard passed |
| Final (real file unchanged) | identical listing, row by row |
| Park drift (real file) | 4 parks OK, 0 drift |
| Owner census vs `build/base` | identical code and data owners |
| `tools/fake_match_scan.py players.c` | 0 leads |

Newly strict exact in the real file: **none**. Every exact row below was reached only in scratch candidates for parked functions.

## 3. Park-reopen proposals (scratch candidates = current real file plus only the reopened bodies)

| Park | Padded / meaningful | Target nSHA16 | Candidate | Lever |
|---|---:|---|---|---|
| `_unit_should_autopick_weapon` | 144 / 134 | `1c1da7cda8b1b98d` | `scratch/workers/players/park_autopick_candidate.c` | single-exit law |
| `_players_compute_combined_pvs` | 368 / 354 | `9e3ab79a5087c8d1` | `scratch/workers/players/park_pvs_candidate.c` | owner local type (`short`), two casts removed |
| `_player_teleport_on_bsp_switch` + `_players_reconnect_to_structure_bsp` (coupled) | 320 / 320 + 736 / 729 | `73f7fae1e4082fd7` / `de7f7f7138236ad8` | `scratch/workers/players/park_bsp_candidate.c` | boolean if/else store, long ?: cluster phi, natural block scoping |
| All four together | 1,568 / 1,537 | as above | `scratch/workers/players/park_all4_candidate.c` | gate `exact 61 residual 8 unwritten 1`; only the four park rows change |

Checks on every candidate:
- emitted-symbol guard passed;
- no baseline-EXACT row lost (full `--all` diff);
- parks not being reopened show no drift, and each reopened park's drift equals its target hash;
- code and data owner census identical to `build/base`;
- fake scan 0 leads.

### 3.1 `unit_should_autopick_weapon`

- **Park premise.** The park called this a "register-allocation" residual. The park body had two `return` statements: `return FALSE;` inside the condition and `return TRUE;` at the bottom.
- **Change.** A single `result` returned once reproduces January's register weights:
  - `weapon_count` goes to EBX with 32-bit test and compare;
  - the definition pointer goes to `[ebp-4]`;
  - the `movsx` happens after the two pushes.
- **Shapes tried.**
  - `result = FALSE; if (positive) result = TRUE;` gets the registers right but leaves the final block polarity wrong.
  - `result = TRUE; if (negated) result = FALSE;` is exact.
  - `if (positive) result = TRUE; else result = FALSE;` is exact.
  - The last two are byte-identical. The proposal uses the positive if/else. Its call order and short-circuit order are unchanged, and it matches HCEA's description of the conditions.
- **Criterion.** "authenticated local-lifetime evidence, not register, volatile, barrier, or inline controls". The premise is refuted: the difference was the return structure, and no control of that kind was used.

### 3.2 `players_compute_combined_pvs`

- **Evidence.** January stores the `objects_get_activating_cluster_index` result with `mov dword [ebp-4],eax`, with no sign extension. It later does `mov eax,[ebp-4]; cmp ax,0xffff; push eax`. That is a `short` local.
- **Change.** The park body declared the local `long` and cast `(short)` at both uses, which placed `movsx eax,ax` before the home store. Declaring the owner prototype's type (`short objects_get_activating_cluster_index(void)`, `objects.h:372`) and removing both casts is exact on the first probe.
- **Refuted theory.** This also refutes the P02 "return-ABI bug / consumer-local long prototype" theory in `players_obj.md`.
- **Criterion.** "authentic statement/lifetime form, never with barriers, aliases, or dead code" is met.

### 3.3 `player_teleport_on_bsp_switch` and `players_reconnect_to_structure_bsp`

- **`teleport_on_bsp_switch`.**
  - January passes `player_index` in EBX (a private register ABI; reconnect pushes only two stack arguments).
  - It homes the outside-trigger boolean at `[ebp-1]` using VC7's if/else store merge: `test al,al; mov byte [ebp-1],1; je; mov byte [ebp-1],0`.
  - Writing `flag = !call()` / `else flag = FALSE` kept the flag in BL (neg/sbb/inc), spent EBX, and forced the stack ABI.
  - The fix is `if (pending != NONE && !scenario_trigger_volume_test_object(TAG_BLOCK_GET_ELEMENT(...)->trigger_volume_index, unit_index)) outside = TRUE; else outside = FALSE;`. The now-unused `bsp_switch` local is removed. The function becomes exact.
  - The fix also corrects reconnect's local-player loop registers.
- **`reconnect_to_structure_bsp`.**
  - January emits `movsx eax,word [leaf+8]` before `add esp,0xc`, then `cmp eax,-1`. The preceding `leaf_index_from_point == -1` test jumps straight to the loop continue. This is a long ?: phi: `cluster_index = scenario_leaf_index_from_point(&biped_base) == NONE ? NONE : TAG_BLOCK_GET_ELEMENT(...)->cluster_index;`.
  - The 0x38 frame has one slot for both the cutscene adjustment and the pill width, yet the x87 add loads the width first:
    - separate function-scope variables give the right x87 order but a 0x3c frame (measured);
    - block-scoped `real adjustment` (cutscene block) and `real_point3d biped_base; real biped_height; real biped_width;` (trigger block) give both.
  - Byte-neutral cleanups, verified exact:
    - removed the function-pointer alias `local_player_index_getter` (direct call);
    - replaced the goto loop with `while (adjustment < 0.3f && collision_test_point(...))`;
    - `teleport_position.z += biped_width`;
    - braces, and a block-scoped `long cluster_index`.
- **Criteria.**
  - teleport: "first-party local-lifetime evidence or a naturally explanatory caller shape".
  - reconnect: "provenance for the temporary lifetime/order, not a codegen control".
  - Both are met: the January frame, ABI and store bytes prove the boolean shape, the phi and the block-scoped temporaries. One existing codegen alias is removed.

## 4. Non-parked residuals (not landed)

| Function | Classification | New evidence / shapes | Reopen criterion |
|---|---|---|---|
| `_player_examine_nearby_device` 176 (ours 160) | x87 argument materialisation needs an rvalue radius | January stages the radius through the x87 at 4 call sites in 4 TUs (players, bipeds `_biped_update_moving`, collisions `_object_test_vector`, ai `_ai_test_line_of_fire`), but not for collision_bsp `data->radius`. A scan of all accepted-exact functions finds the form only with double locals, a genuine macro (DSOUND_POINT3D) or D3D calls. A paren-wrapped radius is exact (diagnostic only, prohibited). A `const` device pointer is byte-identical. | authenticated January macro or accessor yielding an rvalue |
| `_player_set_action_result` 240 | x87 stack scheduling inside the first `distance3d` (k,i,j vs i,k,j) | none new (tie) | owner real_math.h distance/magnitude spelling |
| `_player_examine_nearby_vehicle` 448 / `_player_examine_nearby_objects` 240 | third vehicle fetch (rejected twice) plus the constant-folded flip call inlined | nested-if flip branch is byte-identical | first-party re-fetch spelling and an out-of-line flip reason |
| `_player_handle_powerup` 240 / `_minor` 160 / `_update_powerups` 112 | January keeps sext(powerup_type) live (in-place `movsx esi,si`, 32-bit inlined helper tests); ours CSEs the element address | See the list below this table. | caller-side carrier consistent with the standalone helpers; start from p1/m1 |
| `_player_teleport_internal` 1296 (ours 1312/62) | entry-region register weights (source_unit_index in ESI instead of its parameter home), frame 0x78 vs 0x7c | See the list below this table. | evidence lowering source_unit_index's entry register weight |
| `_player_examine_nearby_unit` 16 | owner-blocked target-only `ret` stub | none | first-party stub source plus owner policy |

Powerup family shapes:
- **p1** (single-exit handle_powerup) fixes the first-half registers.
- **v1–v3** (one-case switch in the helpers) reproduce the caller shape but break the standalone `player_powerup_on` constant store.
- **m1** (block `long` carrier with the ordinary helper call) differs from January by one `test si,si`.
- **u1–u4** (loop spellings) give 96 [size].

`player_teleport_internal` slot map (January):
- `-0xc`: root pointer, later the adjustment index;
- `-0x10`: biped, later the scenario;
- `-0x18`: scale, later a separate trigger index;
- loops use the `for` + `if (result) break;` shape.

`teleport_internal` shapes:
- **t0** (previous lane's v2) and **t1** (plus the loop shape and block scoping, `tele_t1_fn.txt`) both measure 1280/61, frame 0x78.

## 5. Rejected shapes (do not repeat)

- **device.** Paren radius (prohibited). `const` device pointer (inert).
- **vehicle.** Nested-if flip branch (inert).
- **powerup.** One-case switch in any helper (breaks the standalone `powerup_on`). Short or long indexed `for` loops (96). Short index in the current loop (inert). Carrier passing the original short to the helper (176).
- **teleport_on_bsp_switch.**
  - b2 (`flag=FALSE; if(!call) flag=TRUE`) fixes the ABI, but the store order is wrong.
  - b3 (`succeeded=TRUE` first) measures 304/20.
  - b5 (nested if/else) and b7 (init plus `!call`) keep neg/sbb.
- **reconnect.**
  - Separate function-scope adjustment and width variables give frame 0x3c.
  - `do { if (!hit) break; ... } while` adds one jmp.
  - `while (hit) { ...; if (!(adj<0.3f)) break; }` measures 752/41.
  - `leaf`-pointer removal alone and add-operand respellings are inert.
- **teleport_internal.** t1 scoping and loop shape (frame 0x78 unchanged).

## 6. Proposals for the orchestrator

- **Park reopens.** Four park-reopen proposals (§3); the combined candidate is `scratch/workers/players/park_all4_candidate.c`.
- **Unpark commit.** In one dedicated commit:
  - apply the four bodies;
  - remove the four `source/game/players` entries from `config/parked.json`;
  - rebuild the full board and diff it per function.
- **Headers and config.** No header or config text is otherwise required.
