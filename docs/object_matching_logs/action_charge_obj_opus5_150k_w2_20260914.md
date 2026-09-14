# `action_charge.obj`: setup frame lever and helper-linkage measurement (Opus 5 150K house-clean lane, wave w2, 2026-09-14)

## Result

- `_action_charge_setup` (944 padded, previously a structurally incomplete 688 / 20 residual) is now a complete typed-C body,
  landed as **fuzzy** (zero credit): **944 / 29 relocations, instruction-identical to January except the placement of one
  `result = FALSE` store** (`mov byte [ebp-1],0` at +0x138 instead of +0xe4).
- A **strict-EXACT** setup candidate exists (`scratch/workers/action_charge/w2/v3.c`, 21/22 object). It is held back because it
  needs a January-proven redundant `result = FALSE;` store (see section 4). Owner ruling requested.
- `_action_charge_perform` is unchanged (fuzzy 3216 / 84). The requested helper-linkage measurement is recorded in section 5; the
  ruling alone does not produce an exact function.
- Object: `== exact 20  residual 2  unwritten 0` (baseline identical counts). No newly exact function. All 20 baseline-exact rows
  keep their listings row for row.

## 1. Baseline and provenance

- Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, base `ea9175577`. Gate without the `_point_from_line3d` guard
  (brief 3b: action_charge.obj is the genuine owner; the final object defines exactly one `_point_from_line3d`, strict EXACT,
  sha16 `9b763841f8519177`).
- Baseline rows: perform `[size 3216!=3248, sha]`, setup `[size 688!=944, relocs 20!=29, sha]`, 20 EXACT.
- Read: all eight earlier action_charge ledgers, the 100K handoff, the fresh-graphs worker notes and scratch
  (`opus5-30k-fresh-graphs-20260914/scratch/workers/action_charge`: `setup_v12.txt`, `t_setup.asm`, `t_perform.asm`).
  `branch_sweep`: 12 blobs, none better. A cut-off earlier run in this lane left undocumented probes
  (`scratch/workers/action_charge/d_*.c`, `probe/`); used as evidence only.

## 2. New evidence

- **HCEX.pdb (SHIP) locals** (`DIA2Dump -sym`): `action_charge_setup` homes only `short melee_start_tick`,
  `short melee_end_tick`, `float melee_start_range`, `float melee_end_range` (all other locals enregistered).
  `action_charge_valid_melee_destination` is `static unsigned char (long actor_index, short goal, union real_point3d *melee_target_point)`
  with locals `estimated_position`, `collision`. Later build: names and signature shape only.
- **January caller bytes** confirm that signature: setup pushes `[ebp+0xc]` (the short `goal` parameter) and passes
  `&prop->center_of_mass` in ESI. The 2026-09-06 ledger's "the point is the second argument, do not import the short
  signature" premise is refuted by these bytes; the helper ignores the stack `goal`, and its point parameter is
  `melee_target_point`. The helper stays strict EXACT with the authentic signature, and the `(void)melee_target_point;` line is gone.
- `ai_debug.c` charge-decision renderer: `field_198` is the "airborne" (leap) flag, `field_194` is the cannot-move range.

## 3. Setup frame lever (new measured VC7 13.00.9254 law)

January's frame is 0x20 with `leap` (byte) at [-8] and `end_tick` (short) at [-6] in **one** 4-byte cell. The v12 body declares
`leap` and the four tick/range outputs in the same block and gets 0x24 (separate cells). Wrapping the four outputs in a nested
block around the `unit_get_melee_range_and_ticks` if/else (leap stays in the enclosing block) reproduces January's frame and
slot set exactly.

Law: locals declared in the **same** block never share a frame cell. A nested-block local can be packed into, or overlap, an
enclosing local's cell when their live ranges allow it. Probe `scratch/workers/action_charge/probe2/b.c`: f1 (same scope)
frame 20, f2/f3/f4 (outputs nested) frame 16. This contradicts the older note that flattening a block is inert. Only
promotion to the enclosing scope was measured there.

## 4. Setup: the redundant-store blocker

January stores 0 to `result` ([ebp-1]) at +0xe4, which dominates the inhibited test, and again at +0xea (inhibited arm) and
+0x107 (notarget arm). The deep failure arms (no-animation, cannot-move) have no store. Only the literal source
`result = FALSE;` before `if (TEST_FLAG(... inhibited))`, together with `result = FALSE;` in both arms, is strict EXACT
(`w2/setup_exact.txt`).

Non-redundant alternatives measured (all on the nested-scope base):

| Shape | Size / relocs | Outcome |
| --- | --- | --- |
| A1 pre-branch store only | 1024 | tail-duplicated epilogues |
| A4 store at top of melee arm | 1024 | same |
| A6 stores in every failure arm, none before | 1056 | `result` loses its home |
| R1 / R1b / R2 / R3 early `state_data->goal = goal; return ...;` arms | 1024 / 992 / 1024 / 1024 | own epilogues; VC7 does not coalesce return values into `result` |
| **H1** arm stores + `result = FALSE;` first in the melee-attempt arm | **944 / 29** | one store placement differs (**landed, fuzzy**) |
| H3 same store before `prop_get` | 928 | |
| S2a / S5 separate valid/success flag copied into `result` | 1072 / 1072 | |

Owner ruling requested: admit the January-proven redundant store (`w2/v3.c`, the landed file with one `result = FALSE;` moved to
before the inhibited test) for +944 padded exact. The class is the handoff's "January-proven redundant logic"
(`_extract_sequence`).

## 5. Perform: helper-linkage measurement

`w2/v4_inline.c` is the landed file with `__inline` added to the 11 local helper definitions. January's atlas `i` records mark
`normalize2d`, `point_from_line3d`, `magnitude3d`, `normalize3d` and `collision_test_line` as inline COMDATs.

- All 20 other rows stay strict EXACT, with no new owner.
- Perform goes from 3216 to 3264 padded (meaningful about 3250 against January's 3246).
- The two separate `direction = prop->actor_to_prop` copies come back.

What remains after the lever:

- one extra `mov ecx,[ebp-8]` prop reload at +0x256 (the only instruction-count difference);
- the melee-abort block: January keeps `check_range` in BL and reloads the prop, with cells `check_range` [-2] and
  `berserk_ranges`+`melee` [-1]; ours keeps the prop in EBX;
- the `&field_114` register;
- x87 scheduling of `lead_fraction * ticks`, the suicide dot product and the ticks `movsx`;
- argument registers of three calls.

Two more shapes were inert: W1 (the weapon pointer in its own block) and W3 (negated-or LOS test). Not landed:
perform is an already-written residual and none of these is strict EXACT.

## 6. Checks (landed real file)

- `gate --all`: `exact 20 residual 2 unwritten 0`. The diff against HEAD's listing is only the setup row
  (`[size 688!=944, relocs 20!=29, sha]` becomes `[sha]`).
- Parks: none for the unit.
- Owner census against `build/base`: the only new owners are the target-owned `actor %s melee animation has no damage keyframe`
  string and the `state_data` assert literal, which January references as undefined. Pre-existing non-target owners
  (`_vector_from_points3d` SELECT_ANY, `__real@41f00000`, `__real@3f000000`, D3D tables) are unchanged.
- `tools/fake_match_scan.py`: 0 leads.

## 7. Proposals

1. Owner ruling: admit `w2/v3.c` (redundant `result = FALSE;` proven by January +0xe4/+0xea/+0x107) as strict EXACT setup.
2. Helper linkage (`__inline` local helper definitions): measured in section 5. It does not give exact credit on its own, so it is
   not needed now.
3. Pre-existing debt, not introduced here: action_charge.c carries a consumer-local prototype of units.c's
   `unit_get_melee_range_and_ticks`. Its owner would be `source/units/units.h`, which has 66 includers
   (declaration-count risk, units.c protected neighbour). Proposed text, placed next to `unit_melee_attack_begin`:
   `boolean unit_get_melee_range_and_ticks(long unit_index, boolean secondary, short *melee_tick, real *attack_time, short *frame_count, real *damage_time);`
   in house one-parameter-per-line form.

## 8. Reopen criteria

- **setup:** the owner ruling above, or a non-redundant source that emits a dominating FALSE store before the inhibited branch
  while keeping both arm stores.
- **perform:** a helper-linkage ruling plus a source-level cause for the +0x256 prop reload and the `check_range` BL allocation.
  Do not re-permute declaration order or scopes (fresh-graphs do-not-repeat list).
