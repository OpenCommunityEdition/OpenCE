# `items.obj`: `_item_update` park re-investigation (opus5 150K lane, wave 2)

Date: 2026-09-14. Worker scope: `source/items/items.c` only. No source, header or config change landed;
`git diff --stat -- source/items/items.c` is empty.

## Baseline

| row | January padded | relocs | status |
| --- | ---: | ---: | --- |
| 17 functions | — | — | strict EXACT (unchanged) |
| `_item_update` | 2,352 | 110 | residual `[sha]`, **parked** (`unclassified`) |

Baseline gate: `== exact 17 residual 1 unwritten 0`; `_point_from_line3d` guard passes.
Parked body (real file): 2,352 / 110 / `3daa5a96ffd38c2e...`; target normalized SHA-256 `78237a8528e5d6a8...`.
Aligned diff (alndiff): about 49 differing instructions in seven windows; `relocdiff` 87 differing relocation rows
(addresses shifted by the early windows).

Duplicate prevention: all seven earlier `items_obj*.md` ledgers, the parked.json evidence, the 100K handoff, the
repository history (21 commits) and `branch_sweep` (17 blobs; none better than 17/1/0; no exact body anywhere).

## New evidence: the parked body's carrier unions were the cause of six of seven windows

The parked body models the 0xC8 frame with a `union item_update_scratch` (marker / padded collision result), a
`union item_update_work` (matrix / padded sound location) and an `item_update_storage` struct, plus
`(real_vector3d *)&candidate` casts for the cross-product and fall-vector temporaries.

The January frame census shows a different layout mechanism:

- `[ebp-0x0c]` (12 bytes) is used by five disjoint temporaries: the z-up cross temp, the moving-block candidate
  position, the at-rest fall acceleration, the at-rest support point, and the spin cross temp.
- `[ebp-0x5c..-0x28]` holds both `struct sound_location` (starting at -0x54) and the spin `real_matrix4x3`
  (starting at -0x5c). Both **end at the same top**.
- `[ebp-0xc8..-0x5c]` holds both `struct object_marker` (starting at -0xc8) and `struct collision_result` (starting at
  -0xac). Both **end at the same top**.

Two differently sized objects sharing a top-aligned region is what VC7's packing of **disjoint block-scope locals**
produces, not one carrier union. HCEA `item_update.c` independently describes the marker and collision buffer as one
slot whose live ranges never overlap. The earlier "block-scoped left temp grew the frame to 0xD4" negative is
explained: in that probe `candidate` was still function-scope, so the temp could not share its slot.

The object-rest case in January is `push 0; push eax; mov [esi+0x1b8],eax; call _object_get_node_matrix; lea; push;
lea; push; push eax; call _matrix4x3_inverse_transform_point`. Under the store/call-order law this means:

- the rest-object index is stored **before** the node-matrix call;
- the matrix is a named local, not a nested call argument.

## Shapes (scratch only; the parked real body was never edited)

| # | shape | result |
| --- | --- | --- |
| 1 | natural block scoping, no unions/casts (`collision` in the moving block, `marker` in the at-rest block and separately in the spin block, `sound_location` in the impulse block, `matrix` in the spin else-block, all 12-byte temps block-scoped) | frame 0xC8 kept. The candidate/call-argument interleave, sound-location stores, bounce multiply, rotation-axis copy and cosine load windows all close. 2,384 bytes because of the nested `object_get_node_matrix` arguments. |
| 2 | 1 plus node-matrix locals, with the index stored before the call | **2,352 / 110, 688/688 instructions, 0 differing relocation rows**. Only the angular-dot term order differs (9 instructions). |
| 3 | `dot_product3d(&angular_velocity, &n)` and `(&n, &angular_velocity)` | sha-identical to each other; order unchanged |
| 4 | five explicit association / term-order spellings | sha-identical to shape 2: VC7 canonicalises the sum |
| 5 | zero-store placement (`set_real_vector3d`, after scale, before align); a named dot local before zeroing; a `double` carrier narrowed at the use (the `_item_accelerate` lever); 12 declaration permutations; `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` instead of the local rename plus prototype | all inert or worse |

Diagnostics only, never candidates:

- Removing later uses of the normal, the reflection or the settle dot leaves `k` first.
- 1 to 8 dummy declarations are inert, so declaration count is not the key.
- A file-local inline projection wrapper gives 2,336 bytes, far worse.

A credibility bonus: `-dot_product3d(&collision.plane.n, &velocity) < 0.05f` and
`reflection = -1.4f * dot_product3d(&collision.plane.n, &velocity)` are byte-identical to the parked body's
hand-expanded mixed-order spellings. The candidate uses the natural helper forms.

## Final best candidate

`scratch/workers/items/candidate_item_update.c` is the current real file with the three carrier types removed and
`item_update` replaced.

- `_item_update`: 2,352 / 110, normalized SHA-256
  `8495836d2b150134677a736a706d5f7625dd96eb806b56900137c134d0c849f9`.
- The same 688 instructions, and all 110 relocation rows identical including addresses.
- The only residual is 9 instructions at +0x390. January evaluates the angular-velocity projection dot product as
  `j, k, i` (the `j` term addresses `[esi+0x40]`, then the CSE'd `lea eax,[esi+0x3c]` serves `k` and `i`). VC7 emits
  `k, j, i` for every tested spelling.
- Classification: x87 sum term-order tie.
- Checks: 17 siblings stay EXACT; point guard passes; owner census is identical to `build/base`; fake scan finds 0 leads.

It is **not** strict EXACT, so no park-reopen is proposed.

## Proposal to the orchestrator (park re-baseline)

Replace the locked parked body with the candidate body and re-measure the park `base` to
2352 / 110 / `8495836d2b150134677a736a706d5f7625dd96eb806b56900137c134d0c849f9`.

- It removes the artificial layout unions and representation casts.
- It is behaviour-identical.
- It is strictly closer: about 49 to 9 differing instructions, and 87 to 0 differing relocation rows.
- The only consumer is items.c. The single config text change is the parked.json `base` measurement.

The park evidence should be amended: the "seven scheduling windows" premise is refuted. Six were caused by the carrier
unions and the node-matrix statement order.

## Final real-file gate

`== exact 17 residual 1 unwritten 0`; guard passes; `scratch/parkcheck.py` shows drift 0; the real file is unchanged.

## Reopen criterion

Reopen on either of these:

- a measured VC7 lever that decides the canonical term order of an inlined `dot_product3d` sum (`j,k,i` versus
  `k,j,i` with one pointer-based operand), proven on another object;
- authenticated January source for the angular-velocity projection statement.
