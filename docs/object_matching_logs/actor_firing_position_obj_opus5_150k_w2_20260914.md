# `actor_firing_position.obj` - opus5 150K house-clean lane, wave w2 park re-investigation (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_firing_position.c`, target `build/split/source/ai/actor_firing_position.obj`
  (January 2002 Xbox debug, VC7 13.00.9254).
- Lane `opus/150k-house-clean-20260914`, worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
- **The real source file was not modified** (`git diff --stat` empty). All four residuals are PARKED/LOCKED; all work is scratch-only
  under `scratch/workers/actor_firing_position/`. No header, config, tool or other source file was edited; no ninja/configure/pytest/git write.
- Ledgers read: `actor_firing_position_obj_jonas_prefix_20260821.md`, `actor_firing_position_obj_opus5_100k_20260914.md`,
  `actor_firing_position_obj_opus5_100k_waveD_20260914.md`, `opus5_100k_consolidated_HANDOFF_20260914.md` (rejections: "unused lookup
  locals" for post_evaluator_global / pre_evaluator_combatmove), `config/parked.json` entries.
- Duplicate checks: `git log --all` refs afc739fc3, ff71ecfc2, c5d7751bd, 1cc269896, 5d3444f79, 25c45ad20, 89b1d0664, f9dd42218,
  41a807650, c8a531ba2; `branch_sweep.py`: 10 blobs, none better than the current 23/4.
- Evidence: January COFF bytes/relocations (authority); HCEA `src/blam/ai/pre_evaluator_attack.c`, `pre_evaluator_global.c` (release
  build; semantics only); accepted production precedents named below.

## 2. Baseline and result

| | exact | residual | unwritten |
|---|---:|---:|---:|
| real file baseline (guard passed) | 23 | 4 | 0 |
| final real file (unchanged, guard passed, park drift 0/4, census clean) | 23 | 4 | 0 |
| scratch park-reopen candidate `s2.c` | 26 | 1 | 0 |

| Parked function | Target size/relocs/sha16 | Park base | Scratch result |
|---|---|---|---|
| `_pre_evaluator_global` | 1616 / 53 / `d83fd2341499b721` | 1616/53 | **EXACT** (g6) |
| `_post_evaluator_global` | 224 / 8 / `e07f193d22593c7a` | 208/7 | **EXACT** (s1) |
| `_pre_evaluator_combatmove` | 576 / 21 / `dec21af4f9dee891` | 560/19 | **EXACT** (s2) |
| `_pre_evaluator_attack` | 1120 / 39 / `7b38541b26618530` | 1120/38 | 1120/39 residual, owner-blocked |

Candidate `scratch/workers/actor_firing_position/s2.c` (sha256 `ff46dcac3f5ab61eaea6ce9584d13c59b8ab26403c63dadbfd6ed2fd76abbbe4`) =
current real file + only the three reopened bodies. Listing diff vs baseline changes only those three rows; `_point_from_line3d` guard
passes; `scratch/parkcheck.py`: the three reopened rows equal their parked `target` records, `_pre_evaluator_attack` equals its `base`;
owner census vs `build/base`: no candidate-only or base-only owners, 0 literal deltas; `tools/fake_match_scan.py`: 0 leads (HEAD 0).
Each reopen was also measured alone on the real file (`g6.c`, `s2_pg.c`, `s2_cm.c`): 24/3 each, only its own row changes.

## 3. Proposed controls (park-reopen)

### `_pre_evaluator_global` (g6)
January @0x244-0x269 calls `point_to_line_distance_squared3d`, then reads `danger_radius` and squares it on the x87 under the forwarded
call result (`fld [edi+0x294]; fld st(0); fmul st(1); ... fcompp`), unlike the `&&`-chained `vector_to_line` test @0x2b5 whose radius is
memory-CSE'd before the call. Recovered source: the danger-corridor test is nested and binds the body point-to-line distance to a local
in its own block: `if (sphere_test && current_distance > danger_radius) { real distance_squared= point_to_line_distance_squared3d(...);
if (distance_squared > danger_radius*danger_radius) {...} }`. Kept g5 features (each ablated -> residual): late `evaluation= 10.0f` /
`15.0f` stores (STORE ORDER after the loop / reject test), `distance_squared3d(&center, &point)` argument order (subtraction direction),
`!(fabs(d) < _real_epsilon)` (NaN polarity `test ah,0x41`; same spelling path_smoothing.c:186), `(r + 2.5f) * (r + 2.5f)` (radius sum
loaded after the subtractions; precedent action_obey.c:2566), compare-and-assign minimum, direct `avoid_points[avoid_index]` access.

### `_post_evaluator_global` (s1)
January @0x10-0x20: `datum_get` -> EDI, then `tag_get('actr', [edi+0x58])` whose EAX is never read. The 100K lane's unused
`definition` local was rejected. **New evidence:** the accepted, strict-exact production functions `carrier_decide_action`
(`actor_type_carrier.c:60`), `flood_decide_action` (`actor_type_flood.c:91`) and `infection_decide_action`
(`actor_type_infection.c:191`) spell this exact January call pair as `struct actor_datum *actor = actor_get(actor_index);` followed by the
statement `actor_definition_get(actor->meta.definition_index);` - a debug-build tag validation, no local. With that statement plus the
inherited h3 tail (NULL arm only accumulates the bound; `if (!firing_position) return TRUE; return firing_position->valid;`, identical to
the accepted uncover/hide/attack evaluators in this TU) the function is strict EXACT.

### `_pre_evaluator_combatmove` (s2)
January @0x7-0x19: `datum_get(actor_data, actor_index)` whose EAX is overwritten by `actor_combat_get_firing_variant_definition` unread;
`mov [ebp-4],8.0f` @0x32 after both calls. Sibling `pre_evaluator_attack` has the byte-identical two-call prologue and uses the actor.
**New evidence:** accepted strict-exact `actor_pursuit_consider_nearby_actor` (`actions.c:1815`) uses the statement
`actor_get(actor_index);` (documented datum validation); `action_converse.c:70/159` likewise. Shape: `actor_get(actor_index);
variant_definition= actor_combat_get_firing_variant_definition(actor_index); range_weight= 8.0f;` - strict EXACT.

## 4. Experiment matrix (this wave)

| ID | Function | Shape | Size T/C | Relocs T/C | Result |
|---|---|---|---|---|---|
| g5 | pre global | 100K best | 1616/1616 | 53/53 | one compare block |
| g6 | pre global | g5 + nested if + `distance_squared` result local | 1616/1616 | 53/53 | **EXACT** |
| gab_A..E | pre global | g6 with one g5 feature reverted each | 1616 | 53 | all residual (all features load-bearing) |
| s1 | post global | h3 + statement `actor_definition_get(...)` | 224/224 | 8/8 | **EXACT** |
| s1 | combatmove | statement `actor_get(actor_index)`, `range_weight= 8.0f` at declaration | 576/576 | 21/21 | 8.0 store before datum_get |
| s2 | combatmove | s1 + `range_weight= 8.0f` after the lookups | 576/576 | 21/21 | **EXACT** |
| s1 | pre attack | d3 + `20.0f * (distance_from_boundary / 2.0f)` | 1120/1120 | 39/39 | constant block matches; projection block + frame 0x40 |
| s2 | pre attack | d3 + in-place `add_vectors3d(&direction, &projection, &projection)` | 1120/1120 | 39/39 | projection still stored (frame 0x40) |
| s3 | pre attack | `20.0f * distance_from_boundary / 2.0f` | 1120/1104 | 39/38 | folds, rejected |
| d4 re-measure | pre attack | cast + `point_from_line3d` (diagnostic, guard FAIL) | 1120/1120 | 39/39 | frame 0x34, still one inner-add operand order ((jj+kk)+ii vs (kk+jj)+ii) |

## 5. Do-not-repeat

- pre attack: scale+add in any destination (direction, fresh local, in place) keeps the projection stored; unparenthesised
  `d/2.0f*20.0f`, `20.0f*d/2.0f` fold or drop a relocation; point_from_line3d with casts is not exact either.
- combatmove: `range_weight` initialised at its declaration places the 8.0 store before the lookups.

## 6. Residual classification and blockers

- `_pre_evaluator_attack`: OWNER-BLOCKED. January frame `sub esp,0x34` (13 dword slots) never stores the projection; @0x376-0x396
  `fchs` once then per component `fld t; fmul v.c; fadd direction.c` in i,j,k order = the `point_from_line3d` body (`v->c*t + p->c`)
  applied to the stored direction (HCEA release shows the same per-component `vector*neg_along + (pos-point)`). No cast-free,
  non-point_from_line3d spelling reproduces it; even the forbidden form leaves one x87 operand-order difference. Best cast-free body
  `scratch/workers/actor_firing_position/bodies/attack_s1.c`.

## 7. Reopen criteria

- Three parks: orchestrator verifies `scratch/workers/actor_firing_position/s2.c` (full build, unpark `_pre_evaluator_global`,
  `_post_evaluator_global`, `_pre_evaluator_combatmove` in a dedicated commit).
- `_pre_evaluator_attack`: owner ruling on point_from_line3d emission/representation for this TU, then the magnitude operand order; or a
  January helper with body `v->c*t + p->c` over vectors.

## 8. Disposition

Real file unchanged (23/27 strict exact). Three parks proposed for reopen with strict-exact scratch evidence (2,416 padded bytes);
one park remains locked with a precise owner blocker. Newly exact in the real file: none.
