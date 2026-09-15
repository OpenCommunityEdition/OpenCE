# StructureLensFlares (opus5 250K house-clean lane, wave w3c, 2026-09-15): residual mechanism decoded, still owner-blocked

Worker lane `opus/250k-house-clean-20260915`. **No production source changed.**
`git diff --stat -- source/structures/structure_lens_flares.c` is empty; the final real-file gate equals the baseline.

## 1. Baseline and final (real file, whole-TU gate, `_point_from_line3d` guard passed)

`== exact 3  residual 1  unwritten 0  (of 4 listed)` — unchanged at the end of the wave.

| Row | January (padded / relocs / sha16) | Real file |
| --- | --- | --- |
| `_build_structure_lens_flares` (PARKED, `instruction-scheduling`) | 4336 / 156 / `40aa8e486e52fe25` | 4336 / 156 / `57d6f033f3793735`; 1327/1327 instructions, 13 differing aligned blocks |
| `_cluster_index_from_point` | 96 / 3 | EXACT |
| `_compare_temp_markers` | 32 / 0 | EXACT |
| `_structure_lens_flares_place` | 16 / 2 | EXACT |

## 2. Duplicate prevention

Read: the four canonical Jonas ledgers (grid topology 0826, policy 0829, Fable uniform boundary 0830, authenticated
fuzzy closeout 0901), the w2 ledger `structure_lens_flares_obj_opus5_150k_w2_20260914.md` and its worker notes
`scratch/workers/structure_lens_flares_breakable_surfaces.md`, `scratch/w2/laws_w2.md`, `scratch/w3/laws_w3.md`,
and the memory records (lens-flares 4-instruction witness, mechanism-proven parks, fast_ftol, no-fake-matching).
`git log --all -- source/structures/structure_lens_flares.c`: 21 commits. `branch_sweep.py`: 19 unique blobs,
14 compile-fail, the others 0-2 exact with 2-4 unwritten — **HEAD is the best blob in the corpus**. The Fable census
(ledger 0830) covered 1,562 worktrees and 903 distinct objects with zero strict-exact bodies.

## 3. Residual decomposition (alndiff vs `build/split`, re-measured at this HEAD)

Frame layout is identical to January's: `[ebp-0x34]/-0x30/-0x2c` = `s_temp.n.i/j/k`, `[ebp-0x24]/-0x20/-0x1c` =
`origin.x/y/z`, `[ebp-0x88]/-0x84/-0x80` = `t_plane.n.i/j/k`.

- **N1 @0xa65** — January `fstp [ebp-0x2c]`, then j*j, then a reload `fld [ebp-0x2c]; fmul [ebp-0x2c]`; ours forwards k
  in st(0) (`fst` + `fmul`).
- **N2/N3/N4 @0xb05, @0xb3d, @0xbb0** — every s_temp/s_plane sum (magnitude_squared3d inside `normalize3d`,
  `s_temp.d = dot_product3d(origin, n)`, and both `plane3d_distance_to_point` sites) is **j,k,i** in January and
  **k,j,i** in ours. The t_plane sums are **k,j,i in both** and match; k is the youngest t leaf, so ours is the plain
  laws_w3 A35(2) "youngest term first" order and January's s-sums are the tie-broken ones.
- **derivative @0xdee** — 2-byte `mov edi,edi` vs 3-byte `lea ecx,[ecx]` loop padding, a pure consequence of the 3-byte
  shift from N1; it disappears whenever N is fixed.
- **tie A @0xe28** (`fld [ebp-0x84]` vs `mov [ebp-0x44],eax`, s-loop preheader) and **tie B @0xe6a** (`push ebx` vs
  `fadd [ebp-0x24]`, inside project_point3d's argument pushes) — int-vs-FP interleave transpositions, Fable
  dbg32b-certified DAG-creation-order tie-breaks after ~15k probes.

## 4. New evidence from this wave

1. **The marker-search statement is byte-insensitive to its spelling.** `alndiff base.obj vs f1.obj` (f1 = w2's lf_F1,
   i.e. HEAD with `point_from_line3d(&test_point, &direction, offset, &test_point)` restored) shows the two objects are
   **byte-identical from 0xdf0 to the end of the function**, with identical frame size and identical `[ebp-N]` offsets.
   The genuine inline call and HEAD's hand expansion emit exactly the same local code; the call's whole effect is a
   global IL/DAG tie-break perturbation ~1,000 bytes earlier (N1-N4 and the padding). January's bytes therefore cannot
   discriminate the two spellings locally. This is the "per-basin lottery" the Fable ledger recorded, promoted here from
   a hypothesis to a measurement, and it is the reason no COMDAT-free respelling can be argued from byte evidence.
2. **laws_w3 A35(4) is confirmed for this function** (the law file already names this group N as its example): the hand
   expansion ages `direction`/`test_point` field names and flips the later s-sums.
3. **A31(b) / A35(1) pointer-write routing is refuted here.** `set_real_vector3d(&direction, ...)` — the lever that
   closed breakable_surfaces' cross-product roles and `_lightning_offset_marker_position` — makes this function worse
   (13 -> 16 differing blocks) and adds a candidate-only `_set_real_vector3d` COMDAT that the January object does not
   own (census: `('code','_set_real_vector3d')` absent from target).
4. **Premise correction to the w2 ledger.** w2 recorded the d-only plane form as blocked by "a representation cast
   (house rule)". That is void: laws_w2 section B and laws_w3 section B both ADMIT point-to-vector `(real_vector3d *)`
   casts, and `source/math/real_math.h:1325` (`plane3d_from_point_and_normal`) itself writes
   `dot_product3d((real_vector3d *)point, &plane->n)`. The d-only form is house-rule admissible; it is simply inert on
   its own. The **sole** remaining blocker for group N is the `_point_from_line3d` COMDAT.

## 5. Law detector pass and shapes tried (3 of 5; scratch only, siblings 3/3 exact in every probe)

| Probe | Shape (law) | Result |
| --- | --- | --- |
| `scratch/workers/w3c_structure_lens_flares/p1.c` | marker update as `test_point.x += direction.i * offset;` x3 (natural-spelling control for A35(4)) | 4336/156, 13 blocks; **function bytes identical to base** |
| `.../p2.c` | `set_real_vector3d(&direction, ...)` initialisation (A31(b) + A35(1)) | 4336/156, **16 blocks — worse**; new candidate-only `_set_real_vector3d` owner |
| `.../p3.c` | `s_temp.d = dot_product3d((real_vector3d const *)&origin, &s_temp.n);` alone, no `point_from_line3d` (corrected cast premise, single-plane granularity; w2's F2 tested both planes together) | 4336/156, 13 blocks; **function bytes identical to base** — VC7 fully eliminates the `plane3d_from_point_and_normal` self-copy |

Detectors that did not fire: A12 touches tie A but w2's P* probes already spent the six `s_grid`/`s_count`/`t_distance`
decl-init orders in this basin (no flip); A31(a) does not apply (neither tie site is a post-call stack adjust — tie A is
a preheader integer store against a hoisted x87 invariant, tie B is one push inside a push sequence); A24 is inert (the
frame already equals January's); A23, A25-A30, A32-A34, A36-A45 have no hook (no unwritten row, no size/reloc delta, no
frame gap, no switch/loop-spelling question, no C4013 site in `scratch/gate4/c4013.txt`, no static-linkage candidate, no
`_code_` label, no alias class).

Not repeated (earlier waves): L1, LB1/LB2, L2/L2b, LS2-LS4, the 16-combination `POINT_DOT_VECTOR3D` lattice, F4-F6,
G1-G3, M1/M2, K1-K4, P*, and the Fable ~15k-probe tie search.

## 6. Outcome

**NOT LANDED — OWNER-BLOCKED (group N) + SKIPPED-EXHAUSTED (ties A/B). No proposal.**

Strict EXACT is unreachable without emitting `_point_from_line3d`, which WORKER_BRIEF sections 0 and 9b make a lane
rule and which laws_w3 R14 forbids re-proposing; and even with it the body stays two instruction pairs short (w2's
lf_F3/lf_F7). Brief section 6 allows an already-written residual to land only when it becomes strict EXACT, so no
body — however much closer — is landable here. Deliberately no orchestrator proposal: the w2 ledger already records
this function on the `point_from_line3d` owner-ruling list.

## 7. Reopen criterion (revised; replaces the w2 three-clause criterion)

Reopen only if **both**: (a) the owner lifts the `_point_from_line3d` no-emission rule; and (b) a source lever is found
for the two int-vs-FP transpositions at +0xe28 and +0xe6a. The w2 criterion's third clause ("a cast-free d-only plane
spelling") is **struck**: the cast is admitted policy and the d-only form is byte-inert on its own.

Do **not** spend further waves searching for COMDAT-free respellings of the marker-search update. This wave proved the
statement's local bytes are spelling-insensitive, so no byte evidence can select one, and any COMDAT-free spelling that
happened to win the global tie-break would be an unfalsifiable lottery pick barred by the no-fake-matching gate.

## 8. Verification

- `python -B tools/campaign/gate.py source/structures/structure_lens_flares --all --forbid-emitted-symbol _point_from_line3d`:
  guard passed, `== exact 3 residual 1 unwritten 0` (equals baseline).
- `python -B scratch/parkcheck.py source/structures/structure_lens_flares <base.obj>`: parks 1, drift 0.
- owner census vs `build/base` and `build/split`: candidate-only `[]`, base-only `[]`.
- `python -B tools/fake_match_scan.py source/structures/structure_lens_flares.c`: 0 review leads.
- `git diff --stat -- source/structures/structure_lens_flares.c`: empty.
