# `decals.obj`: Opus5 100K consolidated lane worker (2026-09-14)

## 1. Scope and provenance

- Unit `source/effects/decals.c` -> January `build/split/source/effects/decals.obj`.
- Compiler: XDK 3911 CL 13.00.9254, repository flags via `tools/campaign/gate.py`.
- Lane `opus/100k-consolidated-20260914` (worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`), worker
  `waveB:decals`. The real file carries the orchestrator's uncommitted `_decal_insert` assert-text fix; this worker made no
  change to the real file.
- Evidence used: January COFF (authoritative; full decode of `_decal_new_from_collision`), January debug map atlas
  (`scratch/atlas/decals.obj.txt`, generated with `scratch/atlas_dump.py`), HCEA `decal_new_from_collision.c`,
  `decal_clip_to_surface.c`, `decal_sprite_get_bounds.c` (semantics only), pastudan `src/halo/effects/decals.c`
  (semantics only), punpckhdq `source/physics/bsp3d.h` and `math/real_math.h`. The Ghidra decomp for decals was not
  prepared for this lane and was not used.
- Per-function worker log: `scratch/workers/decals.md`.

## 2. Validated baseline (lane HEAD + orchestrator fix)

`== exact 27 residual 6 unwritten 0 (of 33 listed)`, 6,304 / 15,584 padded code bytes exact; `_point_from_line3d`
guard passes. Final gate is identical (this worker landed nothing).

| function | January size/relocs/sha16 | ours size/relocs/sha16 | first divergence |
|---|---|---|---|
| `_bsp3d_get_plane_from_designator` | 112/1/`9715a442e80abf39` | 112/1/`798bd1fe257e59ef` | +0x27 `jns` vs `jge` |
| `_decal_clip_to_surface` | 1776/59/`d875457fa2f02176` | 1776/59/`2cc51c87ea496852` | prologue `sub esp,0x6c` vs `0x70` |
| `_decal_new_from_collision` | 6176/210/`38ad39f815f1cf52` | 6048/203/`6a5eef8d962383f5` | frame `0x7308` vs `0x72e8` |
| `_decal_sprite_get_bounds` | 384/16/`e77622d865444b0c` | 384/16/`9658324e079e3b4b` | +0x140 negated product |
| `_decal_update` (PARKED) | 400/25/`dcc6a629481fabd9` | 384/26/`1ab3a89f52527a51` | unchanged, no drift |
| `_render_debug_decals` | 432/27/`8fc837f289208891` | same sha | reloc identity only (semantic tier, skipped) |

## 3. Accepted controls

None retained by this worker (no candidate reached strict EXACT in real headers). Nothing changed in production.

## 4. Experiment matrix (all whole-TU gates of complete candidate files under `scratch/workers/decals/`)

### `_bsp3d_get_plane_from_designator`
| shape | result |
|---|---|
| shadow `bsp3d.h` with `if (plane_designator & LONG_MIN)` (punpckhdq spelling), measurement only (`v4.c`) | EXACT `9715a442e80abf39`; decals 28/5/0; only other change `_decal_new_from_collision` (residual) |
| same shadow in items.c, collision_bsp.c, collision_features.c, bipeds.c (inserted before each file's first include that reaches bsp3d.h) | counts unchanged 17/1, 23/7, 20/0, 40/11; only the `_bsp3d_get_plane_from_designator` COMDAT changes (to the January hash) |

### `_decal_sprite_get_bounds`
| shape | result |
|---|---|
| `width_scale * -registration_point.x` | byte-identical to base (inert) |
| `-(registration_point.x * width_scale)` | byte-identical to base (inert) |

### `_decal_new_from_collision`
| candidate | size/relocs/sha16 | differing normalized insns |
|---|---|---|
| base | 6048/203/`6a5eef8d962383f5` | 630 |
| c1: normalize3d inline for tangent/bitangent + correct assert literal | 6048/203 | 626 |
| c2: complete January-decoded body (see section 5 list), real headers | 6160/210/`b0734a197571338f` (reloc multiset identical) | - |
| c3: c2 + shadow bsp3d.h | 6176/210/`08a73f0c31e2142f` | 293 |
| t11: c3 + `point_from_line3d` inline for the projected velocity | 6176/210/`a9305d93469983d1` | 283 (emits forbidden `_point_from_line3d`) |
| t14: t11 + per-vertex `real_point2d texcoord` aggregate | 6176/210/`7aac27abebc794bd`, frame 0x7304 | 287 |
| probes on c3/t11/t14 (declaration orders, reuse-flag init position, `{0.0f}` offset, write-index scope, int-store order, reversed angle compare, local `normal` pointer, ternary assert expansion) | inert or worse | 291-318 |
| measurement-only `volatile` reuse flag (not admissible, never landed) | frame becomes 0x7308 but 306 | confirms the last 4-byte frame cell is the reuse flag's home |

### `_decal_clip_to_surface`
| candidate | result |
|---|---|
| `texture_y` moved into the vertex loop | inert |
| per-vertex `offset` declared at emission-if scope | inert |
| k1: previous/current points in a bare block wrapping the clipping do-while | frame 0x6c, slot set identical, 7 differing normalized insns, sha `bd07cbd66f87a751` |
| k2-k14: tail statement orders, else-scope declaration orders, moving edge_index/edge_iteration/input_points into the block, `output_points = NULL` | 7-11 differing insns, none exact |

## 5. Findings worth keeping (measured, not landed)

`_decal_new_from_collision` January shape, each backed by the January bytes:
1. Assert literal is `### ERROR unsupported projection dimension` (43-byte `??_C@_0CL@PJMHEHFM`); the file's current text is wrong.
2. Tangent and bitangent are normalised with the inline `normalize3d` (`!(_real_epsilon>fabs(m))` encoding); the edge axis
   uses `if (normalize3d(&edge_axis) > 0.0f)`.
3. Snap-to-axis: per-case `set_real_vector3d`, then `add_vectors3d`/`subtract_vectors3d` chosen by
   `dot(n, velocity) > 0` (first pass) and `dot(axis, n) > 0` (degenerate pass); the degenerate pass writes the
   plane-projected velocity into the same axis vector through an inline taking the normal by pointer (point_from_line3d).
4. `local_random_range` / `real_local_random_range` inlines (dead parameter-home stores at `[ebp-0x170/-0x178]`,
   `[-0x164/-0x19c]`, `[-0x168/-0x16c]`, `[-0x174/-0x17c]`).
5. The primary `basis.scale` is never written (only `wrapped_basis.scale = 1.0f`).
6. A sprite-index variable is zeroed in the not-reuse block and used only for the sprite TAG_BLOCK lookup; the bounds call
   and the datum use literal 0.
7. Per-iteration `surface_queue_read_index = 0; deviant_surface_count = 0;` precede `decal_definition_get`.
8. Wrap grouping loop is `while (remaining>0) { bunch=0; for (i=0; !bunch && i<count; i++) ... }`; closest-edge distances
   swap into min/max; closest plane fetched before the closest_* stores; distances not zero-initialised.
9. Normal bounds are a `real_rectangle3d` updated with `MIN(up, x0)` / `MAX(up, x1)`.
10. Emission re-reads `decal_surface_vertex_counts[i]` after the 2650 assert; UV clamps are `PIN`; conversion is
    `fast_ftol((real)floor(...))`; texcoord stores follow the 2696 assert; the clamped UV pair is a `real_point2d`.

`_decal_clip_to_surface`: January's previous/current points share frame bytes with the per-vertex offset, so they are
not declared in the else-block scope that encloses the emission loop.

## 6. Do-not-repeat

- `_decal_sprite_get_bounds`: negated-product respellings (both inert).
- `_decal_new_from_collision`: declaration re-ordering, reuse flag init placement, assert-macro expansion form, local
  normal pointer, `{0.0f}` offset init - all measured inert or worse on the t11/t14 base.
- `_decal_clip_to_surface` on k1: tail statement order and else-scope declaration order permutations.
- Never land `volatile`, a renamed `point_from_line3d_inline`, or a guard-bypassing COMDAT.

## 7. Residual classification

- `_bsp3d_get_plane_from_designator`: source text of a shared header inline (measured fact); closes with the header
  proposal below.
- `_decal_sprite_get_bounds`: x87 NEG/MUL canonicalisation tie (fact: two spellings inert; inference: no source lever).
- `_decal_clip_to_surface`: scope structure fixed in scratch (fact); remaining constant-zero temp in EAX vs ECX at the
  clip-loop preheader plus iteration/edge-index load order (scheduling/register tie, inference).
- `_decal_new_from_collision`: structurally complete in scratch (fact: size, relocation multiset, frame object set);
  remaining register allocation (January keeps a zero in ESI from function entry and velocity in EBX, reuse flag in
  memory). Blocked additionally by the header proposal and by the `_point_from_line3d` emitted-symbol policy.

## 8. Orchestrator proposals

- A. `source/physics/bsp3d.h` `bsp3d_get_plane_from_designator`: `if (plane_designator<0)` -> `if (plane_designator & LONG_MIN)`.
  Blast radius measured on all four other callers (only the COMDAT copy changes, to the January hash).
- B. Policy: January decals inlines `point_from_line3d` in `_decal_new_from_collision`; the lane guard makes that authentic
  source unlandable.
- C. Naming: atlas `00086fe0 exact decals.obj _decal_check` = our exact `_decal_verify_neighbors`.

## 9. Reopen criteria

- `_bsp3d_get_plane_from_designator`: proposal A applied.
- `_decal_new_from_collision`: A and B resolved, plus a source lever for the entry zero-register / velocity-EBX binding;
  start from `scratch/workers/decals/t14.c`.
- `_decal_clip_to_surface`: a source form that materialises the zero before `input_points` is stored; start from
  `scratch/workers/decals/k1.c`.
- `_decal_sprite_get_bounds`: same-compiler donor keeping NEG before MUL on an enregistered x87 local.

## 10. Disposition

No new strict-exact functions from this worker. Real file unchanged by this worker; `_decal_update` park has no drift;
owner census unchanged (only the orchestrator's assert literal owners differ from `build/base`); fake-match scan has the
single pre-existing inline-assembly lead.
