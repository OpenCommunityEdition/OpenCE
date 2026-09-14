# `structures.obj` — Opus5 150K house-clean lane, wave w2 (2026-09-14)

## Outcome

No change to `source/structures/structures.c` (git diff empty). All seven
non-exact rows are parked or policy-blocked, so nothing could be admitted
to the real file directly. The work is scratch-only and produced:

- **4 park-reopen proposals.** Each is strict EXACT in a whole-TU gate of
  "current real file + only that body":
  - `_structure_clusters_in_cone` (480 B)
  - `_structure_render_surface_from_point_and_leaf` (432 B)
  - `_structure_cluster_unmarked` (128 B)
  - `_structure_cluster_mark` (128 B)
  - Combined, the four give 14/17 exact with 0 regressions.
- **1 exact body blocked by the owner policy on `_point_from_line3d`:**
  `_render_debug_fog_planes` (352 B).
- **2 near-exact residuals, documented but not landed:**
  - `_sphere_intersects_cluster_portal` (432 B). It is not parked, but it
    needs `_point_from_line3d` and still has a 3-slot interleave tie.
  - `_structure_get_planar_fog` (448 B). It needs an unused lookup and still
    has one register tie.

| row | baseline gate | best scratch result | disposition |
| --- | --- | --- | --- |
| `_structure_clusters_in_cone` | residual, relocs 25!=26 | **EXACT** `c1.c` | PARK-REOPEN-PROPOSED |
| `_structure_render_surface_from_point_and_leaf` | residual, sha | **EXACT** `s2.c` | PARK-REOPEN-PROPOSED |
| `_structure_cluster_unmarked` | residual, sha | **EXACT** `u1.c` | PARK-REOPEN-PROPOSED (Fable-history disclosure) |
| `_structure_cluster_mark` | residual, sha | **EXACT** `m2.c` | PARK-REOPEN-PROPOSED (Fable-history disclosure) |
| `_render_debug_fog_planes` | residual, size 368!=352 | **EXACT** `f2.c`, but emits `_point_from_line3d` | OWNER-BLOCKED |
| `_sphere_intersects_cluster_portal` | residual, size 416!=432 | 432/11, 6 ops `v5.c`, emits `_point_from_line3d` | NOT-LANDED |
| `_structure_get_planar_fog` | residual, size 416!=448, relocs 14!=15 | 448/15, one register tie `g1.c` | NOT-LANDED |

## Baseline

- Tree: `C:\halo-worktrees\opus5-150k-house-clean-20260914`.
- Real file sha256 prefix: `9632d4fe732053a9`.
- January split `structures.obj` sha256 prefix: `e99923d38269a098`.
- `gate.py source/structures/structures --all --forbid-emitted-symbol _point_from_line3d` reports exact 10, residual 7, unwritten 0. The guard passes.
- `branch_sweep`: no branch blob is better than the current file.
- Parks at baseline:
  - `_structure_render_surface_from_point_and_leaf`
  - `_render_debug_fog_planes`
  - `_structure_cluster_mark`
  - `_structure_cluster_unmarked`
  - `_structure_clusters_in_cone`
  - `_structure_get_planar_fog`

Ledgers read before any work:
- `structures_obj_jonas_cluster_marker_pair_20260828`
- `structures_obj_fable_cluster_pair_intake_20260906`
- `structures_obj_jonas_planar_fog_definition_index_boundary_20260829`
- `structures_obj_jonas_sphere_portal_first_shot_20260830`
- the structures section of `lane_reconciliation_batch5_fuzzy_ui_world_20260909`
- the Opus5 100K handoff

Scratch files, all under `scratch/workers/structures/`:

| file | sha256 prefix |
| --- | --- |
| `c1.c` | `bf9de1a7aa1e9eb1` |
| `s2.c` | `1420630c6a929ef2` |
| `u1.c` | `bc045e0860139d80` |
| `m2.c` | `3be47e8c225b5cbd` |
| `reopen_all.c` (all four reopens) | `4de0e2e69aa2b54d` |
| `f2.c` | `860e8313fab4cef1` |
| `v5.c` | `5d820d45949f9b06` |
| `g1.c` | `90e3f6e07811661f` |

## Park-reopen proposals

Every candidate below passed the same checks:
- a whole-TU gate that keeps all 10 baseline-exact rows exact;
- the `_point_from_line3d` guard;
- `scratch/parkcheck.py`, where the other parks do not drift;
- an owner census identical to `build/base`;
- `tools/fake_match_scan.py`, with 0 leads.

The combined candidate `reopen_all.c` gates 14 exact / 3 residual. The only
park drifts are the four reopened bodies.

### `_structure_clusters_in_cone` (480 B, 26 relocations)

**Change.** A single line. The while condition becomes
`stack_depth > 0 && cluster_count < maximum_cluster_count`.

**January evidence.**
- The loop is rotated. Entry jumps straight to the count test, because
  `stack_depth = 1` is known and that test is folded.
- The bottom of the loop tests `stack_depth` and then re-enters the count test.
- So the depth test comes first in the source condition.
- HCEA `structure_clusters_in_cone.c` also tests depth first:
  `while (stack_depth > 0) { if (found_count >= maximum_cluster_count) break;`.

**What it fixes.**
- The missing `_structure_globals+4` relocation. The inlined
  `structure_cluster_marker_begin` switches from `inc [mem]` to load/inc/store.
- Every register and slot difference.

All of these were downstream effects of the loop lowering.

### `_structure_render_surface_from_point_and_leaf` (432 B, 13 relocations)

**Change 1: result flag (single-exit law).**
- January zeroes a result byte before the loop and returns it on the
  empty-range edge.
- It returns constant FALSE on normal loop exit and constant TRUE on success.
- There is no flag test in the loop condition.
- Source form: `boolean found = FALSE;` then, inside the loop,
  `if (point_in_triangle3d(...)) { *surface_index = ...; found = TRUE; break; }`,
  then `return found;`.
- Rejected shape `s1` put `!found &&` in the loop condition. That adds a
  per-iteration test and a spilled flag.

**Change 2: nested lookup (nested-call argument interleave law).**
- January pushes the node size and the node index before it calls the
  collision-BSP element lookup.
- So the collision-BSP lookup is nested inside the node lookup's argument list.
- There are no `collision_bsp` or `bsp3d_node_index` locals.

### `_structure_cluster_unmarked` (128 B, 11 relocations)

**Change (single-exit law).**
- `boolean unmarked;` is declared at the top.
- After the two asserts: `unmarked = cluster_magic_numbers[cluster_index] != cluster_marker;`.
- Then `return unmarked;`.
- The variable cannot be initialised at its declaration. That would read the
  array before the bounds assert.

**Disclosure.**
- The 2026-09-06 Fable intake rejected a post-measurement cast form and
  asked for a single natural authority.
- Fable's `w_structures_v2.c` tried a result temp, but declared it after the
  asserts. That is not valid C89 in this C TU.
- This proposal rests on the single-exit law, which was established after that
  intake. It does not use the rejected cast or if forms.
- The orchestrator or owner decides whether the law satisfies the intake.

### `_structure_cluster_mark` (128 B, 12 relocations)

**Change.**
`if (magic != marker) { magic = marker; return TRUE; } return FALSE;`

**January evidence.**
- January's block layout: the store+TRUE path falls through, and the FALSE
  epilogue comes last.
- The current equal-first source keeps its own source order when compiled: it
  lays out the FALSE epilogue first. So January's layout indicates the
  not-equal-first order.
- The single-exit result-flag spelling was measured (`mu1`) and is not January:
  it pins `bl` and grows to 144 B. So this function has two returns.
- HCEA's equal-first order comes from a PPC decompiler, not from source order.

**Disclosure.** This is the same body shape that the Fable intake rejected as
post-measurement selection. The new argument is the block-layout evidence plus
the measured refutation of the result flag. Owner or orchestrator decision.

## Owner-blocked: `_render_debug_fog_planes` (352 B, 20 relocations)

**January evidence.**
- The frame is `0x20`: two index spills and two `real_point3d`, with no `offset` slot.
- The `n*t` products are recomputed for the second point, so VC7 did not CSE
  across the two offset points. That is the inline boundary of real_math.h
  `point_from_line3d(p, v, t)`.
- Each component is `fld t; fmul v.i; fadd p.x`.
- The loop is a rotated `for` loop, with the next index computed before the
  first element fetch.
- The negated distance is duplicated three times up front. That happens when
  `-render.fog.planar_maximum_distance` is passed directly to both calls. A
  named local duplicates lazily (shape `f1`, 3 ops off).

**Result.** `f2.c` is strict EXACT. It emits the `_point_from_line3d` COMDAT,
which this lane forbids.

**Why there is no workaround.** Written-out component expressions are CSE'd by
VC7 and cannot reproduce the recomputation.

## Not landed

### `_sphere_intersects_cluster_portal` (not parked)

**January evidence.**
- The shared `fchs` t followed by `fld st0; fmul n.i; fadd p.x` is `point_from_line3d`.
- `movzx ax, al` after `projection_sign_from_vector3d` shows the sign local is
  `short`. A scan of every January caller finds this is the only zero-extension site.
- The tail is `if (convex_hull2d_test_circle(...)) return TRUE;` nested under
  the two rejection tests, followed by one `return FALSE;`.
- The collision plane is passed directly to `plane3d_distance_to_point`, with
  no named local. That fixes the point-first x-term.

**Result.** `v5.c` reaches 432/11 with 6 differing ops. What remains is a
3-slot integer/x87 interleave:
- the `movzx` of the sign,
- the push of the sign,
- the `lea &projected_position`.

Each sits one x87 op earlier in our build than in January's.

**Shapes that did not move the interleave:**
- `word` sign type;
- `real_plane3d *` local;
- top-level declarations;
- `point_from_line3d` nested as an argument.

**Diagnostics.**
- The manual `real t` / `double t` expansions `d9` and `d10` bring back the
  dot-product and magnitude operand-order differences. So the presence of the
  inline changes x87 operand numbering.
- Putting the TU in January's section order was measured as inert (`r0`).

**Blocked by** the `_point_from_line3d` policy, whatever happens with the tie.

### `_structure_get_planar_fog`

**January evidence.**
- A second `global_structure_bsp_get` re-fetch feeds the cluster lookup and
  both fog-plane lookups. The entry `structure` register is destroyed by the
  inlined TRUE-index call.
- The 15th relocation is a fog-plane lookup at the top of the offset block.
  Its result is unused and it comes before the literal-zero load.
- Store order is density, depth, distance (HCEA agrees).
- The `planar_mode` store comes before the first fog-plane lookup call.

**Result.** `g1.c` reaches 448/15. What remains is one ecx/edx allocation
choice in the first fog-plane lookup; the other diffs are literal-owner naming
artifacts.

**Shapes that did not help:**
- `g2`, a block-1 local: identical.
- `g4`, an inner `current_structure` local: identical.
- `g3`, a shared `fog_plane` variable: worse, 464 B.

**Also needs** an owner ruling on unused-lookup locals.

## Orchestrator proposals

1. **Owner ruling: the `_point_from_line3d` COMDAT policy.** Structures adds two bodies:
   - `_render_debug_fog_planes`, 352 B, EXACT;
   - `_sphere_intersects_cluster_portal`, 432 B, near-exact.

   Both require the real_math.h inline. The policy already gates weapons, decals and actor_moving.
2. **Owner ruling: unused-lookup locals.** `_structure_get_planar_fog` needs a
   fog-plane lookup whose call January proves but whose result it never uses.
3. **Optional debt, not needed for exactness.** The TU-local views
   `structure_cluster_graph`, `structure_fog_plane_render` and
   `structure_cluster_portal` duplicate partial shared owners in
   `structure_bsp_definitions.h`:
   - `structure_cluster` lacks `portal_indices`;
   - `structure_fog_plane` lacks `vertices`.

   Completing those owners is a header packet for the coordinator. No text is
   proposed here, because of the declaration-count risk.

## Reopen criteria

| function | reopen when |
| --- | --- |
| cone, surface, unmarked, mark | Orchestrator verification of the scratch candidates above. |
| `_render_debug_fog_planes` | The `_point_from_line3d` ruling admits the COMDAT; then land `fog_v2.txt`. |
| `_sphere_intersects_cluster_portal` | The `_point_from_line3d` ruling, plus a lever for the sign-conversion interleave. |
| `_structure_get_planar_fog` | The unused-lookup ruling, plus a lever for the first fog-plane lookup's ecx/edx tie. |
