# `structure_visibility.obj` — Opus5 100K wave D park re-investigation (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/structures/structure_visibility.c`; target `build/split/source/structures/structure_visibility.obj`.
- Compiler XDK VC7 13.00.9254, repository flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` (unchanged).
- Evidence: January split COFF (instruction streams, relocations, `.rdata` assert literals), HCEA Xbox 360
  reconstructions of the same routines (semantics/names only), `source/physics/bsp3d.c` for the child-index idiom.
  No Ghidra/atlas output exists for this object. No header, config or build change was made or needed.
- Wave rules: all 11 residuals are PARKED/LOCKED. Every experiment was scratch-only
  (`scratch/workersD/structure_visibility/`); the real file is unchanged (`git diff --stat` empty).

## 2. Validated baseline (lane HEAD)

`== exact 10 residual 11 unwritten 0`. Parked residuals (target/base size, relocs; parked base hash):

| Function | T/B size | T/B relocs | base sha16 |
|---|---:|---:|---|
| `_structure_visibility_traverse_subclusters` | 416/416 | 24/24 | 49dc2e10af3b5970 |
| `_structure_visibility_traverse_surface_lists` | 528/528 | 27/27 | 9aeaac4160f423a5 |
| `_planes_intersect_rectangle` | 384/368 | 8/8 | f226643f238a408f |
| `_structure_visibility_build_surfaces_traverse_clusters` | 352/352 | 7/7 | b7e4c3b369bcc5b7 |
| `_structure_visibility_build_surfaces_traverse_leaf` | 480/480 | 27/27 | 55702e72a3c51ba3 |
| `_portal_hull_from_points` | 480/464 | 16/16 | 004a86dfae5e2df2 |
| `_structure_visibility_find_mirror` | 704/688 | 18/18 | 820dc0b28c1c49bf |
| `_structure_visibility_build_surfaces_traverse_node` | 608/624 | 26/26 | 26c73014d2655b88 |
| `_structure_visibility_traverse_cluster` | 912/896 | 53/51 | (parked) |
| `_structure_visibility_build_surfaces` | 480/480 | 18/18 | (parked) |
| `_structure_visibility_compute` | 576/544 | 48/47 | (parked) |

The 2026-09-02 reconstruction ledger classified all 11 as "unclassified VC7 scheduling". This wave shows that
8 of them were source-shape defects provable from January bytes.

## 3. Proposal candidate (not landed; orchestrator park-reopen)

`scratch/workersD/structure_visibility/proposal_reopen.c` gates `== exact 18 residual 3 unwritten 0` with the
`_point_from_line3d` guard passing, all 10 baseline-exact rows still exact, code owners identical to
`build/base`, the only data-owner change being January's own leaf assert literal, and 0 fake-scan leads.
Eight parked functions reach their parked TARGET hashes. `traverse_surface_lists` and `portal_hull_from_points`
keep their parked base hashes (bodies untouched). `traverse_leaf` necessarily drifts (its call to
planes_intersect_rectangle changes ABI) to new base `480/27 f18ce216611c937faf04ad47048498fcb9a4b389ac19c757afa66cc324b428c5`.

### Evidence-backed shapes (each is a January byte fact)

- **planes_intersect_rectangle**: 32-bit returns (`xor eax,eax`, `mov eax,1/2`; a short return emits `mov ax`)
  => `long`, like the exact sibling bounding_rectangles_intersect; branchy final return => `if (accumulated_behind)
  return spanning; return in;`; every product loads the normal first => `plane.n.i * local_bounds.x0`
  (plane3d_distance_to_point convention); January passes `cull_plane_count` on the stack (frame 0x30, its home reused)
  and that private ABI appears only when `accumulated_behind` is declared before the `local_bounds` copy
  (6-permutation census: exact iff that ordering; callee-register-pressure law).
- **traverse_node**: the park body had WRONG assert anchors — January pushes 0x2AB parent_bounds, 0x2AC
  cull_sphere_center, 0x2AD cull_bounds, 0x2AE intersection; `cull_plane_count` and `intersection` parameters are
  reassigned (`mov [ebp+0x28],0`; no child locals, frame 0x20); node-element fetch nested in the dequantize
  arguments; `test eax,eax; js` child test with the node call in the fall-through => `if (!(child & LONG_MIN)) node;
  else if (child != NONE) leaf` (bsp3d.c idiom).
- **traverse_clusters**: `bounding_rectangles_intersect(bounding_box, &subcluster->world_bounds)` (ecx/edx roles,
  HCEA agrees); 16-bit tests of both helper results => short locals, spelled with traverse_node's own
  `plane_intersection = rectangle_intersection ? planes_intersect_rectangle(...) : _intersection_out`; pointer walk
  over the surface list advanced before the index.
- **build_surfaces**: `cmp ax,0xffff; je <node call at end>` => `if (center_location.cluster_index != NONE)
  { in_sphere; return traverse_clusters } return traverse_node`.
- **find_mirror**: frame 0x1834 with the intersection count stored at [ebp-0x1834] directly below the output
  vertices => a third `struct portal_hull`; pinned `bl` FALSE returned at the end => single exit
  `if (render.cluster_index != NONE) {...} return found;`; zero-PVS-word skip at loop bottom with shared
  `cluster_pvs++` => `if (*cluster_pvs) {...} else cluster_index += LONG_BITS`; `test ah,5; jp` => `fabs(..) < 0.0001f`.
- **traverse_cluster**: rendered slot re-read from `rendered_cluster_indices` for `rendered_cluster_get` after
  `indices[ci] = render.rendered_cluster_count++` (+2 relocations); end clear reuses the first BIT_VECTOR word/bit;
  debug colour never initialised and red loaded straight into the call register => scoped
  `if (debug_portals) render(red); else if (ai_debug_highlight_cluster(ci, &color)) render(color);`;
  `cmp word [esi], dx` => `portal->cluster_indices[0] == cluster_index`.
- **compute**: PVS fetched inside the csmemcpy argument list (nested-call interleave); `test bx,bx; cmp bx,0x200`
  => `short cluster_index`; slot re-read (+1 relocation); lazy `lea edi,[esi+0x134]` => no `clusters` local.
- **traverse_subclusters**: same pointer-walk idiom as traverse_clusters; pointer named `surface_index_buffer`
  after January's own assert literal in traverse_surface_lists.

## 4. Experiment matrix (scratch; T/B = target/candidate)

| ID | Function | Shape | Size T/B | Relocs | Result |
|---|---|---|---:|---:|---|
| P1 | planes | `?:` return | 384/368 | 8/8 | sete kept |
| P2 | planes | long return | 384/368 | 8/8 | ABI unchanged |
| P3 | planes | + decl order, if-return, normal-first | 384/384 | 8/8 | EXACT (perm census 3/6 exact) |
| L1 | leaf | assert text, intersection reuse, recomputed bound | 480/480 | 27/27 | 1 insn position |
| L2-L9 | leaf | decl split, MIN spellings, if-MIN, leaf_intersection local, decl hoists | 480-496 | 27 | no better / worse |
| N1 | node | anchors, param reuse, nested fetch, bsp3d child idiom | 608/608 | 26/26 | arm order only |
| N2 | node | `!(child & LONG_MIN)` node first | 608/608 | 26/26 | EXACT |
| N3 | node | `child >= 0` | 608/608 | 26/26 | jl, refuted |
| C1-C3 | clusters | cond placement, arg order, pointer walk, short chain | 352/352 | 7/7 | 29/7/3 blocks |
| C4/C5 | clusters | increment order; node ?: idiom | 352/352 | 7/7 | EXACT (C5 retained) |
| B1 | build_surfaces | inverted NONE guard | 480/480 | 18/18 | EXACT |
| PH1-PH3 | portal_hull_from_points | long, asserts, `facing > 0` guard, ?: pair | 480/480 | 16/16 | 10 blocks |
| PH4-PH9 | portal_hull_from_points | for-loop, ?: order, HCEA walk_step | 480/480+ | 16 | no better; walk_step broke caller |
| M1 | find_mirror | hull struct, single exit, else skip, `<` | 704/704 | 18/18 | EXACT |
| T1-T3 | traverse_cluster | slot re-read; scoped if/else-if colour; compare order | 912/912 | 53/53 | EXACT at T3 |
| K1/K2 | compute | nested pvs, short counter, re-read; no clusters local | 576/576 | 48/48 | EXACT at K2 |
| S1 | subclusters | pointer walk | 416/416 | 24/24 | EXACT |
| SL1-SL4 | surface_lists | nested material fetch, index-first, `*p++` header, `p += 2` | 528/528 | 26-27 | 22 blocks best natural |

## 5. Do-not-repeat

- planes: `(x==0)+1` and `?:` spellings (setcc), short return type (16-bit moves).
- leaf: the remaining `mov ebx,eax` placement is inert to decl order, MIN spelling, separate declarations,
  if-form MIN and intersection-as-temp; `leaf_intersection` local is refuted (496).
- node: `child >= 0` (jl) refuted; `plane3d_distance_to_point` inline also exact but adds a new COMDAT owner.
- portal_hull_from_points: HCEA `walk_step` local refuted (frame 0xc0c, breaks portal_hull_from_portal).
- surface_lists: index-first vertex arithmetic is canonicalised; three `*buffer++` header reads lose a relocation;
  the `buffer += 2; group_end = consumed + *buffer++ + 3` split reproduces +8/+4 but is not natural source.

## 6. Residual classification (proposal candidate)

- `_structure_visibility_build_surfaces_traverse_leaf`: scheduling — one `mov ebx,eax` save emitted before two
  stack-argument pushes (January after). All structure fixed (measured).
- `_portal_hull_from_points` (scratch PH3, not in the proposal): zero-register pinning + register-load scheduling
  (`xor edi,edi` before `cmp winding,1`; winding loaded to edx before `add`). Structure fixed (measured).
- `_structure_visibility_traverse_surface_lists`: register-role tie (esi/edi) + triangle argument load order;
  +8/+4 header advance only reproduced by an unnatural spelling (inference: an unrecovered natural form exists).

## 7. Reopen criteria

- leaf: a lever for callee-saved save placement around stack-argument pushes.
- portal_hull_from_points: a natural source handle for the pinned-zero `?:` materialisation.
- surface_lists: a natural spelling of the split 3-long header advance plus an esi/edi role lever.

## 8. Disposition

No real-file change (all targets parked). Orchestrator proposals: park-reopen for 8 functions from
`proposal_reopen.c` (8 x strict EXACT in its whole-TU gate), re-measure the traverse_leaf park base, keep
surface_lists and portal_hull_from_points parked with unchanged measurements. No credit claimed in this wave.
