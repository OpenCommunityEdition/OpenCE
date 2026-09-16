# `biped_limp_noodle.obj` second-pass park re-investigation (opus5 250K lane, wave w3c, 2026-09-15)

Worker scope: `source/units/biped_limp_noodle.c` only. All three non-exact rows are parked
(`config/parked.json`), so every body below is scratch-only. **The real file is unchanged**
(`git diff --stat` empty). No exact bytes are claimed.

Scratch: `scratch/workers/w3c_biped_limp_noodle/` (v0, vj1, vp1, vp2, vp3, rw1, rw2, vm6p).
Notes: `scratch/workers/w3c_biped_limp_noodle.md`.

## Baseline and final (identical)

Whole-TU gate with `--forbid-emitted-symbol _point_from_line3d`:

| Function | Target padded / meaningful / relocs | Real file | Status |
| --- | --- | --- | --- |
| `_biped_limp_noodle_get_max_relaxation_iterations` | 16 | exact | EXACT |
| `_biped_limp_noodle_valid_joint_rotation` | 1248 / 1244 / 34 | 1216 / 34 | residual (parked) |
| `_biped_limp_noodle_move_relax_and_constrain_positions` | 1968 / 1955 / 66 | 1888 / 66 | residual (parked) |
| `_validate_real_vector3d_axes3` | 496 | exact | EXACT |
| `_biped_limp_noodle_adjust_orientations` | 704 | exact | EXACT |
| `_biped_limp_noodle_relax_nodes_onto_environment` | 256 / 244 / 9 | 240 / 9 | residual (parked) |

`== exact 3 residual 3 unwritten 0` at baseline and at the end; the `_point_from_line3d` guard passes;
`scratch/parkcheck.py` reports parks 3 / drift 0; `tools/fake_match_scan.py` finds 0 leads on the real file.

Read first: WORKER_BRIEF sections 1-9c (incl. the FINAL 2026-09-15 owner rulings),
`scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, the w1 ledger
`biped_limp_noodle_obj_opus5_150k_w1_20260914.md`, `biped_limp_noodle_codex_checkpoint.md`,
the bipeds w2/w3 ledgers, `scratch/workers/biped_limp_noodle.md`, and the HCEA reconstructions
`.../halocea-review/src/blam/units/biped_limp_noodle_*.c`.

## 1. `_biped_limp_noodle_valid_joint_rotation` - residual narrowed from 5 insns to 3; still OUT-blocked

w1 left the best admissible body (`vj1.c`, component projection) at 1248/34 with a **five**-instruction
difference, and a strict-EXACT body (`vj2.c`) that uses inline `point_from_line3d` and therefore fails the
lane guard. The `_point_from_line3d`-COMDAT ruling is FINAL (brief 9b, laws_w3 C2/R14), so no proposal is
filed here.

### New January decode

The entire residual is the assert's inlined `plane3d_distance_to_point(&plane, &rotate_to_position)`:

* January (T 0x293): `fld [-0xc]; fmul [-0x44]` (j-term, point field first), `fld [-0x10]; fmul [-0x48]`
  (i-term, point first), `faddp`, `fld [-0x40]; fmul [-8]` (k-term, **normal** first), `faddp`,
  `fsub [-0x3c]`. Term order **j, i, k**.
* The *first* `plane3d_distance_to_point` in the same function (T 0x253, applied to `*new_world_position`)
  is **j, k, i** with normal-first in all three terms. So VC7 flattens and re-ranks `dot_product3d`'s sum,
  and January's own two dots rank differently; the only thing that changed is that the second dot's point
  fields are locally created values (laws_w2 A22 / laws_w3 A35(2)).
* `plane3d_from_point_and_normal` is an out-of-line CALL at T 0x24b, so VC7 cannot know `plane.n ==
  plane_normal`; the projection multiplies read `[-0x38/-0x34/-0x30]` = the `plane_normal` copy (re-confirms
  w1 item 5).

### Shapes (3 new, all whole-TU gates of `vj1.c` plus one edit)

| id | shape | law | result |
| --- | --- | --- | --- |
| vp1 | `set_real_point3d(&rotate_to_position, plane_normal.i * projection_distance + new_world_position->x, ...)` | A4 helper routing + A31(b) | 1248/34 `[sha]`; residual **3 instructions**, byte-identical to January's, purely the placement of the k-term (ours `(j+k)+i`, January `(j+i)+k`). Every per-term operand order now matches. |
| vp2 | same call, point-first arguments | A35 leaf age | byte-identical to vp1 |
| vp3 | same call, scalar-first multiply | A22 younger operand | byte-identical to vp1 |

`set_real_point3d` reproduces January's fused `fld n.i; fmul st(1); fadd [esi]; fstp x` compute/store block
exactly (T 0x271-0x28e equal) and fixes the operand orders the component form got wrong. What it cannot fix
is that the call-site expression still reads `plane_normal.i/j/k` **by name**: per A35(1)/(4) a named read
ages that leaf and re-ranks the later inlined dot. The only admissible spelling that reads the vector through
a pointer is `point_from_line3d` (OUT). No other `real_math.h` helper carries the
`result->x = v->i*t + p->x` body, and a `scale_vector3d` + `add_vectors3d` pair would split the fused
multiply-add into six stores (refuted by inspection of the header bodies, not gated).

Best admissible body: `scratch/workers/w3c_biped_limp_noodle/vp1.c` (1248/34, 3 insns). Not landed
(parked, not exact). **No orchestrator proposal** - R14 rejects re-proposing the OUT ruling.

## 2. `_biped_limp_noodle_relax_nodes_onto_environment` - fully decoded allocator tie

Full aligned decode (`alndiff --include-equal`): 103 target instructions including 12 bytes of padding,
93 ours including a 2-instruction (4-byte) loop-top pad - i.e. **the same 91 real instructions** and the
**same frame (0x10)**. The entire residual is which local loses the last register:

* January keeps `biped` in ESI for the whole function. Slots: `[-0x10]` animation_graph, `[-0xc]`
  node_matrices, `[-8]` node_index (`mov dword [ebp-8],0` before the loop, reload + store each trip). The
  loop's five registers are eax/ecx (the two strength-reduced IVs), edx (src copy), edi (dst copy), ebx (temp).
* Ours splits `biped` (store `[-0x10]` at 0x1e, reload at 0xa8), keeps node_index in EDX and uses ESI as the
  loop's src scratch. Slots: `[-0x10]` biped, `[-0xc]` animation_graph, `[-8]` node_matrices.
* In both builds the slot order is reverse creation order, so the frame layout is a consequence of the spill
  set and not an independent lever.
* Ours is the cheaper schedule (two memory operations total versus one plus two per trip), so January took
  the option VC7's own cost model disprefers - consistent with w1's `rF` probe, where re-reading the two
  biped fields at the guard flipped the allocation but emitted an extra compare.

### Shapes (2 new)

| id | shape | law | result |
| --- | --- | --- | --- |
| rw1 | `long node_index = 0;` declared+initialised in the `if (!relaxation_complete)` block, `for (; node_index < animation_graph->nodes.count; node_index++)` | A38 - January's zero store sits between the loop guard's `test ecx,ecx` and its `jle` | byte-identical (240/9) |
| rw2 | bare scope block around the snapshot loop (disjoint lifetime) | laws_w2 A6 recipe; scope blocks are an admitted device | byte-identical (240/9) |

Detector sweep: A23 cannot fire (our loop already has the increment clause, and w1's `while` respelling rD
was byte-identical); A24 cannot fire (frames equal, no parameter home reused for a temp); A28/A29/A31/A33/A35
have no site. laws_w3 A44's "frame gaps that are allocation, not missing locals" is the closest classification.

HCEA (`biped_limp_noodle_relax_nodes_onto_environment.c`, later PowerPC build - names and topology only)
has the same statement set and order but caches `relax_limit` and `node_count` in locals and re-tests the
fields at the guard. January reads each field once (`mov cl,[esi+0x47d]; mov dl,[esi+0x47c]; cmp; sbb; inc`)
and re-reads `animation_graph->nodes.count` inside the loop, so neither cached local is January's. HCEA gives
no new lever.

Ten shapes are now spent across w1 + w3c (rA, rB, rC, rD, rE, rF, rH2, rH3, rw1, rw2). Treat as exhausted.

## 3. `_biped_limp_noodle_move_relax_and_constrain_positions` - admissible body improved 1888 -> 1952

w1 solved the structure (helper-routed push distance, `embedded[0] + embedded[1]`, the endpoint ternary, the
NaN-form range tests, and the enqueue-and-`continue` rejection path that reproduces January's two distinct
enqueue-children copies) but its best body `vm6.c` reaches 1968/66 only with two inline `point_from_line3d`
sites, and still differs by about twelve instructions of first-node-block scheduling.

One new shape, `vm6p.c` = `vm6.c` with all three `point_from_line3d` sites re-routed through
`set_real_point3d`:

* the `_point_from_line3d` guard **passes**, and the row is **1952/66** - an admissible body now lands 16
  bytes short of January instead of the real file's 80;
* the remaining real-code deltas are the w1-documented first-node-block schedule (January stores the gravity
  velocity k, then i and j, then loads the parent index and forms the segment with indexed addressing and a
  late `lea`) plus the 16 bytes the one-pointer helper costs at the endpoint-push site;
* it cannot close: January's apply loop loads the endpoint into **two** registers (`p` and `result`), which
  only a two-pointer helper produces.

Not proposed as a park remeasure (R11 rejects zero-credit remeasures that are not tied to a reopen);
recorded as the starting point for a future wave.

## Proposals

None. The only lever that closes `valid_joint_rotation` is the `_point_from_line3d` COMDAT, whose OUT ruling
is final for this lane (brief 9b; laws_w3 C2 and R14), and no header, config or symbols.json change is
needed by any of the three rows.

## Reopen criteria

* **`valid_joint_rotation`** - unchanged: an owner ruling admitting an inline `point_from_line3d` with no
  emitted COMDAT. `scratch/workers/biped_limp_noodle/vj2.c` is already strict EXACT; reopening it drifts the
  `move_relax_and_constrain_positions` park (1888/66 sha `34b872cd` -> `683135cea08a8197`, the call-site
  register ABI), so both must move in one commit. Do not re-spend j1/j3/j4/j5 or vp1/vp2/vp3.
* **`move_relax_and_constrain_positions`** - (a) an authentic spelling for the first-node-block schedule AND
  (b) either the same owner ruling (then `vm6.c` is the base) or an admissible two-pointer helper spelling.
  Start from `scratch/workers/w3c_biped_limp_noodle/vm6p.c` (1952/66, guard-clean). Do not re-spend m1-m14.
* **`relax_nodes_onto_environment`** - an authentic spelling that raises `biped`'s allocation weight above the
  loop index while still branching on the stored `relaxation_complete` boolean. Do not re-spend rA-rH3, rw1, rw2.
