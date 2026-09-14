# `actor_firing_position.obj` - opus5 100K consolidated lane, wave D park re-investigation (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_firing_position.c`, target `build/split/source/ai/actor_firing_position.obj`
  (January 2002 Xbox debug `cachebeta.exe`, VC7 13.00.9254, `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/100k-consolidated-20260914`, worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`, HEAD `548451f6d`.
- **The real source file was not modified.** All seven residuals of this object are PARKED/LOCKED; all work below is
  scratch-only (`scratch/workersD/actor_firing_position/`). No header, config, tool or other source file was edited; no
  ninja/configure/pytest/git write was run.
- Evidence: January COFF bytes/relocations (authority); HCEA `src/blam/ai/{post_evaluator_uncover,post_evaluator_hide,
  post_evaluator_attack,post_evaluator_global,pre_evaluator_attack,pre_evaluator_global}.c` (later release PPC build, semantics
  only); `config/parked.json` entries; prior ledgers `actor_firing_position_obj_jonas_prefix_20260821.md`,
  `hs_actor_owner_reconciliation_20260907.md` (park origin), `claude_remaining_donor_batch_20260908.md` (combatmove donor verdict),
  `actor_firing_position_obj_opus5_100k_20260914.md` (wave A), `actor_combat_obj_opus5_100k_20260914.md` (unused-local precedent).
- Duplicate checks: `git log --all` refs ff71ecfc2, c5d7751bd, 1cc269896, 5d3444f79, 25c45ad20, 89b1d0664, f9dd42218, 41a807650,
  c8a531ba2; `branch_sweep.py`: 9 blobs, none better (no historical blob has any of the seven evaluators exact).

## 2. Validated baseline and result

| | exact | residual | unwritten |
|---|---:|---:|---:|
| lane HEAD baseline (real file) | 20 | 7 | 0 |
| final real file (unchanged) | 20 | 7 | 0 |
| scratch park-reopen candidate `m1.c` | 25 | 2 | 0 |

| Parked function | Target size/relocs/sha16 | Park base | Scratch result |
|---|---|---|---|
| `_post_evaluator_uncover` | 192 / 4 / `45c381842b37aba1` | 192/4 sha | EXACT (h3) |
| `_post_evaluator_hide` | 224 / 11 / `66434779de62f535` | 224/11 sha | EXACT (h3) |
| `_post_evaluator_attack` | 224 / 3 / `46496861931cc356` | 208/3 | EXACT (h3) |
| `_post_evaluator_global` | 224 / 8 / `e07f193d22593c7a` | 208/7 | EXACT (h3 + prologue definition lookup) |
| `_pre_evaluator_combatmove` | 576 / 21 / `dec21af4f9dee891` | 560/19 | EXACT (prologue actor lookup) |
| `_pre_evaluator_attack` | 1120 / 39 / `7b38541b26618530` | 1120/38 | 1120/39, one projection block differs |
| `_pre_evaluator_global` | 1616 / 53 / `d83fd2341499b721` | 1616/53 | 1616/53, one comparison differs |

Checks on `m1.c`: listing diff vs the real file changes only the five rows; `_point_from_line3d` guard passes; the five
reopened functions measure exactly their parked `target` records and the other two parks still equal their parked `base`
records; code and data owner sets equal `build/base`; `tools/fake_match_scan.py`: 0 leads. Newly exact in the real file: none.

## 3. Proposed controls (park-reopen, not in production)

- **Post evaluators uncover/hide/attack/global (h3):** the `has_target && firing_position==NULL` arm only accumulates the
  post-evaluation bound and falls through; the function ends with `if (!firing_position) { return TRUE; } return
  firing_position->valid;`. Binary: the NULL arm has its own epilogue with `mov eax,1` loaded *before* the `fadd` and the `fstp`
  interleaved after `pop esi` (a jump-threaded duplicate of the bottom `return TRUE`, not a cross-jumped early return), and the
  bottom is `test esi,esi; jne valid; mov eax,1; pop; pop; ret`.
- **`post_evaluator_global` prologue:** `struct actor_datum *actor= actor_get(actor_index); struct actor_definition *definition=
  actor_definition_get(actor->meta.definition_index);` - January `datum_get` then `tag_get('actr', [actor+0x58])` with the result
  never read. The identical prologue is used (definition consumed) by the exact same-TU `actor_nearby_firing_positions` and
  `actor_select_firing_position`.
- **`pre_evaluator_combatmove` prologue:** `struct actor_datum *actor= actor_get(actor_index);` before
  `actor_combat_get_firing_variant_definition(actor_index)`. January's sibling `pre_evaluator_attack` (same table, same signature)
  has the byte-identical two-call prologue (@0007/@0012/@001b) and uses `actor->emotions.berserk`: the combat-move evaluator's
  prologue is a copy of the attack evaluator's, so the unused lookup is a leftover, not bait.
- **New evidence against the parks' premise ("HCEA omits it"):** HCEA is a release build whose `actor_get`/`tag_get` are the inline
  `DATA_ARRAY_ELEMENT`/`TAG_GET` accessors (HCEA `post_evaluator_global.c` documents the inline clrlwi16+mulli actor accessor;
  `pre_evaluator_attack.c` line 33). An unused pure inline accessor is dead-code-eliminated there, so HCEA cannot witness it. In the
  January debug build the accessor is an out-of-line call and survives. Lane precedent: `actor_combat.c` (ffc0dec8f) retains an
  unused `trigger` accessor local for the same reason. This is an owner/verifier decision; the h3 subset (`p_h3.c`) is independent
  of it.

## 4. Experiment matrix

| ID | Function | Shape | Size T/C | Relocs T/C | Result |
|---|---|---|---|---|---|
| v1 | 4 post evaluators | HCEA `return fp ? fp->valid : TRUE` | = base | = base | byte-identical to base |
| a | uncover | single-exit `long result= TRUE` | 192/192 | 4/4 | result held in edi, no |
| b | uncover | result var + early return | 192/192 | 4/4 | edi CSE, no |
| c/f | uncover | early return kept + `if (!fp) return TRUE; return valid;` | 192/192 | 4/4 | early block ok, tail if-converted |
| h1 | uncover | no early return + `?:` | 192/192 | 4/4 | no |
| h2 | uncover | no early return + base tail | 192/192 | 4/4 | no |
| h3 | uncover, hide, attack | no early return + NULL-guard tail | = | = | **EXACT x3** |
| g1 | post global | h3 + definition prologue | 224/224 | 8/8 | **EXACT** |
| m1 | combatmove | + actor prologue | 576/576 | 21/21 | **EXACT** |
| a1/a1b | pre attack | scale+add into a fresh local | 1120/1120 | 39/38 | identical to base |
| c1 | pre attack | `d/2.0f*20.0f` | | 38 | folds to 10.0, no |
| c4 | pre attack | PAREN LAW `(distance_from_boundary*0.5f)*20.0f` | | 39/39 | reloc closed |
| d1/d2 | pre attack | late `evaluation= 0.0f;`/`6.0f;` statements | | 39/39 | store positions match |
| d3 | pre attack | explicit `if (distance_squared<nearest)` | | 39/39 | compare matches; projection block + frame 0x40 vs 0x34 left |
| d4 | pre attack | `point_from_line3d((real_point3d *)&direction, ...)` diagnostic | | | reproduces block but representation cast + emits `_point_from_line3d` (guard FAIL) - rejected |
| g1 | pre global | sphere tests `distance_squared3d(&center,&p) < (r+2.5f)*(r+2.5f)` | 1616 | 53 | both blocks match |
| g2 | pre global | `r*r < point_to_line(...)` | 1616 | 53 | slots fixed, compare still differs |
| g3 | pre global | avoid arg order, explicit if-min, late `evaluation=`; `!(fabs(d) < _real_epsilon)` | 1616 | 53 | avoid/driving blocks match |
| g4a/g4b | pre global | array access w/o g2; nested-if split | 1600/1616 | 53 | array access right; nested-if inert |
| g5 | pre global | g3 + direct `avoid_points[avoid_index]` | 1616 | 53 | one compare left |

## 5. Do-not-repeat list

- Post evaluators: `?:` tails, result variables and keeping the early `return TRUE` all cross-jump or if-convert; only h3 matches.
- pre_evaluator_attack: `d/2.0f*20.0f` folds; scale_vector3d+add_vectors3d into direction or into a fresh local are
  byte-identical; the per-component unstored `v*t + p` needs point_from_line3d semantics (casts + forbidden COMDAT).
- pre_evaluator_global: a `bounding_radius` local statement (any position) is loaded before the subtraction; nested-if split of
  the danger-corridor test does not change the danger_radius CSE.

## 6. Residual classification

- `_pre_evaluator_attack` (measured): projection vector computed per component on the x87 and never stored (frame 12 bytes
  smaller). Inference: January used a `v*t + p` per-component form (point_from_line3d-shaped) into an unstored local; no
  admissible cast-free spelling found. Class: source/topology, unresolved. Best scratch body `bodies/attack_d3.c`.
- `_pre_evaluator_global` (measured): one comparison; January reads danger_radius after the point_to_line call and squares it on the
  x87; ours either memory-CSEs it before the call or squares before the call. Class: x87 CSE/scheduling tie. Best scratch body
  `bodies/global_g5.c`.

## 7. Reopen criteria

- Five evaluators: orchestrator accepts the park-reopen packet (`scratch/workersD/actor_firing_position/m1.c`, or the
  cast/lookup-free subset `p_h3.c` for uncover/hide/attack only).
- `_pre_evaluator_attack`: a natural cast-free form (or genuine January helper) producing the unstored per-component `v*t + p`
  vector without emitting `_point_from_line3d`; start from `bodies/attack_d3.c`.
- `_pre_evaluator_global`: a natural form that kills the danger_radius memory CSE across the point_to_line call; start from
  `bodies/global_g5.c`.

## 8. Disposition

Still NonMatching; real file unchanged (20/27 exact). Five parks proposed for reopen with strict-exact scratch evidence; two parks
remain locked with narrowed residuals.
