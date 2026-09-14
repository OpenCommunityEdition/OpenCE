# path.obj: opus5 100K consolidated lane, wave D park re-investigation (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/ai/path.c`; target object `build/split/source/ai/path.obj`.
- Compiler VC7 13.00.9254 (XDK), unit flags `/nologo /c /O2 /Oy-` (no override).
- Lane `opus/100k-consolidated-20260914`, worker `waveD:path`; scratch
  `scratch/workersD/path/`, per-function notes `scratch/workersD/path.md`.
- Evidence: January split object (instruction streams, relocations), January PDB public
  list `scratch/pdb200-current/pdb_symbols.json`, HCEA reconstructions
  (`research-cache/halocea-full-blobs-20260830/src/blam/ai/*.c`, semantics only).
  No debug-map atlas and no Ghidra decompilation exist for this object in `scratch/`.
- Work was scratch-only for the eight locked parks (brief section 10). The real source
  file is unchanged (`git diff --stat -- source/ai/path.c` empty).

## 2. Validated baseline

Real-file gate at lane HEAD and again at the end:
`== exact 23  residual 8  unwritten 1  (of 32 listed)`, `_point_from_line3d` guard passed,
all eight parks at `measurements.base`, fake-match scan 0 leads, owner census identical to
`build/base` (code 40 / data 62 owners).

| Function | Target size/relocs | Park base size/relocs | Gate tag |
| --- | --- | --- | --- |
| `_build_path_edges_for_surface` | 384 / 13 | 384 / 13 | sha |
| `_closest_point_to_attractor` | 208 / 2 | 208 / 2 | sha |
| `_path_attractor_weight` | 192 / 6 | 192 / 6 | sha |
| `_path_heap_bubble_down` | 560 / 24 | 544 / 24 | size, sha |
| `_path_state_approach_point` | 304 / 13 | 304 / 13 | sha |
| `_path_state_begin` | 592 / 20 | 608 / 20 | size, sha |
| `_path_state_estimated_distance` | 496 / 13 | 528 / 13 | size, sha |
| `_path_state_traverse` | 2096 / 65 | 2096 / 65 | sha |
| `_code_0004c7b0` | 16 / 0 | unwritten | owner-blocked name |

## 3. Park-reopen proposals (scratch candidate, strict EXACT, not landed)

Candidate `scratch/workersD/path/park_reopen_candidate.c` (the real file plus the four body
changes below) gates `== exact 27 residual 4 unwritten 1`; the four rows become EXACT, no
baseline EXACT row changes, the other four parks stay at `measurements.base`, the point
guard passes, owner census is identical to base (40 code / 62 data owners, no size change),
fake scan 0 leads. Each step was also gated cumulatively (v1, v3, v16, v19) with no loss.

1. `_path_heap_bubble_down` (560/24, sha `7a171cedd6da`). January's loop head stores the
   best-so-far triple (node index, heap location, cost) before `child_number = 0` and
   `lea esi,[edi+edi]`, then jumps over 8 alignment bytes. Our initialisers placed the child
   cursor first, reloaded `node_index` (+3 bytes) and lost the jump: the whole 16-byte size
   gap. Fix: declare `child_heap_location`/`child_number` without initialisers and assign
   them after the three `new_*` initialisations (HCEA uses the same statement order).
2. `_path_attractor_weight` (192/6, sha `9785d44ef219`). The only residual was the order of
   the `weight = 0` / `distance = REAL_MAX` initialiser stores. VC7 emits same-block constant
   initialisers in declaration order (measured on this function), so January's byte order is
   its declaration order: `real weight = 0.0f; real distance = REAL_MAX;`.
3. `_path_state_approach_point` (304/13, sha `d5abe183e0db`). January's loop-tail fetch loads
   the parent index with `movsx edi, word [edi+2]` (a short variable definition), ours with
   the compare-style `xor eax,eax; mov ax,[edi+2]`. Fix: walk through the existing index local,
   `node_index = node->parent_node_index; node = path_get_node(state, node_index);`.
4. `_build_path_edges_for_surface` (384/13, sha `3f8e92ea9ce0`). Three January facts:
   `mov edi,[eax+4]` keeps the surface pointer and the loop test re-reads
   `surface->first_edge_index` (no `first_edge_index` local); `movzx edx, byte [ebp-1]` indexes
   `edge_indices[right_surface]` and `sete` after `test al,al` indexes
   `surface_indices[!right_surface]`; the side test precedes the output-edge pointer with
   `cmp ecx(surface_index), edx(field)`, so
   `boolean right_surface = surface_index == collision_edge->surface_indices[1];` is declared
   before `struct path_edge *edge = &edges[edge_count++];`.

The park premise "only instruction selection/register scheduling remain" is refuted for items
1, 3 and 4. For item 2 the premise ("no declaration reorder attempted") is met by compiler
context: the declaration order is what the byte order records.

## 4. Experiment matrix (scratch only)

| # | Function | Shape | Result | Kept |
| --- | --- | --- | --- | --- |
| v1 | bubble_down | best-so-far triple before child cursor init | 560/24 EXACT | proposal |
| v2 | bubble_down | for loop with break | compile error (C89 decl) | no |
| v3 | attractor_weight | weight declared before distance | 192/6 EXACT | proposal |
| v4 | closest_point_to_attractor | `dot_product3d(&segment,&offset)` | bytes identical to base | no |
| v5 | closest_point_to_attractor | `t = dot; t /= magnitude_squared3d` | identical to base | no |
| v6 | closest_point_to_attractor | offset computed before segment | worse | no |
| v7 | estimated_distance | distance3d(entry,end); named attractor distance + MIN; `while (child != NONE && d < 0.8f)` | 496/13 (size closed) | scratch best-1 |
| v8 | estimated_distance | v7 + `result = TRUE` before the direction block | 496/13, registers/slot tie | scratch best |
| v9 | estimated_distance | result = TRUE at top of found block | 496, own [ebp-1] slot, frame 0x18 | no |
| v10 | estimated_distance | uninitialised result, FALSE in else | identical to v8 | no |
| v11 | begin | single exit: overflow error leaves result FALSE | 592/20 (size closed) | scratch |
| v12 | begin | closest stores distance, point, node, cost | node store after z | no |
| v13 | begin | distance, point, cost, node | wrong | no |
| v14 | begin | distance, node, point, cost | only sqrt chain order left | scratch best |
| v15 | begin | `square_root(distance_squared3d(...))` | identical to v14 | no |
| v16 | approach_point | index local drives the re-fetch | 304/13 EXACT | proposal |
| v17 | edges | loop-bound re-read + boolean index | 384/13, 1 block left | step |
| v18 | edges | + declaration order (field == surface_index) | one `cmp` operand left | step |
| v19 | edges | + `surface_index == field` | 384/13 EXACT | proposal |
| v20 | traverse | full decoded reconstruction (traverse_a.c) | 2096/65, 666 insns | no |
| v21 | traverse | new_node->surface_index from the local | wrong CSE | no |
| v22 | traverse | else-if hash tail, `new_node_index = NONE` | 2080/65 | no |
| v23 | traverse | HCEA while-condition probe | identical to v22 | no |
| v24/b0 | traverse | probe loop owns empty-slot and found cases | 2080/65, 658 insns, frame equal | scratch best |
| b1/b2 | traverse | operand spellings (midpoint, dot args) | identical to b0 | no |
| b3 | traverse | dot_product3d for edge length | worse (2112) | no |
| b4 | traverse | point_from_line3d midpoint | identical to b0 | no |
| b5 | traverse | point_from_line3d pinned point | out-of-line `_point_from_line3d` (guard fail) | rejected |

## 5. Do-not-repeat

- `closest_point_to_attractor`: dot argument order, `/=` split, and offset/segment order are
  measured (VC7 canonicalises the first two; the third is worse).
- `path_state_begin`: `square_root(distance_squared3d())` is identical to `distance3d()`; the
  closest-store permutations other than distance/node/point/cost are wrong.
- `path_state_estimated_distance`: result placement at the top of the found block or as an
  else-assignment does not produce January's `[ebp+0x17]` spill.
- `path_state_traverse`: `point_from_line3d` in the pinned-point arm is forbidden (emits the
  COMDAT); operand spellings of the midpoint/dot do not move the j/k rank.

## 6. Residual classification

- `_closest_point_to_attractor`: x87 three-term chain order (numerator and denominator);
  measured fact, no mechanism found. Tie.
- `_path_state_begin`: after v14 only the sqrt chain order differs (January natural x,y,z;
  ours the common z,x,y idiom; only one other natural-order site exists in January objects,
  `bipeds::_biped_accelerate`, itself unsolved). Tie.
- `_path_state_estimated_distance`: after v8 only register/slot allocation differs (January
  spills `result` into the dead `distance_reference` home byte and keeps `path_distance` in
  the node-index spill slot). Tie.
- `_path_state_traverse`: source topology largely recovered (frame size now equal); remaining
  block layout of the hash tail, j/k operand rank of `edge->edge_vector`, and the slot
  permutation that follows. Unresolved structure plus ties.
- `_code_0004c7b0`: not in the January PDB public list (every public path.c function is), so
  it is a static-class TRUE stub between `path_state_reset` and `path_heap_bubble_up`; no
  authentic name. Owner-blocked.

## 7. Reopen criteria

- The four proposals above: orchestrator unpark and land `park_reopen_candidate.c` bodies.
- `closest_point_to_attractor` / `path_state_begin`: a measured source form that emits the
  January chain order in an exact function first.
- `path_state_estimated_distance`: evidence for what makes VC7 spill the constant result.
- `path_state_traverse`: evidence for the hash-tail statement structure that defers the
  found/overflow blocks, and for the edge_vector j/k rank; start from `scratch/workersD/path/frag_b0.c`.
- `_code_0004c7b0`: an authentic name.

## 8. Disposition

Object still active. Real file unchanged: 23 exact / 8 residual / 1 unwritten, zero new exact
credit from this worker. Four park-reopen proposals would take the object to 27 / 4 / 1
(+1,440 padded bytes) once the orchestrator unparks them. No header, config or symbols change.
