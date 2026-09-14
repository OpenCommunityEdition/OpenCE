# `damage.obj` wave w3 second pass (Opus5 150K house-clean, 2026-09-14)

## Result

`source/objects/damage.c` was **not modified** (`git diff --stat` empty). Final real-file gate:
`== exact 32 residual 2 unwritten 0`, point guard passed, `scratch/parkcheck.py` 2 parks / 0 drift, owner
census identical to `build/base`, fake scan 0 leads. No function became exact; no reopen is proposed.
The inherited `_object_cause_damage` (100K lane) was not touched or claimed.

| function | January size / relocs / sha16 | best scratch (w2 `park_rebase_candidate.c`, re-gated as w3 `d0.c`) | disposition |
|---|---|---|---|
| `_area_of_effect_cause_damage_to_object` | 1328 / 51 / `d3f929d4ce73b3cd` | 1312 / 51, one x87 kill-form instruction | parked |
| `_object_damage_body` | 1360 / 43 / `15116626eee783a5` | 1360 / 43, four register / x87-copy packets | parked |

## Prior work read

- Ledgers: `damage_obj_opus5_150k_w2`, `_opus5_100k_waveD`, `_area_of_effect_cluster`,
  `_object_cause_damage_fuzzy`, `_object_damage_update`, `_small_vitality_leaves`, `_private_helpers`,
  `collision_bsp_damage_owner_reconciliation`.
- Also: both `parked.json` entries, `scratch/workers/damage.md`, `scratch/w2/laws_w2.md`, and the brief.
- All do-not-repeat lists were respected.

## Law detector pass

| law | AOE clamp | body tail |
|---|---|---|
| A3 single exit | n/a (no result flag) | n/a (void) |
| A4 helper routing | no `real_math.h` helper expresses the falloff | vertex helpers n/a |
| A5 alias removal | n/a | `region` one-use alias already inert (waveD v3); `child_unit` has 3 uses, so it is genuine |
| A10 conditional initializer | pre-init already measured worse (w2 s2) | if/else `actual_damage` changes January-identical bytes at 0xca/0xd5, so January uses the pre-initialised form |
| A11 explicit-cast CSE kill | all operands are `real`; a double literal turns January's dword constants into qword (probe `dp1.c`), so refuted | n/a |
| unroller / loop form | n/a | reusing the `region_index` parameter as the loop index: identical; `while` loop: worse (1344) |

## `_area_of_effect_cause_damage_to_object`

**Residual (w2 scratch).** January kills the dead radius delta with `fxch st(1); fstp st(0)`; we emit
`fstp st(1)`. The two extra bytes cross the 16-byte pad (1312 vs 1328).

**New diagnostics.** These used a micro TU with damage.obj flags and were not landing shapes:

- `dp1.c`: double-literal variants.
- `dvar.py`, 8 forms:
  - reuse `distance` as the result;
  - a block-local fraction;
  - else arm first;
  - `0 < delta`;
  - PIN inside the arm;
  - no delta local;
  - a two-statement quotient.
- `km1.py`-`km3.py`, 22 forms:
  - HCEA if/else-if clamp in either arm;
  - a ternary inside PIN;
  - FLOOR/CEILING split;
  - a declaration initialiser;
  - downstream store, compare, `scale_vector3d` and three-use multipliers.

Every form that keeps a kill emits `fstp st(1)`. The January kill form still reproduces only in w2's
`p8.c`, where a scalarised aggregate member is consumed by a three-use inline helper. This function has no
such counterpart. No evidence-led real-TU shape remained. Residual: class D x87 kill-form tie.

## `_object_damage_body`

The w2/waveD best body (`d0.c`) has four remaining packets:

1. The dying-loop `edi` reload comes before `object_destroy_region`; January reloads after it.
2. The body-damaged effect index uses `ecx`; January uses `eax`.
3. The localized `damage_resistance` reload uses `edx`; January uses `ecx`.
4. `*body_damage = damage_amount` is copied through x87; January uses an integer copy.

Diagnostics `b1` (if/else), `bd1` (parameter reuse as the loop index) and `bd3` (while loop) were all
reverted. None moved packets 1-4. Residual: class C/D global allocation tie.

## Reopen criteria (unchanged)

- **AOE:** a measured source mechanism that selects `fxch st(1); fstp st(0)` for a dead value lying below
  a freshly computed x87 result.
- **Body:** a measured law for integer versus x87 copies of a real frame local, or for reload placement of
  a register-ABI argument after a clobbering private call inside a loop.

## Do-not-repeat (additions)

- **AOE:** all dvar/km/dp1 probe forms listed above.
- **Body:**
  - if/else `actual_damage`;
  - parameter reuse as the dying-loop index;
  - a `while` dying loop.
