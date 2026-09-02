# `lightning.obj` small-families wave (2026-09-01)

## Scope and provenance

- unit: `source/objects/widgets/lightning.c`; target
  `build/split/source/objects/widgets/lightning.obj` (January 2002 split,
  byte authority); worktree `C:\halo-worktrees\fable-small-families-20260901`,
  branch `fable/small-families-20260901`.
- compiler: XDK 3911 `xbox\bin\vc7\CL.Exe` 13.00.9254.1 with the build.ninja
  flags, driven only through `tools/campaign/gate.py`.
- evidence: January disassembly/relocations (`gate.py --disas`,
  `scratch/xrefs.py`, `scratch/alndiff.py`, `scratch/fnsbs.py`), the assert
  strings owned by the object (`c:\halo\SOURCE\objects\widgets\lightning.c`
  lines 50/116/117/118/239/280/383/408/426), HCEA reconstruction cache
  `halocea/src/blam/objects/widgets/lightning_submit.c` and
  `lightning_offset_marker_position.c` plus the `lightning_*` headers
  (semantics, names, local vocabulary only), HCEX.pdb via DIA2Dump
  (`lightning_submit` locals `marker`, `bitmap`, `points[0x1001]`,
  `centroid`, `bounds`, `up`; helper is `static` with params
  `position`, `matrix`, `random_position_bounds` and local `offset`),
  `render_contrails.c` (exact sibling, style donor for the strip emission),
  and the two prior ledgers `lightning_obj_jonas_offset_helper_fixed_point_20260826.md`
  and `lightning_obj_jonas_complete_donor_one_shot_20260828.md`.
- this wave resumed a stopped worker whose edits were already on disk
  (log `scratch/workers/lightning.log`, shapes S1-S5).

## Validated baseline

Original campaign baseline (board.py/gate.py before the first worker):
**7/9 exact, 0 residual, 2 UNWRITTEN** (`_lightning_offset_marker_position`
240 B, `_lightning_submit` 2576 B); 256/3072 bytes.

Resumed on-disk state (previous worker's S5): 7 exact, 2 residual, 0
unwritten; `_lightning_submit` size 2576 = target, relocs 80 = target,
`[sha]` only; helper 240/22 relocs `[sha]` only. All seven baseline-exact
functions verified exact at every gate in this wave.

The target symbol for the helper is already named
`_lightning_offset_marker_position` in the split (no `--alias` needed).

## Accepted source shapes (retained, all measured one factor at a time)

| id | shape | evidence |
| --- | --- | --- |
| S6 | `point_count = 0;` before `csmemset(points, 0, sizeof(points));` in the first-marker block | target stores `[ebp-0x14],0` between the csmemset pushes and the call (0x14f) |
| S8c | second strip vertex: `width = -width;` then `position->x + up.i*width` (contrails `texture_u_step = -texture_u_step;` idiom) | target `fld width; fchs` kept on the x87 stack for three `fmul st(1)` (0x850-0x874); `up.i*-width` was CSE'd into `fsub` and perturbed 16 spill slots |
| S9 | strip locals initialised `thickness_scale`, `tint`, `brightness_scale` | target store order 0x618/0x622/0x625 |
| S7 | `short segment_point_index_increment = 2*segment_point_start_index;` declared before the line-408 assert, used as the for-increment | target computes `lea eax,[edi+edi]; mov [ebp-0x44],eax` between `test di,di` and `jg` (0x3cc), i.e. before the assert, while the other invariant `(real)segment_point_count` is LICM'd into the preheader |
| S13b | `real segment_fraction = (real)segment_point_index/segment_point_count;` named before `jitter_offset` | restores the target's `fild; fdiv; fld; fsub; fmulp` order that S7 alone flipped |
| S11b | vertex field order `point`, `texture.x`, `color`, `texture.y` | target stores color (`mov [esi-4],eax`) before texture.y in both vertices |

## Experiment matrix (`_lightning_submit`, size/relocs always 2576/80)

| id | factor | hunks (alndiff) | structural hunks | decision |
| --- | --- | --- | --- | --- |
| S5 (inherited) | previous worker's final shape | 99 | 22 | baseline of this wave |
| S6 | point_count=0 before csmemset | 97 | 20 | retain |
| S7 | named increment before assert | 101 | 26 | retain only with S13b |
| S9 | tint init order | 98 | 20 | retain |
| S8c | `width = -width` | 28 | 12 | retain (fixes slot permutation) |
| S10a | S6+S8c+S9 | 24 | 8 | - |
| S10b | S10a+S7 | 28 | 12 | x87 order of jitter product flipped |
| S11a | S10a + order point,color,tex.x,tex.y | 21 | 5 | vertex-2 color store too early |
| S11b | S10a + order point,tex.x,color,tex.y | 21 | 5 | retain |
| S12a | S10b with `long` increment | 141 | 44 | frame shrinks to 0x24140 - reject |
| S12c | S10b with `<<1` spelling | 28 | 12 | identical to S10b |
| S13c | S10b, increment declared after `segment_point_index` | 28 | 12 | inert |
| S13b | S10b + named `segment_fraction` | 21 | 5 | retain |
| S14 | S13b + S11b (installed) | 18 | 2 | **retained**: every frame slot matches; only `mov ecx,[ebp-8]` vs `fstp [esi+0xc]` transposed at 0x876/0x879 |

"hunks" counts alndiff hunks including the 16 `.rdata`/`__real@` annotation
artifacts that never change.

## Residual classification

- `_lightning_submit` (2576/80 exact size and relocations): after S14 the
  only divergence is a dependency-free transposition of the reload
  `mov ecx,[ebp-8]` (vertex_index for the bounds test) and the
  `fstp [esi+0xc]` texture.x store of the second vertex. Class D
  scheduler tie unless a store-order permutation moves it (S11c/S11d/S11e
  below).
- `_lightning_offset_marker_position` (240/22 exact): one transposition,
  `fadd [esi]` before `add esp,0x18` in the target, after in ours (0xd0).
  The 2026-08-26 ledger's other swap (`push ebx` vs the first `fmul`) is
  already gone in the inherited shape. Do-not-repeat list of that ledger
  respected; no new provenance for the remaining swap.

## Do not repeat

- `long` for the increment local (frame size changes).
- Declaration order of the increment local (inert).
- `<<1` vs `2*` spelling (identical).
- `up.i*-width` (CSE generator, wrong slots).
- Everything in the 2026-08-26 helper ledger's do-not-repeat list.

## Name provenance

| target symbol | source name | provenance | evidence |
| --- | --- | --- | --- |
| `_lightning_submit` | `lightning_submit` | unchanged (already named in split and header) | - |
| `_lightning_offset_marker_position` | `lightning_offset_marker_position` (static) | unchanged (split already names it; HCEX PDB confirms `static`) | DIA2Dump `static void lightning_offset_marker_position(...)` |
| `_lightning_globals` | `lightning_globals` | unchanged | - |

Local names: `segment_point_index_increment` and `segment_fraction` are
inferred (no January or HCEA local record); all other locals follow the
January assert text (`point_count`, `segment_point_count`,
`segment_point_start_index`, `segment_point_index`, `vertex_index`,
`vertices`) or the HCEX PDB (`marker`, `bitmap`, `points`, `centroid`,
`bounds`, `up`, `offset`).

## Disposition

In progress - see the end of the file for the final state.
