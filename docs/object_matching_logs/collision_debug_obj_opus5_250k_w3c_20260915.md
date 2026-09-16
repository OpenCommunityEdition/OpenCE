# `collision_debug.obj` - opus5 250K house-clean lane, wave w3c (2026-09-15)

## 1. Scope and baseline

- Unit `source/physics/collision_debug.c`; target `build/split/source/physics/collision_debug.obj`;
  VC7 13.00.9254, `/nologo /c /O2 /Oy-`, no per-file override. Worker owns only this `.c`.
- Baseline gate (real file, guard `_point_from_line3d`):
  `residual 4192 _collision_debug_render [size 4112!=4192, relocs 275!=278, sha]`,
  `== exact 0 residual 1 unwritten 0`. No parked entries, no sibling rows.
- Target: 4192 padded / 4182 meaningful / 278 relocations / sha16 `5a365dfa90e8300c`, frame `0xb528`.
- Ledgers read: `collision_debug_obj_opus5_150k_w1_20260914.md`, `collision_debug_obj_opus5_100k_20260914.md`,
  `collision_debug_obj_jonas_fixed_point_closeout_20260830.md`, `collision_debug_obj_jonas_public_donor_audit_20260826.md`,
  plus `scratch/workers/collision_debug.md`. `branch_sweep`: best history blob `a37c99c9` sizedelta 64 - worse than the
  w1 scratch best (sizedelta 0). The atlas record for this object is EMPTY and the PDB is public-only, so no local or
  static names exist; Ghidra is hypothesis-only.
- This unit is `laws_w3` C1 entry "collision_debug / _collision_debug_render / 4192 / local-first x87 products /
  A35: on the scan; no w3 worker looked at it".

## 2. Outcome

**NOT LANDED.** The real file is unchanged (`git diff --stat -- source/physics/collision_debug.c` empty); the final
real-file gate equals the baseline and the guard passes. The function is an already-written residual, so the brief
allows a landing only at strict EXACT.

Best scratch candidate: `scratch/workers/w3c_collision_debug/pF.c` - **4192 padded / 278 relocations / frame 0xb528**,
40 real diff blocks (was 241 for the w1 credible best `best_w1.c`, 45 for the w1 mechanism witness `n1_X2.c`).
Guard clean, fake scan 0 leads.

## 3. New evidence (this wave)

### 3.1 The w1 reopen criterion is answered: laws_w3 A35 explains the v-first x87 order, and closes the block

w1 recorded "January loads `debug_vector.i` first in `debug_vector.X * collision.t`; every spelling tested in-situ and
in mini TUs loads `t` first ... this looks systematic" and made a January-side explanation the reopen criterion.
The explanation is A35(1) (leaf age), and two source facts flip it:

1. **`scale_vector3d(&debug_vector, length, &debug_vector)`** after `debug_vector = collision_debug_vector;`.
   The fields are written THROUGH AN INLINE POINTER PARAMETER, so they stay young; the **j** and **k** rows flip to
   January's `fld debug_vector.X; fmul collision.t` while the scaling code itself stays byte-identical (January's
   copy-then-multiply-in-place bytes at T `0x2f7..0x33b`, i.e. `mov [-0x48],ecx; fld [-0x48]; fmul st(1); fstp [-0x48]`).
   The w1 ledger listed this shape (`n1_SC`) as "inert" because it is inert in size and relocations - it is NOT inert
   in the x87 leaf ages, which is what the residual actually needed.
   The A30 census removes w1's other objection: `_scale_vector3d` is a real_math.h select-any surplus COMDAT on the
   accepted systemic list (only `_point_from_line3d` and `_vector_from_points3d` are out).
2. **`real t = collision.t;`** in the phantom block. The remaining **i** row is A35(1)'s documented offset-0 anomaly:
   both leaves sit at offset 0 of their object (`debug_vector.i` and `collision.t`, `t` being the first member of
   `struct collision_bsp_test_vector_result`). Binding the field to a named local, used by the three component
   statements, closes it: 42 -> 40 real diff blocks, and the two removed blocks are exactly T `0x375` and T `0x37e`.

With both, the entire `collision_debug_phantom_bsp` block is byte-identical to January.

Negative worth recording: `point_from_line3d(&debug_point, &debug_vector, collision.t, &point)` (`n1_PL`, lane-OUT in
any case) makes **all three** rows `t`-first, i.e. strictly worse than the hand-expanded statements. No later wave
should spend an owner-ruling request on it for this function.

### 3.2 A4 helper routing replaces w1's rejected `X2` camera hand-expansion with an admissible spelling

w1 needed componentwise `collision_debug_point.x = render.camera.position.x; ...` to get January's callee-saved
assignment (flags=ESI, ignore=EBX) and the 278th relocation, and declined to propose it because nothing in January
distinguishes a componentwise copy from a struct copy (a hand expansion of a copy, laws_w3 R15 class).

`set_real_point3d(&collision_debug_point, render.camera.position.x, render.camera.position.y, render.camera.position.z)`
- a genuine real_math.h helper whose select-any COMDAT is on the A4/A30 accepted census - produces a **byte-identical
object** to the `X2` hack (`pF` and `pC` diff-block headers are identical), with flags=ESI, ignore=EBX, 4192/278.

Controls:
- routing the second copy the same way (`set_real_vector3d(&collision_debug_vector, ...)`, `pG`) overshoots to
  4208 and 233 diff blocks, reproducing w1's `X2X7` overshoot;
- without either form (`pE`, natural struct copies plus the two A35 fixes) the object is 4176/277, flags=EBX,
  ignore=EDI, 241 diff blocks.

The asymmetry (January's `collision_debug_point` assignment is three scalar stores in the IL, its
`collision_debug_vector` assignment is a struct copy) is measured but not explained; see the reopen criteria.

### 3.3 The residual is now four causes, arithmetically characterised

`relocdiff --count-by-target` on `pF.obj`: **278 target / 278 candidate, 7 differing rows, all of them the three spray
BSS names**. So the relocation multiset, the relocation count, the size and the frame are all correct, and the
remaining byte differences are:

| id | blocks | cause |
| --- | ---: | --- |
| (a) | 5 | the three spray relocation NAMES - the pending `config/symbols.json` split (section 5) |
| (b) | ~20 | frame rank tie between the 4-byte cell `{ignore_object_index, render-loop `row*8-row*40` temp, radius}` (T `-0x4c`, 6 refs) and the 16-byte cell `{instance, clipped_velocity, projection_bounds, direction, features `center`, point-spray x CSE}` (T `-0x5c..-0x50`, 24 refs). Both are exactly **1.5 refs/byte**; the per-slot ref censuses of target and candidate are IDENTICAL (multiset {4,4,12,4,6} over `0x4c..0x5c`). January ranks the small cell first, ours the big cell; everything else in the group shifts by one slot. |
| (c) | ~8 | second frame rank tie: the write loop's `column` IV (T `-0x6c`, 3 refs) vs the hoisted `(real)row` temp (T `-0x70`, 4 refs) are swapped in ours. This one CONTRADICTS a refs/size rank in January's favour (3/4 outranks 4/4; frequency weighting makes it worse still), and the emitted definition order (`mov [column],0` then `fstp [row_real]`) is the same in both builds. |
| (d) | 2 | `fld row_real; fmul vertical_step.i|.j` (January) vs `fld vertical_step.i|.j; fmul row_real` (ours) in the spray write loop. The **k** row already matches in both, because column_real's last use consumes st(0) there. |
| (e) | 1 | `lea ecx,[eax+ebx]` vs `[ebx+eax]`, render loop only (the same source expression emits `lea ecx,[ebx+esi]` in both builds in the write loop). |

## 4. Experiment index (all under `scratch/workers/w3c_collision_debug/`)

| id | shape | size / relocs | real diff blocks | result |
| --- | --- | --- | ---: | --- |
| n1 (= w1 `best_w1.c`) | w1 credible best, re-gated at this HEAD | 4176 / 277 | 241 | carry-forward baseline |
| n1_X2 | w1 mechanism witness (componentwise camera position) | 4192 / 278 | 45 | carry-forward baseline |
| v1 | n1 + `scale_vector3d(&debug_vector, length, &debug_vector)` | 4176 / 277 | 241 | phantom j,k flip to January |
| v2 | n1_X2 + scale_vector3d | 4192 / 278 | 42 | phantom j,k flip to January |
| pA | v2 + `ignore_object_index` declared first in the outer block | 4192 / 278 | 42 | byte-identical (rank tie (b) is not source-visible) |
| pB | v2 + index-first products `column * horizontal_step.i + row * vertical_step.i + base_vector.i` | 4192 / 278 | 42 | byte-identical - refutes a commutative (R3) fix for (d) |
| **pC** | v2 + `real t = collision.t;` | 4192 / 278 | **40** | phantom block complete |
| pE | pC with the struct-copy camera (no X2) | 4176 / 277 | 241 | the X2-class shape is load-bearing for the register roles |
| **pF** | pC with `set_real_point3d(&collision_debug_point, ...)` in place of X2 | **4192 / 278** | **40** | best; byte-identical to pC with an admissible spelling |
| pG | pF + `set_real_vector3d` for the forward copy | 4208 / - | 233 | overshoot (= w1 `X2X7`) |

## 5. Proposals

### ORCHESTRATOR-PROPOSAL 1 - `config/symbols.json` (third endorsement, text unchanged)

Replace `{ "file_offset": 4550336, "flags": 0, "name": "_collision_debug_spray_globals" },` with:

```
{ "file_offset": 4550336, "flags": 0, "name": "_collision_debug_spray_normals", "static": true },
{ "file_offset": 4564736, "flags": 0, "name": "_collision_debug_spray_points", "static": true },
{ "file_offset": 4579136, "flags": 0, "name": "_collision_debug_spray_hit_flags", "static": true },
```

- Evidence (100K + w1): January addresses the region with three independent relocation addends (+0, +0x3840, +0x7080)
  and a 2/2/3 reference multiset that only three separate static arrays reproduce; the aggregate struct folds two of
  the pointers into one IV (100K `s4.c`, 275 relocations).
- NEW this wave: with `pF.c` the candidate already has **278/278 relocations and all seven differing relocation rows
  are these three names**, so the split is the only naming blocker left.
- Names are descriptive reconstructions (no atlas, PDB or HCEA name exists; the current `_collision_debug_spray_globals`
  is equally invented). Consumer: `source/physics/collision_debug.c` only.
- Credit: zero on its own; causes (b)-(e) remain.

### ORCHESTRATOR-PROPOSAL 2 - source body (only meaningful after proposal 1)

Retain `scratch/workers/w3c_collision_debug/pF.c` as the zero-credit best body. It is strictly closer than both the
real file (4112/275) and the w1 best (4176/277): size, relocation count, relocation multiset and frame all equal
January. It is also more credible than w1's `n1_X2.c`, because the two shapes that buy the difference are genuine
real_math.h helper calls (`scale_vector3d`, `set_real_point3d`) rather than hand-expanded copies.
Owner census vs `build/base`: `+.bss` the three spray arrays (proposal 1), `+.rdata` the January `count<=14` literal,
`+.text _scale_vector3d` and `+.text _set_real_point3d` (real_math.h select-any surplus COMDATs, both on the A4/A30
accepted systemic census). Not landed by the worker: the brief allows a landing on an already-written residual only
at strict EXACT.

## 6. Do-not-repeat additions

- Any `point_from_line3d` spelling of the phantom point: it is strictly WORSE (all three rows `t`-first), quite apart
  from the lane rule.
- Index-first spray direction products (`pB`) - byte-identical; the (d) x87 order is not a source commutative-order
  question, and an R3 swap would be inadmissible regardless.
- Declaration-order probes for `ignore_object_index` (`pA`) - byte-identical.
- Routing the `collision_debug_vector` copy through `set_real_vector3d` (`pG`) - overshoots, as w1's `X2X7` did.
- Everything on the w1 and 100K do-not-repeat lists still holds: render/write loop index spellings (solved:
  `short row, column` for-loops), the aggregate spray struct, declaration-count probes, the 21-form render-loop
  sweep, the x87 operand-order spellings of `v.i * collision.t` (now SOLVED by A35, so that entry is retired).

## 7. Reopen criteria

1. A mechanism for VC7's frame-slot rank tie-break when two cells have equal `refs/size` (cause b), or a January-side
   reason the `column` IV outranks the hoisted `(real)row` temp (cause c). Then resume from `pF.c`: the phantom block,
   both loop nests, the frame size, the relocation multiset and the size are already correct.
2. A January-side explanation for the IL asymmetry in section 3.2 (three scalar stores for `collision_debug_point`,
   a struct copy for `collision_debug_vector`). `set_real_point3d` reproduces it and is admissible, but an explanation
   would let the body be proposed with confidence rather than as the better-spelled of two equal-byte shapes.
3. Landing proposal 1 (symbols.json) removes cause (a) and lets any future candidate gate strictly without alias
   plumbing.
