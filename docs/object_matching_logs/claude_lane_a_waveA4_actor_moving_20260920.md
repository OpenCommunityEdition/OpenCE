# Lane A wave A4 — `_actor_move_update` structural corrections

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, on top of wave A3 (`4fa32b2c`).

**New strict bytes: 0. New strict functions: 0. Regressions: 0.** Zero exact credit.

## Result

| `_actor_move_update` | baseline | after |
|---|---:|---:|
| section size | 3,088 | **3,120** (January 3,136) |
| size gap | 48 bytes short | **16 bytes short** |
| instructions | 869 | **882** (January 888) |
| instruction gap | 19 short | **6 short** |

Unit census unchanged at `exact 31 residual 5 unwritten 0`.

## The three retained changes

1. **Initialized-declaration order.** Four initialized `boolean`/`real` locals are
   reordered to the sequence January's twelve-store prologue emits. This is *not*
   the inert declaration-order permutation the campaign warns about: slot
   assignment is inert, but the *initializer stores* are emitted in declaration
   order, and January's prologue store sequence is directly observable.
2. **Branch polarity.** `else if (moving_into_fire_timer <= 0)` becomes
   `else if (moving_into_fire_timer > 0)` with its two arms swapped. Semantically
   identical; branch polarity is a class-A source lever in
   `docs/matching_methodology.md`.
3. **Comparison spelling.** `normalize3d(&grenade_direction) > 0.f` becomes
   `!= 0.f`, matching January's emitted comparison.

## What was rejected, and why the rejection made it better

The worker's preferred candidate (`cand_25`) additionally expanded
`actor_move_transform_avoidance_vector` by hand at two call sites in
`_actor_move_test_avoidance_vector`. That is **manual inlining of an existing
ordinary helper**, which house rule 13 forbids, and "manual expansion" is named
explicitly on this unit's do-not-repeat list in the lane brief. Not landed. Its
measurements are preserved below as evidence, because they are genuinely new:

    _actor_move_test_avoidance_vector  544 -> 768   (January 752)   relocs 17 -> 16, now MATCHING
    _actor_move_vector_avoidance      4192 -> 4160  (January 4144)  relocs still 140 vs 135
    _actor_move_update                3088 -> 3104  (January 3136)

So the expansion does fix `test_avoidance_vector`'s relocation count and shrinks
its size gap from 208 to 16 — but it overshoots rather than converging, it is
forbidden source, and the worker flagged it as contentious themselves.

The worker's own rule-15-clean fallback (`cand_28`) also carried a fourth change:
rewriting `TEST_FLAG(flags, crouch ? A_bit : B_bit)` as
`(crouch && TEST_FLAG(flags, A_bit)) || (!crouch && TEST_FLAG(flags, B_bit))`.
**Measured and dropped.** Restoring the natural ternary and keeping only changes
1-3 gives **3,120** bytes against `cand_28`'s 3,104 — the concise, more plausible
source is also the closer one. The verbose `||` expansion was costing 16 bytes.

## Reopening conditions

- `_actor_move_test_avoidance_vector` — an **authentic** mechanism for January's
  absence of the helper call: a genuine macro or `__inline` definition, an
  include/owner difference, or a same-compiler donor call site. January
  references `_global_zero_vector3d` here and makes zero calls to
  `actor_move_transform_avoidance_vector` where we make two; the operations are
  therefore in January's source, but hand-expanding them is not an admissible way
  to express that. 16 bytes and 6 instructions separate the expanded form from
  January, so even the forbidden shape is not a closure.
- `_actor_move_update` — the remaining 16 bytes / 6 instructions.

## Verification

    ninja -j4 all_source progress semantic_progress
      -> Validated parked compiler ties: 184
      -> halobetacache 1,444,193 / 1,770,166 (7,292 / 7,574)   [unchanged]
    stable_verdicts diff vs frozen baseline: gained 1 / 224 bytes / regressions 0 (exit 0)
    parks 184/0/0 | admission 19/0/8/0 | source/ai fake scan 2 (both inherited)
    fake scan on the changed file: 0 | point guard PASS | git diff --check clean

No park entry was touched: `actor_moving` has none.
