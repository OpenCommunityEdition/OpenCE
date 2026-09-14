# StructureLensFlares (opus5 150K house-clean lane, wave w2, 2026-09-14): residual re-attributed, still blocked

Worker lane `opus/150k-house-clean-20260914` (HEAD `ea9175577`). **No production source changed.**
`git diff --stat -- source/structures/structure_lens_flares.c` is empty. The final real-file gate matches the baseline.

## 1. Baseline (real file, whole-TU gate, `_point_from_line3d` guard passed)

`== exact 3  residual 1  unwritten 0  (of 4 listed)`.

| Row | January (padded / relocs / sha16) | Real file |
| --- | --- | --- |
| `_build_structure_lens_flares` (PARKED, `instruction-scheduling`) | 4336 / 156 / `40aa8e486e52fe25` | 4336 / 156 / `57d6f033f3793735`; 1327/1327 instructions |

There are 13 differing aligned lines in three groups:

- **N**: the `normalize3d(&s_temp.n)` keep-vs-fold at `+0xa65`, plus the s-plane dot term orders at `+0xb05`, `+0xb3d` and
  `+0xbb0`.
  - January evaluates j,k,i on every `s_temp` / `s_plane` sum; ours evaluates k,j,i.
  - The `t_plane` sums are identical.
- **Tie A** at `+0xe28`: an int-vs-FP transposition.
- **Tie B** at `+0xe6a`: an int-vs-FP transposition.

## 2. Duplicate prevention

Read:
- the four canonical ledgers: grid topology 0826, policy 0829, Fable uniform boundary 0830, authenticated closeout 0901;
- the memory records for the 4-instruction witness and the vendored-assembly mechanism proof;
- the Fable worktree ledger `structure_lens_flares_obj_fable_two_byte_frontier_20260830.md`: ties A and B are decoded as
  DAG-creation-order tie-breaks, after about 15k probes;
- `git log --all` on the file.

`fast_ftol` is already admitted and in production, so the vendored-assembly boundary is no longer the blocker.

## 3. New evidence

The HEAD function was diffed against the Fable base8 witness source (`fable-exact-finish-20260829/scratch/_m_base8.c`).
Apart from names and prototypes, HEAD differs in exactly two respects:

- **(a)** HEAD expands the marker cluster search by hand as scalar `test_point` updates. The witness calls
  `point_from_line3d(&test_point, &direction, offset, &test_point)` there. The expansion exists only to avoid the COMDAT.
- **(b)** HEAD calls `plane3d_from_point_and_normal(&s_temp, &origin, &s_temp.n)` and the same for `t_plane`. That copies the
  normal onto itself. The witness writes only `X.d = dot_product3d((real_vector3d const *)&origin, &X.n)`.

Measured, all in scratch:

| File | Change | Differing lines |
| --- | --- | ---: |
| `lf_F2.c` | casts without `point_from_line3d` | 13 (inert) |
| `lf_F1.c` | HEAD + `point_from_line3d` in the marker search | 6 (N gone except the in-loop s dot) |
| `lf_F7.c` / `lf_F8.c` | F1 + the d-only form at either plane | **2 (ties A and B only)** |
| `lf_F3.c` | both d-only forms | 2 |

So group N is not a scheduler tie. It is the `point_from_line3d` no-emission rule plus the normal self-copy.

## 4. Probes (scratch; siblings 3/3 exact in every probe)

| Probe | Result |
| --- | ---: |
| L1: pointer-normal `fabs` (the breakable lever) | 13, inert |
| LB1: `projection_from_vector3d(&plane.n)` with the January decals body emulated | byte-identical diffs to HEAD |
| LB2: LB1 + `projection_sign_from_vector3d` | 17 |
| L2 / L2b: s-term spellings without `ds_x` | 15 |
| LS2 / LS3 / LS4: `point_from_line3d` grid terms | 16 / 21 / 14 |
| `lf_lattice.py`: named `POINT_DOT_VECTOR3D` at bounds init/loop, `s_temp.d`, `t_plane.d` (16 combinations) | 14-32, all worse |
| F4-F6: named-macro d | 9-32 |
| G1-G3: separate normal vectors | 300+ (frame) |
| M1 / M2: non-self normal sources | 296 / 133 |
| K1-K4 (tie-only F3 basin): grid forms | 2-8, no flip |
| P*: six decl/init-split orders of `s_grid` / `s_count` / `t_distance` | 2 or 10, no flip |

## 5. Outcome

**NOT LANDED.**
- **Group N is OWNER-BLOCKED.** It needs `point_from_line3d` emission, which is forbidden, and the d-only plane form, which
  needs a representation cast. No cast-free d-only spelling was found.
- **Ties A and B stay at the Fable dbg32b-certified frontier.** The new named/pointer access lever does not move them.

The closest scratch body is `lf_F3.c`: 4336/156, two differing instruction pairs. It is not admissible.

**Orchestrator proposal.** Add `_build_structure_lens_flares` to the `point_from_line3d` owner-ruling list, noting that it
stays two instruction pairs short (ties A and B) even with the ruling. Do not unpark.

**Reopen criterion.** Reopen only when all three hold:
- the `point_from_line3d` ruling;
- a cast-free d-only plane spelling;
- a source lever for ties A and B.
