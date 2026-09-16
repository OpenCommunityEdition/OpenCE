# `biped_limp_noodle.obj` structural park pass (opus5 next-150K lane, wave n3a, 2026-09-15)

Worker scope: `source/units/biped_limp_noodle.c` only. All three non-exact rows are parked
(`config/parked.json`), so every body referenced below is scratch-only. **The real file is unchanged**
(`git diff --stat -- source/units/biped_limp_noodle.c` is empty). No exact bytes are claimed and no
orchestrator proposal is filed.

Notes: `scratch/workers/n3_biped_limp_noodle.md`. Scratch: `scratch/workers/n3_biped_limp_noodle/`
(`base.obj`, `vp1r.obj`, `vm6pr.obj`, `vp1_full.txt`, `vm6p_diff.txt`).

Read first: `scratch/opus5-next150/WORKER_BRIEF.md` (incl. the FINAL 2026-09-15 owner rulings),
`scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`, and every ledger for this object:
`biped_limp_noodle_obj_opus5_250k_w3c_20260915.md`, `biped_limp_noodle_obj_opus5_150k_w1_20260914.md`,
`biped_limp_noodle_codex_checkpoint.md`, `bipeds_obj_jonas_limp_flight_pair_20260831.md`,
plus `scratch/workers/biped_limp_noodle.md` and `scratch/workers/w3c_biped_limp_noodle.md`.

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
`branch_sweep.py` (8 unique blobs) shows the current blob `706055aa` is the best in history — no donor.

## 1. Mandatory wave-n3 census

The wave-n3 method asks whether a fresh census refutes each park's recorded premise (a `[size]`/`[relocs]`
gap that the park text calls a compiler tie but that is really a source defect: a missing statement whose
relocation January owns, a hand-expanded initializer, a one-use alias, a wrong declared width, or wrong
control flow). The census was run before any shape.

### 1.1 Relocation census by target (`relocdiff --count-by-target --allow-structural`)

**All three rows carry the identical relocation multiset to January**, target for target and (for
`relax_nodes`) in the identical order; only addresses shift. The rows `relocdiff` prints as differing in
the summary tables are the csplit representation of the same literal (`symbol:__real@3f800000` versus
`defined-noncode:.rdata:__real@3f800000`, whose `symbolic=` form resolves equal) — see
`csplit-relocation-alias` / laws_w3 A45 for the class.

* `relax_nodes_onto_environment` 9/9; `valid_joint_rotation` 34/34; `move_relax_and_constrain_positions` 66/66.

**No call and no global is missing from any of the three bodies.** The "a missing relocation is a missing
statement" route is closed for this object.

### 1.2 Frame census

* `relax_nodes_onto_environment`: January `sub esp,0x10`, ours `sub esp,0x10`. Four homed cells each side.
  January: `[-0x10]` animation_graph, `[-0xc]` node_matrices, `[-8]` node_index, `[-1]` relaxation_complete.
  Ours: `[-0x10]` biped, `[-0xc]` animation_graph, `[-8]` node_matrices, `[-1]` relaxation_complete.
  Neither side lacks or adds a slot; the *set* of homed values differs by exactly one swap
  (January spills `node_index` and keeps `biped` in ESI; ours does the reverse).
  `alndiff --include-equal`: 103 target instructions including 12 tail-padding bytes versus 93 ours
  including a 4-byte loop-top pad — **the same 91 real instructions, the same control flow, the same widths.**
* `valid_joint_rotation`: January `sub esp,0xe4`; the real file `sub esp,0xcc` (the park's recorded first
  divergence); the w3c scratch body `vp1.c` `sub esp,0xe4`, 431/431 instructions, **size 1248** — re-gated
  this wave as `residual 1248 ... [sha]`, i.e. the `[size]` tag is gone.
* `move_relax_and_constrain_positions`: the w3c scratch body `vm6p.c` reproduces January's frame
  **slot for slot with identical per-slot reference counts** (34 distinct `[ebp±N]` cells, including the
  `[ebp-0x20]` integer-conversion temporary and the `[ebp-0x27]` byte the park text names). The real file
  differs at −0x48/−0x44/−0x40/−0x32/−0x28/−0x27/−0x24/−0x23/−0x20. Instruction counts: target 631,
  `vm6p` 619, real file 598. Full table in the worker notes §0.3.

### 1.3 Verdict

* `relax_nodes_onto_environment` — the park premise is **CONFIRMED**, not refuted: same frame, same
  relocations, same instruction count, same widths, same control flow; the only difference is which of
  `biped` / `node_index` loses the last callee-saved register.
* `valid_joint_rotation` and `move_relax_and_constrain_positions` — the **recorded** premises (a frame gap
  at `+0x03`; a `[ebp-0x20]` vs `[ebp-0x1C]` homing choice) are **stale rather than newly refuted**: w1 and
  w3c already found the source defects behind them and fixed them in scratch. The remaining blockers are
  not structural (below).

**No wave-n3 "hidden source defect" was found.** The structural work on this object was already done; the
residuals are a FINAL-OUT helper spelling and one allocation tie.

## 2. `_biped_limp_noodle_valid_joint_rotation` — three instructions, prohibited-helper-blocked

`alndiff` of `scratch/workers/w3c_biped_limp_noodle/vp1.obj` against `build/split` (re-run this wave) shows
the candidate is **instruction-for-instruction identical to January at every address except one
3-instruction block**. The entire residual is the association of the inlined `dot_product3d` inside the
assert's `plane3d_distance_to_point(&plane, &rotate_to_position)`:

* January (T 0x293): `fld [-0xc]; fmul [-0x44]` (j-term, point field first), `fld [-0x10]; fmul [-0x48]`
  (i-term, point first), `faddp`, `fld [-0x40]; fmul [-8]` (k-term, **normal** first), `faddp`,
  `fsub [-0x3c]` — the source association `(i*bi + j*bj) + k*bk`, with the leaves of the first add swapped.
* Ours emits the same three products with the **same per-term operand orders** but associates `(j + k) + i`.
* The preceding `set_real_point3d` compute/store block (T 0x271-0x291,
  `fld [-0x38]; fmul st(1); fadd [esi]; fstp [-0x10]` and its y/z copies) is byte-equal, so
  `rotate_to_position` is produced exactly as January produces it.

The term rank is therefore set by the leaf ages of `plane_normal.i/j/k`, which every admissible call-site
spelling reads **by name** (laws_w3 A35(1)/(4)). I enumerated every `__inline` helper in `source/math/real_math.h`:
`point_from_line3d` (line 1077) is the only one whose body is `result->x = v->i*t + p->x`; a
`scale_vector3d` + `add_vectors3d` pair splits the fused multiply-add into six stores. `vj2.c` is already
strict EXACT with the inline helper and fails the lane guard, and the `_point_from_line3d` COMDAT ruling is
FINAL (brief §1; laws_w3 C2 and R14, which also rejects re-proposing it).

Shapes spent to date: w1 j1/j2/j3/j4/j5 and w3c vp1/vp2/vp3 (nine, including the strict-exact OUT body);
the post-tie 2-shape budget is spent. **No shape was spent this wave** — the triage result is SKIPPED-EXHAUSTED.

## 3. `_biped_limp_noodle_move_relax_and_constrain_positions` — 31 real blocks, two of them OUT-shaped

`alndiff` of `vm6p.obj` against `build/split` gives 76 blocks, of which **31 are real** after filtering the
csplit representation rows. They fall into four families:

1. **First-node block (T 0x1a0-0x1e8).** January forms the segment with full indexed addressing
   (`fld [edi+esi+0x28]; fsub [eax+esi+0x28]`, then the y pair) and only afterwards emits the two
   `lea [edi+esi+0x28]` / `lea [eax+esi+0x28]` pointer forms; ours emits the `lea`s first and reads
   `[ebx]`/`[edi+4]`. This is the accessor-binding split (January uses both forms, per site). w1 tried the
   reorder as m7/m8/m9/m10/m14 — all byte-identical to m6.
2. **Two squared-term hoists (T 0x356, T 0x43f).** Ours hoists `fld [-0x78]; fmul [-0x78]` above the
   neighbouring `xor ecx,ecx`; January emits it after. Pure schedule; the differing 2- versus 3-byte
   alignment pad (`mov edi,edi` versus `lea ebx,[ebx]`) follows from it.
3. **Endpoint loop (T 0x481-0x4d3).** January colours the counter EDX and the endpoint pointer ECX (ours
   ECX/EAX) and its layout places the `mov [ebp+ecx*4-0x38],edx; jmp` block between the two ternary arms
   where ours places it at the tail. Register colouring plus block placement — laws_w3 A43 records that no
   source lever moves which duplicate tail survives.
4. **Apply loop (T 0x632-0x67c).** January holds the scale in `st` across the three components
   (`fld [-0x10]; fmul st(1); … fstp st(0)`) and, after `collision_move_point`, loads the **vector component
   first** in each product (`fld [-0x18]; fmul [-4]`), where ours loads the scalar first and re-reads it from
   memory. That is the two-pointer `point_from_line3d` body; `vm6.c`, which uses it, reaches January's 1968.

Family 4 occurs at two sites (ray origin and endpoint push), so this row is unreachable in this lane
whatever happens to families 1-3.

`vm6p.c` re-gated this wave: **1952/66** (real file 1888), with `_valid_joint_rotation` simultaneously at
1248/34 `[sha]` and all three sibling rows EXACT, guard clean. `parkcheck` on that object reports
`DRIFT valid_joint_rotation 1216 -> 1248` and `DRIFT move_relax 1888 -> 1952`, so it cannot land even as a
remeasure (R11 rejects zero-credit park remeasures that are not tied to a reopen). Fifteen shapes are spent
(m1-m14 plus vm6p); **none this wave**.

## 4. `_biped_limp_noodle_relax_nodes_onto_environment` — allocation tie, confirmed by the census

Re-derived from `alndiff --include-equal` this wave: the guard is `cmp dl,cl; sbb cl,cl; inc cl`, i.e.
`relaxation_complete = biped->biped.limp_body_current_relaxation_iterations >=
biped->biped.limp_body_max_relaxation_iterations` (`[esi+0x47c]` / `[esi+0x47d]`), stored to `[ebp-1]` and
returned on both exits — identical on both sides. The snapshot loop uses the same two strength-reduced IVs
(`add eax,0x34`, `add ecx,0xc`) and the same two fresh pointer copies per trip (`mov edx,eax` / `mov edi,ecx`
in January, `mov esi,eax` / `mov edi,ecx` in ours), i.e. the same struct-copy source shape. The whole
residual is that January's loop scratch is EDX so ESI can carry `biped` across both calls, while ours takes
ESI for the loop scratch and spills `biped` to `[ebp-0x10]`. Ours is the cheaper schedule (one store plus one
reload versus a per-trip reload and store of `node_index`), so January took the option VC7's own cost model
disprefers — consistent with w1's `rF` probe, which flipped the allocation by re-reading the two biped fields
at the guard but emitted an extra compare and is therefore not January's source.

Detector sweep this wave: A23 cannot fire (our loop already carries its increment clause; w1's `while`
respelling rD was byte-identical); A24 cannot fire (frames equal, no parameter home reused for a temp);
A28/A31/A33/A35/A38/A42 have no site. laws_w3 A44 ("frame gaps that are allocation, not missing locals") is
the classification. Ten shapes spent (rA, rB, rC, rD, rE, rF, rH2, rH3, rw1, rw2); **none this wave**.

## Proposals

None. The only lever that closes `valid_joint_rotation` and `move_relax_and_constrain_positions` is the
`_point_from_line3d` COMDAT, whose OUT ruling is FINAL for this lane (brief §1; laws_w3 C2 and R14), and no
header, config or `symbols.json` change is needed by any of the three rows.

## Reopen criteria (all unchanged from w3c)

* **`valid_joint_rotation`** — an owner ruling admitting an inline `point_from_line3d` that emits no COMDAT.
  `scratch/workers/biped_limp_noodle/vj2.c` is already strict EXACT; reopening it drifts the
  `move_relax_and_constrain_positions` park (1888/66 `34b872cd` -> `683135cea08a8197`, the call-site register
  ABI), so both must move in one commit. Do not re-spend j1/j3/j4/j5 or vp1/vp2/vp3.
* **`move_relax_and_constrain_positions`** — (a) an authentic spelling for the first-node-block schedule AND
  (b) either the same owner ruling (then `vm6.c` is the base) or an admissible two-pointer helper spelling.
  Start from `scratch/workers/w3c_biped_limp_noodle/vm6p.c` (1952/66, guard-clean). Do not re-spend m1-m14.
* **`relax_nodes_onto_environment`** — an authentic spelling that raises `biped`'s allocation weight above
  the loop index while still branching on the stored `relaxation_complete` boolean. Do not re-spend
  rA-rH3, rw1, rw2.
