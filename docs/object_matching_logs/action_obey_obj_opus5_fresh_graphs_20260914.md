# ActionObey fresh-graphs lane (opus5, 2026-09-14): header-blocked private graph, measured in scratch

Worker lane `opus/30k-fresh-graphs-20260914` (base `c3e257e9a`, lane HEAD `47d9dd0da`).
**No production source changed.** `git diff -- source/ai/action_obey.c` is empty; the
final real-file gate equals the baseline. All progress below is scratch-only evidence
for an orchestrator header packet.

## 1. Scope and provenance

- Unit `source/ai/action_obey.c` (SHA-256 `bfc668a6...5fe9`) vs January split
  `build/split/source/ai/action_obey.obj` (SHA-256 `d52f369d...600e`).
- Compiler VC7 13.00.9254 (XDK), flags `/c /O2 /Oy- /DDEBUG /Dxbox`.
- Names: January map atlas `scratch/atlas/action_obey.obj.txt` (all seven non-exact rows
  already carry their authentic names after orchestrator commit `47d9dd0da`).
- Semantics: HCEA `research-cache/halocea-full-blobs-20260830/src/blam/ai/action_obey_command_{begin,perform}.c`,
  `action_obey_individual_perform.c`, `action_obey_control.c`; Ghidra
  `scratch/ghidra/out/action_obey.obj.decomp.c`; canonical uncompiled proposals
  `scratch/action_obey_command_{begin,perform}_proposal_20260905.c`; Fable donor
  `C:/halo-worktrees/fable-50k-small-families-20260904` (`f9dd42218`). No HCEA/Stian body
  exists for `action_obey_describe_command` (PC debug only).

## 2. Validated baseline (lane HEAD)

`== exact 20  residual 1  unwritten 6  (of 27 listed)`, `_point_from_line3d` guard passed,
no parked entries for this unit.

| Row | Target | Status |
| --- | ---: | --- |
| `_action_obey_command_begin` | 4320 / 167 relocs / `5b56fdd8b88a4456` | UNWRITTEN |
| `_action_obey_command_perform` | 1408 / 51 / `afc109dc8ddf7e3f` | UNWRITTEN |
| `_action_obey_control` | 992 | residual [sha] |
| `_action_obey_describe_command` | 1936 / 153 / `d14c27f63086d980` | UNWRITTEN |
| `_action_obey_individual_perform` | 288 / 11 | UNWRITTEN |
| `_action_obey_perform` | 192 / 8 | UNWRITTEN |
| `_vehicle_possibility_qsort` | 64 / 0 | UNWRITTEN (no caller emits it) |

## 3. Results (scratch whole-TU gate, prerequisites declared scratch-only)

`scratch/workers/action_obey/b10.c`: `== exact 24  residual 3  unwritten 0`, all 20 baseline
exact rows still exact, point guard passed, `tools/fake_match_scan.py` 0 leads.

| Row | Scratch result | Candidate text |
| --- | --- | --- |
| `_action_obey_perform` | **strict EXACT** | `scratch/workers/action_obey/perform.inc` |
| `_action_obey_individual_perform` | **strict EXACT** | `BEST_individual_perform.inc` |
| `_action_obey_command_perform` | **strict EXACT** (bytediff 0) | `BEST_command_perform.inc` |
| `_vehicle_possibility_qsort` | **strict EXACT** (via genuine caller) | `qsort_new.inc` |
| `_action_obey_command_begin` | 4336 / 167, frame-packing residual | `BEST_command_begin.inc` |
| `_action_obey_describe_command` | 1824 / 147, cross-jump residual | `describe_v1.inc` (+ dialogue include) |
| `_action_obey_control` | unchanged (register/aggregate-copy tie) | — |

Post-header landing file with no scratch shadows: `scratch/workers/action_obey/LANDING_after_headers.c`.

## 4. Why nothing landed (owner block)

Every new body needs declarations that belong to shared headers (brief §1 forbids worker
header edits and TU-local shadows):

- `describe_command`, `command_begin`: `struct ai_{animation,script,recording}_reference_definition`
  (January element sizes 0x3c/0x28/0x28; PDB records).
- `command_begin`/`command_perform`: go_to, look, animation_mode, crouch, grenade, vehicle,
  animate, action, targeting, initiative, wait, die modifier domains; actor fire-target and
  combat-status domains; unit animation impulses.
- `individual_perform` requires both private bodies; `perform` requires `individual_perform`;
  the qsort comparator is emitted only through `command_begin` (static COMDAT law).
- Owner census: the graph also emits SELECT_ANY copies of `distance_squared3d`, `dot_product2d`,
  `dot_product3d`, `vector_from_points3d`, `real_random_range`, which January references as
  undefined externals (per-caller real_math inline boundary; base already emits the
  magnitude/normalize/scale/square_root family).

Exact ORCHESTRATOR-PROPOSAL text, conflict lists and consumer lists:
`scratch/workers/action_obey.md` (end of file) and `scratch/workers/action_obey/prereq.inc`.

## 5. Accepted source controls (evidence per shape)

- **command_perform**: single `done` result returned once (every January epilogue is `mov al,bl`);
  each guard path assigns its result explicitly (`else { done = TRUE; }`) — removing any one of
  the eight explicit TRUE results changes bytes (measured individually); the TRUE-atom case group is
  the last case (its `mov bl,1` block falls into the return block); `done = dot_product3d(...) >
  current_command->parameter1` for move_direction; flying/2D facing arms share one 0.984 compare via
  `goto facing_done` (HCEA CFG); assert line 983 names `current_command`; action result
  `!complex_control || !complex_control->play_action`.
- **individual_perform**: `command_valid = FALSE; if (index < count) command_valid = TRUE;`
  (January's branchy stores; `= index < count` emits setl and swaps two byte slots).
- **vehicle_possibility_qsort**: `<` → -1, `>` → 1, else 0 with three returns (January reloads `a`
  between compares); HEAD's `a >= b ? a > b : -1` body is the wrong shape.
- **describe_command**: function-scope 5-entry look-modifier table, block-local tables per case
  (packed into the same top slots), 11-entry action table, 256-byte teleport buffer (frame 0x140
  exact); `dialogue_get_vocalization_name` must be prototyped (`units/dialogue_definitions.h`)
  — without it C4013 promotes the short argument with movsx.
- **command_begin**: see §6 v10; ABI EAX=complex_control / ECX=unit_index reproduced.

## 6. Experiment matrix (bounded)

describe_command: v1 all-break 1824/147; v2 all-return identical; v3/v4 mixed return/break
1872/149 and 1824/147; v1b + dialogue include 1824/147 (vocalize fixed). Residual = VC7
cross-jump selection only.

control: single overwritten local (Ghidra form) byte-identical to HEAD; alignment store before
targeted / first → worse; jump-block scope local → identical. Residual = reversed field order of a
register-pair struct copy.

command_perform: v1 1456/52 → v2 goto facing 1424 → v3 HCEA returns 1408/51 (wrong ABI:
complex_control register-passed) → v12 mixed 1408 regalloc right → v14 explicit guard results →
v18 TRUE group last (1 byte) → v19b/v20 EXACT. Uninitialised-`done` probe rejected (UB, and wrong).

individual_perform: donor body 272; A (FALSE/if TRUE) EXACT; ternary next index 272; C 288 sha.

command_begin: v1 4368/179 (switch-compiled sub-dispatch) → v2 if/else chains 4320/167 →
v3 single exit → v5 `short command_number` → v6 two distance locals → v7 double duration store
(refs match January) → v10 scenario local / surface arg / vocalize order / biped ternary 4336/167.
Probes inert or worse: short index local, declaration order, real_point3d facing, direction and
iterator scope moves.

## 7. Do-not-repeat

- describe_command return/break mixes; control statement moves and local scoping; command_begin
  declaration-order permutations (density ranking ignores order off ties).
- Never use an uninitialised result, TU-local enum/struct shadows, or non-static externs for the
  private pair.

## 8. Residual classification

- control: register/store-order tie (measured).
- describe_command: cross-jump granularity (measured instruction-level; source cause unknown).
- command_begin: frame-allocation packing — January classes {direction, teleport facing,
  head_position, vehicle_origin, prop_iterator, actor_definition} (end-aligned at -0x14) and
  {object_iterator, data_iterator} at -0x40; ours forms {object_iterator, facing, prop_iterator,
  actor_definition}, {direction, head, origin} and a lone data_iterator (+16 bytes). Inference, not proof.

## 9. Reopen criteria

- Header packet A–C (and census decision D) landed → gate `LANDING_after_headers.c`; land the four
  exact rows, park or keep begin/describe fuzzy.
- command_begin: an evidence-led function-scope promotion that reproduces January's packing classes.
- describe_command: a source-level reason for January's sparser cross-jumping.

## 10. Disposition

Still active; zero new production credit from this lane. Four rows are strict EXACT in a complete
scratch TU pending owner-header prerequisites.

## Lead admission (2026-09-14): graph landed

The worker's scratch-only prerequisites were resolved as follows, and the
complete private graph was then admitted:

- **Header prerequisite `ef7bc742a`**: `source/ai/ai_scenario_definitions.h` now
  owns the command-list atom modifier enums and the ai animation/script/recording
  reference definitions (HCEX PDB names; January element sizes 0x3C/0x28/0x28).
  `action_alert.c`'s local reference struct was removed. The full build showed
  no inherited strict loss; two already-parked fuzzy functions were re-measured
  (documented in the park evidence).
- **TU-local enums with disclosure**: the actor fire-target and combat-status
  levels cannot be added to `actors.h` in this lane because the active
  `actor_perception.c` already defines the combat-status enumerators locally (a
  redefinition would break it). They follow canonical's existing TU-local practice
  (copies in actors.c, actor_combat.c, actor_perception.c, ai_script.c). The unit
  animation impulse enum is TU-local because units.obj is outside this lane
  (units.c declares only the count locally).
- **Adversarial review** accepted `_action_obey_command_perform` (1,408),
  `_action_obey_individual_perform` (288), `_action_obey_perform` (192) and
  `_vehicle_possibility_qsort` (64) as strict exact, and
  `_action_obey_describe_command` (1824/147 vs 1936/153) as fuzzy. It rejected
  the first `_action_obey_command_begin` spelling for byte-inert house-rule
  defects. The lead removed the duplicated `look_duration` store, collapsed the
  extra look_player braces, re-indented the command block (whitespace only) and
  rewrote the enum provenance comments accurately. The re-gate is unchanged:
  24 exact / 3 residual / 0 unwritten, fake scan 0 leads.
- **Disclosed owner census**: header-inline SELECT_ANY COMDAT copies
  `_distance_squared3d`, `_dot_product3d`, `_real_random_range`,
  `_vector_from_points3d` (January references them as undefined externals) and
  an unreferenced `_dot_product2d` copy. This is the same systemic class as the
  base object's pre-existing magnitude/normalize/scale/square_root copies, and
  no new non-inline code owner was added.
- `_action_obey_command_begin` (4336/167 vs 4320/167) and
  `_action_obey_describe_command` land as zero-credit fuzzy genuine callers.
  `_action_obey_control` is unchanged.
