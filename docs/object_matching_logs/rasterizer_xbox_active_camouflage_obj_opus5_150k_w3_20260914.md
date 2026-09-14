# `rasterizer_xbox_active_camouflage.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## Baseline and outcome

| item | value |
| --- | --- |
| unit | `source/rasterizer/xbox/rasterizer_xbox_active_camouflage` |
| baseline gate | 11 exact / 1 residual / 0 unwritten; guard passes; no parks |
| non-exact row | `_rasterizer_active_camouflage_draw` 2016 / 142 [sha] |
| inherited (not touched) | `_real_alpha_to_pixel32` |
| final real-file gate | 11 exact / 1 residual / 0 unwritten (real file **unchanged**) |
| newly exact | none |

## Duplicate prevention

Read `_jonas_visibility_20260828`, `_jonas_real_alpha_boundary_20260830`,
`_jonas_credibility_recovery_20260830`, `_shared_owner_cache_20260906` and `_opus5_pdb_ida_20260914`.
The pdb_ida do-not-repeat list covers about 60 donor variants: all 24 `actcamo_pc_*` store permutations,
`w13camo_*`, and `acamo_A*/B*/C*`, including ternary/if constant spellings. Also read
`_opus5_150k_w2_20260914` (a1-a9, b1, b2; best structure `a9.c`), the w2 worker notes and
`scratch/w2/laws_w2.md`. The donor scratch listing in `opus-30k-rasterizer-widgets-20260905` was re-checked.

## Re-measurement and law detector

`scratch/workers/w3_rasterizer_xbox_lights_rasterizer_xbox_active_camouflage/c0.c` is w2 `a9.c`. It measures
2016/142 [sha], 542/542 instructions, frame `0x108`. It keeps both residual ties:

- **Tie B.** In arm 2, the zero stores `vertex_constants[1].i..l` and `[2].l` interleave with
  `faddp` / `fmul [intensity]` / `fstp`. The relative order of the integer stores is identical (`[0].k [0].l [1].i [1].j [1].k [1].l [2].j [2].k [2].l`), and so is the order of the x87 ops. Only the merge interleave differs: January emits `[1].i` before the first `faddp`.
- **Tie C.** January evaluates the `active_camouflage_flags` test (`and cl,bl; neg cl`) and then flushes the
  `rgb_outputs[0]`, `rgb_inputs[1]` and `rgb_outputs[1]` constant stores. It materialises `rgb_inputs[0]` with
  `sbb/and/add` only after them. The prior ledger shows C matched in the donor shadow-struct TU with identical
  statement text.

Every law in laws_w2 A1-A22, plus the w3 leaf-age extension measured on rasterizer_xbox_lights, was run as
a detector. Nothing applies:

- no commutative memory x memory operand-order delta;
- no frame, size or relocation delta;
- no store-order or declaration-order delta;
- no exit, helper-routing or C4013 site.

Both residuals are merge-interleave scheduling ties. No new shapes were spent on the exhausted permutation
families.

## Checks

The real file is unchanged (`git diff --stat` empty). Gate: 11/1/0, guard passes. The owner census equals
base, and `fake_match_scan` reports 0 leads.

## Reopen criterion

Unchanged from w2. Start from the a9 structure, then either:

- make a shared-header change that flips tie C; or
- find authenticated evidence for the arm-2 constant-row spelling that moves tie B.
