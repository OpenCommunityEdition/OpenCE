# Lane A wave A2 — three zero-credit source corrections

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, baseline
`6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`, on top of wave A1 (`9eca0524`).

**New strict bytes: 0. New strict functions: 0. Regressions: 0.**
Everything in this wave is a source-authenticity correction retained at **zero
exact credit**, as house rule 11 requires.

## 1. `actor_firing_position.c` — `_pre_evaluator_attack` recovers the missing relocation

| | before | after |
|---|---|---|
| gate tag | `[relocs 38!=39, sha]` | `[sha]` |
| relocations | 38 / 39 | **39 / 39** |
| differing relocation rows | 24 | **9** |
| objdiff | 93.621216 | 96.10909 |
| unit census | `exact 26 residual 1` | `exact 26 residual 1` (unchanged) |

Four levers, each decided by January's bytes rather than by taste:

- **L1** `evaluation = 20.0f * (distance_from_boundary / 2.0f)` replaces
  `distance_from_boundary * 0.5f * 20.0f`. VC7 reassociates the latter, folding
  `0.5f * 20.0f` into `10.0f` and losing a relocation; the former keeps both
  constants and restores the `__real@41a00000` reference. **This is the relocation
  that was missing.**
- **L2 / L3** two declaration initializers become statements. The store of zero to
  `[ebp-0x1c]` sits at `+0x1d8` in January and `+0x12d` in ours, on opposite sides
  of `call _actor_get_weapon_definition` at `+0x169`. A stack store cannot be sunk
  across a call, so that position **is** January's statement order.
  `nearest_distance_squared = REAL_MAX` deliberately stays an initializer, so the
  distinction is authorial, not a blanket restyle.
- **L4** an explicit `if (distance_squared < nearest_distance_squared)` replaces
  `MIN()`. `cseries.h` defines `MIN(a,b)` as `((a)>(b)?(b):(a))`, which can only
  emit `test ah,0x41`; January emits `fcom [ebp-0x10] / test ah,5 / jp`, a strict
  `<`. This is byte evidence, not a style preference.

Relocation addresses now agree exactly through `+0x317` and shift only from
`+0x36b`, inside the single remaining window. `bijection_walk` reports genuine
structural events, so a pure register bijection is excluded by measurement.

A defect was also fixed in passing: the inherited intermediate candidate carried
9 LF-only lines into an otherwise 100% CRLF file. The landed file is 2,790/2,790
CRLF with no BOM.

## 2. `ai.c` — two independent corrections

### `_ai_test_ballistic_line_of_fire`: instruction counts now match

| | before | after |
|---|---|---|
| instructions | 292 vs January's 296 | **296 vs 296** |
| real differing regions | 28 | **11** |
| objdiff | 94.5 | 98.99645 |

This rebuilds the recorded 250K-lane `w3c b5` shape from this baseline: block-scope
`segment_time` to the coordinate statement; parenthesise the linear term of the z
row as `(segment_time * arc_velocity.k + point.z)`; spell the post-call velocity
update as `(segment_end_time - segment_start_time) * gravity + arc_velocity.k`.

It reproduces January's x87 structure for `segment_time` exactly — `fsubr` of the
parameter home at `+0x1e7`, `fst` (store-and-keep) to a dead parameter home at
`+0x1f4`, and a post-call `fld` from that same home at `+0x2cc` feeding
`fmul gravity / fadd / fstp`. The later source-level subtraction is therefore
*not* a duplicated computation in the emitted code: VC7 reloads the stored value,
which is what January does.

Surviving hunks: an `ebx`/`edi` reload transposition at `+0x159`, a
`collision_flags`/`segment_time` parameter-home swap, one 3-versus-2 x87 hunk at
`+0x21a`, and a consequent 3-byte alignment pad.

### `_ai_test_line_of_fire`: the store-order half of `lof2`, and only that half

Inside `if (intersected)`, `line_of_fire = FALSE;` now precedes
`blocking_prop_index = pills[pill_index].prop_index;`, matching January's store of
zero to `[ebp-4]` at `+0xd3`, then `[ebp-8]` at `+0xd7`, then the hit byte at
`+0xda`. objdiff 88.17 → 89.19643.

The correspondence is deterministic and reversible — swapping the source swaps the
emission — so VC7 is order-preserving for these two independent stores and
January's order is source-determined.

**The rejected half is not landed.** The prior orchestrator's rejection of the bare
call-site parentheses `(pills[pill_index].width)` stands; its reopening criterion
(an attested or owner-approved parenthesising spelling for
`fast_vector_intersects_sphere` radius arguments, which would also unlock
players `_player_examine_nearby_device`, collisions `_object_test_vector` and
bipeds `_biped_update_moving`) is unmet by anything found in this lane.

## 3. `actors.c` — a 12-byte `.bss` January does not have

| | `.data` | `.bss` | COMMON |
|---|---|---|---|
| January `actors.obj` | 4 B `_global_updating_actor_index` | **absent** | none |
| ours, before | 4 B, same symbol | **12 B** `_actor_data`, `_swarm_component_data`, `_swarm_data` | none |
| ours, after | 4 B, same symbol | **absent** | 4 B each, all three |

January defines all three in the synthesized `build/split/source/linker_common.obj`
(sections 209/210/211, storage class 2) and its own `actors.obj` references them as
**undefined** — the COMMON model, i.e. the original source used *tentative
definitions*. Our source forced real `.bss` allocation:

    source/ai/actors.c:576  struct data_array *swarm_data = NULL;
    source/ai/actors.c:577  struct data_array *swarm_component_data = NULL;
    source/ai/actors.c:578  struct data_array *actor_data = NULL;

Those were **the only `struct data_array *x = NULL;` definitions in the entire
source tree.** The established convention — used by the sibling unit that is 60/61
exact — is the tentative form (`source/ai/encounters.c:585,588`). Dropping the
three initializers removes the `.bss` entirely, moves all three symbols to COMMON
matching January's `linker_common` attribution, and leaves the function census
unchanged at `exact 72 residual 4`.

Semantics are identical (static storage duration is zero-initialized either way).
This is a prerequisite for any future whole-object admission of `actors.obj`.

## `config/parked.json`

Three entries were **re-baselined**, none retired, none unrelated touched:
`actor_firing_position:_pre_evaluator_attack`, `ai:_ai_test_line_of_fire`,
`ai:_ai_test_ballistic_line_of_fire`. Each entry's `measurements.base` and
`objdiff_percent` were refreshed from the rebuilt objects and its `evidence`
extended with the lever and a reopening condition. Fuzzy bytes remain at zero
credit. The `ninja progress` park validator is what forced this: it fails on stale
measurements, which is the intended safety net.

## Verification

    ninja -j4 all_source progress semantic_progress
      -> Validated parked compiler ties: 184
      -> halobetacache 1,444,193 / 1,770,166 (7,292 / 7,574)   [unchanged from wave A1]
      -> overall 1,465,425 (7,559)

    stable_verdicts diff (vs the frozen baseline, not the previous wave):
      gained 1 / 224 bytes / regressions 0      (exit 0)
    accepted ledger:        +1 function / +214 meaningful, regressions 0
    parked functions:       184 active / 0 stale / 0 invalid
    admission audit:        19 candidates / 0 contradicted / 8 rejected / 0 revoked
    fake scan, each changed file: actor_firing_position 0, actors 0,
                                  ai 2 (both the inherited #pragma optimize pair, lines shifted only)
    fake scan, source/ai:   2 findings, unchanged from baseline
    point guard (--forbid-emitted-symbol _point_from_line3d): PASS on all three units
    git diff --check:       clean

No header, comparator, target object, build tool, `config/config.json` or
`config/symbols.json` change.
