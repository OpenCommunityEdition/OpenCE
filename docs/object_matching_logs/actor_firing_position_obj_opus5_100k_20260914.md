# `actor_firing_position.obj` - opus5 100K consolidated lane (waveA worker, 2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/actor_firing_position.c`, target `build/split/source/ai/actor_firing_position.obj`
  (January 2002 Xbox debug `cachebeta.exe`, VC7 13.00.9254, flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/100k-consolidated-20260914`, worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`,
  HEAD `b8cd50113`. Only this `.c` was edited. No header, config, tool or other source file was changed; no ninja,
  configure, pytest or git write was run.
- Evidence: January COFF bytes and relocations (authority), Ghidra 12.1.2 decompilation
  `scratch/ghidra/out/actor_firing_position.obj.decomp.c`, January map atlas `scratch/atlas/actor_firing_position.obj.txt`,
  HCEA `src/blam/ai/{firing_position_compare,firing_position_compute_line_of_sight,actor_active_select_firing_position,actor_select_firing_position}.c`
  (later release build; semantics and names only), `source/ai/ai_debug.c` renderer (field meanings of the debug records).
- Prior ledgers read: `actor_firing_position_obj_jonas_prefix_20260821.md`, `hs_actor_owner_reconciliation_20260907.md`,
  `fable_50k_small_families_interrupted_lane_intake_20260905.md`, `opus5_fresh_graphs_HANDOFF_20260914.md`.
  `branch_sweep.py` found no better blob in any lane.

## 2. Validated baseline and result

Gate (`tools/campaign/gate.py --all --forbid-emitted-symbol _point_from_line3d`):

| | exact | residual | unwritten |
|---|---:|---:|---:|
| lane HEAD baseline | 16 | 11 | 0 |
| final real file | 19 | 8 | 0 |

| Function | Target size/relocs | Baseline | Final |
|---|---|---|---|
| `_firing_position_compare` | 272 / 15 | 224 / 15 residual | EXACT sha16 `814f436eb77899af` |
| `_firing_position_compute_line_of_sight` | 464 / 13 | 496 / 13 residual | EXACT sha16 `1e3e9a13eb49f869` |
| `_actor_active_select_firing_position` | 576 / 19 | 560 / 17 residual | EXACT sha16 `d068e3ef512a549d` |
| `_actor_select_firing_position` | 5248 / 120 (sha16 `e5ccfff162a8ee26`) | 4272 / 74 residual | fuzzy 5200 / 117 sha16 `36c1be7cf721d1c9`; EXACT under the ai_debug.h proposal (section 6) |
| 7 parked evaluators | unchanged | locked | park drift 7/7 OK |

Newly exact: 3 functions, 1,312 padded bytes. Data sections were not worked; no data credit is claimed.

## 3. Accepted controls (retained source shapes)

- **firing_position_compare**: valid/rejected arms return `(x ? -1 : 1)>0` / `(x ? 1 : -1)>0`; evaluation arm is
  explicit `if (a>b) return FALSE; if (a<b) return TRUE; return FALSE;`. Binary: `sete/setne; lea r,[r+r-1]; test r,r; setg al`
  in the first two arms (a signed sign value tested against zero, not foldable), byte constants `xor al,al / mov al,1` in the
  third (same idiom as exact `_compare_surface_indices`). HCEA independently records the PPC build computing the same sign
  expression in the first two arms.
- **firing_position_compute_line_of_sight**: the target-facing vector is normalised in place,
  `normalize2d((real_vector2d *)&facing)` then `facing.k= 0.0f`. Binary: three `fstp` into one 12-byte slot, no 2-D copy.
  HCEA writes the same call; the idiom already exists in exact actors.c, actor_moving.c, action_obey.c, action_charge.c.
- **actor_active_select_firing_position**: single exit (`short result= NONE; if (encounter!=NONE) {...} return result;`,
  NONE shared in ECX with the encounter compare); `if (total_groups>currently_allowed_groups)` with
  preferred_groups/preferred_weight stored before the assert call; allow_outside_range as an if/else of byte constants;
  short result (no movsx); re-fetch `actor= actor_get(actor_index)` inside the group-toggle arm (second actor_data/datum_get
  relocation pair, fallback arm keeps the first pointer); fallback re-tests the actor fields and binds the index inside the block.
- **actor_select_firing_position** (fuzzy body): see the worker report for the full binary fact list; key shapes are the debug
  selection flag, the switch statements (friend avoidance compare chain; attack vectors 2-entry jump table), the seeded
  `dangerous` boolean, chained counter reset, direct indexed candidate fill, forward-substituted `actor_distance`, else-if prop
  selection, bound target-prop pointer for the attractor object, `path_input_new(radius, ignorant_of_broken_surfaces, NONE)`
  (previous body had these arguments swapped), `distance_squared3d` linear distance, and the single `return best_index`.

## 4. Experiment matrix

| ID | Function | Shape | Size T/B | Relocs T/B | Decision |
|---|---|---|---|---|---|
| C1 | compare | 3-way long difference, single `return difference>0` | 272/320 | 15/15 | reverted (constants not folded) |
| C2 | compare | probes: ternary difference, short, init-0 difference, sign in-branch | 272/(292-320) | 15/15 | sign arms match only as in-branch `>0` |
| C3 | compare | sign returns + explicit FALSE/TRUE/FALSE | 272/272 | 15/15 | EXACT, landed |
| L1 | line_of_sight | in-place normalize2d cast, drop horizontal copy | 464/464 | 13/13 | EXACT, landed |
| A1 | active_select | short result, if/else stores, re-fetch | 576/592 | 19/19 | superseded |
| A2 | active_select | + single exit | 576/576 | 19/19 | register tie in fallback |
| A3 | active_select | top-scoped fallback index | 576/576 | 19/19 | no change, reverted |
| A4 | active_select | fallback tests actor fields, block-bound index | 576/576 | 19/19 | EXACT, landed (cleanly indented) |
| S1..S11 | select | structural completion series (section 3) | 5248/5200..5248 | 120/117..120 | S11 exact only with punning casts (not landable) |
| S-P13 | select | S11 with typed ai_debug.h members (scratch proposed header) | 5248/5248 | 120/120 | EXACT, proposal |
| S-L2 | select | P13 minus the three typed struct copies | 5248/5200 | 120/117 | landed fuzzy |

Every candidate was a whole-TU gate; all non-target rows were unchanged in every retained step (listing diffs), park drift 7/7 OK.

## 5. Do-not-repeat list

- compare: a shared `return difference>0` (any difference type) does not fold the evaluation constants.
- active_select: top-level vs block scope of the fallback index is inert; the tie is the field re-test.
- select: explicit `vector_from_points3d`+`magnitude_squared3d` for the linear distance, moving `from_actor` to the outer block,
  swapping the range comparison sides, and the ternary attractor object all measured worse or inert.
- Representation-punning casts onto `ai_debug.field_7D384` / `actor_record[].__unknown02` produce exact bytes but are prohibited;
  use the typed header instead.

## 6. Residual classification and orchestrator proposal

`_actor_select_firing_position` residual is **ownership/typing (header)**, not codegen: the only missing statements are the
three debug struct copies whose destinations are placeholder members in `source/ai/ai_debug.h`.

Proposal (measured, not applied):
- `scratch/workers/actor_firing_position/asfp/ai_debug_proposal.diff`: `struct ai_debug_actor_record { boolean pursuit; boolean valid; struct firing_position firing_position; }`,
  delete `struct ai_debug_block_7D384`, `field_7D380` -> `evaluation_context_valid`,
  `field_7D384` -> `struct firing_position_evaluation_context evaluation_context`, offset asserts retargeted.
- `scratch/workers/actor_firing_position/asfp/ai_debug_c_proposal.diff`: 21 renames in protected `ai_debug.c`
  (ai_debug.c's own renderer confirms `pursuit`/`valid`/evaluation fields).
- In `actor_firing_position.c`, replace the three disclosure comments by the typed copies and rename the three flag stores;
  reference body `scratch/workers/actor_firing_position/asfp/p13o.c` (strict EXACT 5248/120, all siblings unchanged).
- Direct consumers measured (25 TUs, 1,473 functions, per-function size/relocs/normalized sha): one executable delta, parked
  `_ai_communication_update_speech_timers` flips to its other documented flag-neutral load order (the struct-tag count effect:
  keeping `ai_debug_block_7D384` declared preserves it). Needs park re-baseline or a count-neutral form, and a full build.

Measured facts: all hashes above. Inference: the proposed member names (`pursuit`, `valid`, `evaluation_context`) are derived
from the writer (this function) and the reader (ai_debug.c renderer); no January PDB type record exists for them.

## 7. Reopen criteria

- `_actor_select_firing_position`: apply section 6 and verify with a full ninja build and the park census.
- Parked evaluators: unchanged; reopen only under their own parked.json criteria.

## 8. Disposition

Object remains NonMatching: 19/27 strict exact (3 new, 1,312 padded bytes), 1 fuzzy structurally complete residual pending a
header proposal, 7 locked parks. No credit is claimed for the fuzzy body.
