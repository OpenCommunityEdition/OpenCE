# glow `_get_particle_world_position`: measured at cdc8ebd3

The tree was HEAD eb33baec, which is cdc8ebd3 plus one ledger-only commit. `git status` was clean before and after. No tracked file was edited. The full probe log is in `../LEDGER.md` (G1-G3, B1).

## What the packet is

This is the wave-2 `scratch/w/glow/production.patch`, rebased:

- **glow.c part:** applies unchanged, because glow.c/.h are untouched since e9e62b78. After the apply, glow.c == `scratch/w/glow/cand_final.c` byte-for-byte (CRLF).
- **parked.json part:** applies unchanged. The glow entry now sits at lines 1105-1115, because other parks were retired.
- **config.json part:** failed, because b9a8d587 flipped `light_volumes.c` two lines above. It was regenerated as a one-line status flip.
- **Other config:** symbols.json needs no change, and neither does semantic_data_matches.json.

## Gates (every January function of the affected object)

The affected object is `glow.obj` only. No header changes, so no other TU is affected.

| Build | gate.py --all --forbid-emitted-symbol _point_from_line3d |
|---|---|
| tip (production), control | exact 24 / residual 1: `_get_particle_world_position [size 2528!=1744, relocs 55!=43, sha]` |
| **packet (glow/new)** | **exact 25 / residual 0 / unwritten 0**, guard passed, `_get_particle_world_position` EXACT 1744 |

- **Warnings:** at the default /W1 there are none. At `/W3 /Zs` there are 0 TU-local warnings in both the packet and tip; the 12 header warnings are the same set in each.
- **objcmp tip.obj vs new.obj:** 56 owner-keyed sections, 1 difference (`_get_particle_world_position` only), and 0 symbol-row differences.

## Strip tests at cdc8ebd3 (lab only)

| Variant | Result | Meaning |
|---|---|---|
| sA: case-1 midpoint assigned to `traversal_time_indices[1]` (knot bug repaired only) | 24/1 `[sha]` | the knot defect is load-bearing |
| sB: `real_vector3d traversal_left_vectors[4] = { 0 };` (left-vector reads defined only) | 24/1 `[size 1776!=1744, sha]` | the left-vector defect is load-bearing |
| sC: `index < 4` instead of `NUMBEROF(traversal_time_indices)` | 25/25 | byte-inert; this is review2's nit |
| review2 s1_nocasts: every point/vector view cast removed | 25/25, with C4133 incompatible-type warnings | the casts are byte-inert type correctness, not decoration |

Defined-C route, from earlier waves (not re-probed):

- n3a v3 is still +592 B at January's frame and relocations.
- wave-2 P8 `cand_fixed.c` is 2544/55 with 71 REAL regions. review2 rejected it for its y-base z propagation.

## Section-4 admission audit

- **object_audit (official, build/split):** PASS. 36 January symbols, 0 differ; every January .text/.rdata section is ok.
- **Candidate-only surplus:** identical to production's list.
  - Code COMDATs: `_cross_product3d`, `_distance3d`, `_distance_squared3d`, `_magnitude3d`, `_magnitude_squared3d`, `_normalize3d`, `_real_local_random_range`, `_scale_vector3d`, `_square_root`, `_vector_from_points3d`.
  - Also: the real_math.h assert string, nine `__real@` constants and `.drectve`.
- **surplus_id_s:** 20 candidate-only external sections, 0 not identical/unprovided. `_real_local_random_range` matches bored_camera's selected copy.
- **provider_link new.obj:** 20 rows; **SELECTED-PROVIDER LINK: PASS** in both orders.
- **nodup_census:** 0 NODUP definers for the 10 helpers.
- **pdb_storage** (tree, and the candidate via SPLIT_ROOT variant): 36 split symbols, 0 disagreements. `_get_particle_world_position` is absent from cachebeta publics, and the candidate keeps it `static`.
- **Data:** `.rdata` is 392/392 (100%), with no `.data` or `.bss`. No data entry is needed.
- **Other config:** no glow entry exists in `object_admission_rejections.json`, `semantic_matches.json`, `semantic_credit_rejections.json`, `semantic_data_matches.json` or `symbol_ownership.json`.

## Emulated admission chain (`emu_chain.py`, objdiff-cli 3.3.1)

Controls:
- The mini row for build/split + build/base equals the production row. **True.**
- A fresh semantic audit equals the `build/semantic_report.json` summary. **True.**

| | Halo complete units | complete_code | complete_data | matched_code | matched_functions | parks | revoked | admission |
|---|---|---|---|---|---|---|---|---|
| BASE (tree) | 379 | 890,841 | 714,870 | 1,573,109 | 7,442 | 86/0/0 | [] | 11/0/2/0 |
| EMU glow (01-03) | **380** | 897,828 (+6,987) | 715,262 (+392) | 1,574,838 (+1,729) | 7,443 (+1) | 85/0/0 | [] | 11/0/2/0, 0 contradicted |
| EMU glow + king | **381** | 902,693 (+11,852) | 716,138 (+1,268) | 1,574,838 | 7,443 | 85/0/0 | [] | 11/0/2/0 |
| EMU 01 only (park kept, NonMatching) | - | - | - | - | - | **FAILS**: "parked-function evidence is stale or invalid" | - | - |

The unit row after the change reads 6,987/6,987 code, 25/25 functions and 392/392 data, with `complete` true.

## Patches (`patches/`, see `APPLY_ORDER.txt`)

- `git apply --check` passes on each patch. It also passes on 01-03 combined, and on king 01-05 + glow 01-03 combined; the two config.json hunks work in either order.
- `--whitespace=error-all` is clean.
- Trial-applying onto tip copies reproduces `glow/new` byte-for-byte (CRLF), and the JSON parses.

## Objects that become COMPLETE if the owner says yes

- `source/objects/widgets/glow`: 25/25 functions, 392/392 data, 36/36 symbols. Halo objects 379 -> 380; 381 together with king.
  - New strict function: `_get_particle_world_position`, 1,744 padded bytes (1,729 objdiff bytes).
  - One park retires (86 -> 85).

No other object depends on this ruling.
