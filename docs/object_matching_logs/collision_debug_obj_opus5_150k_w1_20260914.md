# `collision_debug.obj` - opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and baseline

- Unit `source/physics/collision_debug.c`; target `build/split/source/physics/collision_debug.obj`; VC7 13.00.9254,
  `/O2 /Oy-`, no per-file override. Worker owns only this `.c`; headers and config frozen.
- Baseline gate (real file, guard `_point_from_line3d`): `residual 4192 _collision_debug_render [size 4112!=4192,
  relocs 275!=278, sha]`, `== exact 0 residual 1 unwritten 0`. No parked entries, no siblings.
- Target: 4192 padded / 4182 meaningful / 278 relocations, frame `0xb528`.
- Ledgers read first: `collision_debug_obj_opus5_100k_20260914.md` (starting point: its `best.c`, 4160/277, copied here
  as `scratch/workers/collision_debug/prev_best.c`), `collision_debug_obj_jonas_fixed_point_closeout_20260830.md`,
  `collision_debug_obj_jonas_public_donor_audit_20260826.md`, and the 100K worker notes. `branch_sweep`: nothing better
  than the 100K scratch best.

## 2. Outcome

**Not landed.** The real file is unchanged (`git diff --stat -- source/physics/collision_debug.c` is empty). The final
real-file gate equals the baseline and the guard passes. The function is an already-written residual, so the brief
allows a landing only at strict EXACT. The best candidate also needs the pending `symbols.json` spray split (the three
arrays would otherwise be new BSS owners that exist only in the candidate).

Best credible scratch candidate: `scratch/workers/collision_debug/best_w1.c` (identical to `n1.c`).

- Result: **4176 / 277**, frame `0xb528`.
- Skeleton diff (registers and stack slots normalized): 52 aligned instructions. The 100K best had 85.
- Guard passes; fake scan shows 0 leads.

Mechanism witness: `scratch/workers/collision_debug/n1_X2.c`.

- Result: **4192 / 278**. Every callee-saved register matches January.
- Skeleton diff: 7, all of them spray relocation *names*.
- It is not proposed for landing, because its spelling (section 4) is not credible enough.

## 3. New evidence (this wave)

1. **The spray loops are ordinary `short row, column` for-loops (structural; solved).** January's write loop and render
   loop are both
   `for (row = 0; row < COLLISION_DEBUG_SPRAY_ROWS; row++) for (column = 0; column < COLLISION_DEBUG_SPRAY_COLUMNS; column++)`
   with `short row; short column;`. The bit vector and both arrays are indexed by
   `row * COLLISION_DEBUG_SPRAY_COLUMNS + column`. There is no `spray_index` and no hand-written downcounter.
   - VC7 widens the short induction variables to 32-bit memory IVs (`fild dword [ebp-0x6c]` / `[ebp-0x24]`,
     `inc dword [ebp-0x6c]`) and generates the 30/40 downcounters itself.
   - The 100K lane imitated that output with `rows_remaining/columns_remaining` do-while loops. Those loops matched the
     write loop but could never produce the render loop.
   - Proof, mini TU with the unit's exact cflags (`scratch/workers/collision_debug/mini.py`, `minigen.py`): every
     `long row` form gives the continuous word index.
   - `short row` gives January's render loop instruction for instruction: `g2_sl`, `g7_ss_none`.
   - The 16-bit forms produce January's per-row reset from the `row*40` IV, the stored `row*8 - row*40` difference
     and `lea ecx,[diff+word]`.
   - In-situ, `n1.c` makes both loop nests skeleton-identical to January. The write-loop direction x87 order that the
     100K ledger called "coupled scheduling" is also fixed.
   - The explicit downcounters and `spray_index` from the 100K ledger items 8 and 3.6 are retired.
2. **The flags/ignore register choice is a creation-order tie, not a declaration-count tie.**
   - Declaration count: adding 1 to 8 dummy prototypes changes nothing (`dc1`..`dc8`).
   - Region-deletion bisection (`bisgen.py`): removing the pill branch or the vector-test branch flips the choice to
     January's `flags=esi, ignore=ebx`.
   - Inert shapes:
     - pointer locals in the render body
     - pill `position/velocity` as statements
     - `surface` declared together with its initializer
     - TAG_BLOCK chain without the node local
     - ignore/length statement order
     - `player_index` local removed, or a `?:` form
     - camera pointer local
     - declaration placement of `debug_vector`
     - copy-then-scale respellings
     - `point_from_line3d` / `scale_vector3d` inlines (these also emit forbidden COMDATs)
   - Flipping shapes:
     - `S5`: `debug_vector.i = collision_debug_vector.i * length`. The registers and 278 relocations become right, but
       that region's bytes differ from January (January copies first, then scales in place).
     - `X2` / `X7`: copy exactly one of the two `render.camera` fields componentwise. Size 4192, 278 relocations, all
       registers right.
   - Four equal-weight tie groups are left after X2, and they moved together with it:
     - ignore-home slot rank (`-0x5c` vs `-0x4c`)
     - write-loop column / row-real slot swap
     - render `lea` operand order
     - the phantom-point x87 operand order
   - Converting both camera copies (`X2X7`) or `debug_point` (`X1`) overshoots.
3. **The x87 operand order of `debug_vector.i * collision.t` looks systematic.**
   - January loads `v.i` first. Every spelling tested in-situ and in mini TUs loads `t` first:
     - `v*t` and `t*v`
     - named `t` local
     - `point_from_line3d`
     - const and non-const prototypes
     - plain-struct and union vector types
     - result layout, including the exact 0x420-byte struct with `t` first or second
     - vector or result accessed through a pointer
     - declaration order
     - direct `fstp` construction of the vector
   - Only a `double` `t` flips the order, and that is the wrong operand width.
   - January `action_vehicle.obj::_action_vehicle_find_destination` shows the same v-first order for
     `down_vector.i*collision.t + test_point.x`, and our build of it loads `t` first.

## 4. Why X2/X7 are not proposed

Nothing in January distinguishes a componentwise copy from a struct copy of `render.camera.position`/`forward`; the
emitted instructions are identical. Other reconstructed files copy these fields as whole structs (for
example `first_person_weapons.c:1059`; not re-verified as exact here). The effect is a creation-order steer, and three dependent ties plus the systematic x87
order still remain, so the shape would buy no exact credit anyway. It is recorded as mechanism evidence only.

## 5. Experiment index (all under `scratch/workers/collision_debug/`)

| id | shape | size / relocs | registers | result |
| --- | --- | --- | --- | --- |
| prev_best | 100K best.c | 4160 / 277 | ebx/edi | baseline for this wave |
| dc1-8 | +N dummy prototypes | = prev_best | ebx/edi | inert |
| r1 | render loop continuous spray_index | 4176 / 277 | - | inverted IVs (as 100K xe) |
| wd1-5, rr1 | loop-body composition diagnostics | - | - | one array use gives the January render form; composition, not index spelling, decides |
| b1/b2 | pointer locals in the render body | 4160/4176 | - | inert |
| t1-t4 | render spray_index short/word/unsigned | 4192 / 277 (t1) | - | movsx/movzx forms, wrong |
| mini/* | isolated render loop, 20 forms | - | - | `short row` for-loops = January |
| **n1 / best_w1** | short for-loops both nests, `row*COLUMNS+column` | **4176 / 277** | ebx/edi | best credible |
| n1b | same, separate declarations | = n1 | | inert |
| n2 | short row, long column | 4128 / 277 | | worse |
| bis/n1_* | region deletions | - | varies | pill or vector removal gives esi/ebx |
| P1 V1 V4 I1 L1 L2 CB D2 XF X4 X5 | natural respellings | = n1 | ebx/edi | inert |
| PL PS PF SC SD1-3 | helper inlines / scaling respellings | = n1 or worse | ebx/edi | inert; COMDATs forbidden |
| S5 | direct multiply scaling | 4176 / 278 | esi/ebx | scaling bytes differ |
| X6 | componentwise debug_vector copy | 4176 / 278 | esi/ebx | copies folded, differs |
| **X2 / X7** | one camera field componentwise | **4192 / 278** | **esi/ebx** | 4 tie groups left |
| X1, X2X1, X2X6, X2X7, X2XF, X2XG, X2XH | combinations | 4176-4208 | mixed | worse |
| D1 | phantom result declared at outer scope | 4192 / 278 | esi/ebx | frame 0xb7e8, worse |
| mini/ph_* | x87 operand-order probes (13) | - | - | t-first except `double t` |

## 6. Proposals

1. **config/symbols.json (endorsed again; text unchanged from the 100K ledger).** Replace
   `{ "file_offset": 4550336, "flags": 0, "name": "_collision_debug_spray_globals" },` with:
   ```
   { "file_offset": 4550336, "flags": 0, "name": "_collision_debug_spray_normals", "static": true },
   { "file_offset": 4564736, "flags": 0, "name": "_collision_debug_spray_points", "static": true },
   { "file_offset": 4579136, "flags": 0, "name": "_collision_debug_spray_hit_flags", "static": true },
   ```
   - Evidence: January addresses the region with three independent addends (+0, +0x3840, +0x7080). The three separate
     arrays in `best_w1.c` reproduce the multiset, the offsets and the reloc count.
   - Names are descriptive; no atlas, PDB or HCEA name exists.
   - Consumer: `source/physics/collision_debug.c` only.
2. **Source, only after proposal 1.** Adopt `best_w1.c` as the retained zero-credit body.
   - It is strictly closer than the current file (4176/277 vs 4112/275).
   - It is more natural: plain `short` for-loops, no `spray_index`, no hand-written downcounters.
   - Guard and fake scan are clean.
   - Owner census against build/base: the three spray arrays replace `_collision_debug_spray_globals`, and the January
     `count<=14` literal replaces the old assertion literal.

## 7. Do-not-repeat additions

- Render/write-loop index spellings with `long` counters, explicit downcounters or `spray_index`. The loop form is solved:
  `short row, column`.
- Declaration-count probes; the inert respellings listed in section 5.
- x87 operand-order spellings of `v.i * collision.t` (section 3, item 3).
- Componentwise-copy placement sweeps. The mechanism is known, and they are steering unless January-side evidence
  appears.

## 8. Reopen criteria

- A January-side explanation of the v-first x87 operand order, for example a lane that closes
  `_action_vehicle_find_destination`'s identical pattern. Then resume from `n1_X2.c` / `best_w1.c`: the rest is the
  creation-order tie family.
- Or new evidence (PDB locals, a donor body) for how January built the repeat-block and scaling statements.
