# model_animations.obj — opus5 150K lane, wave w2 (2026-09-14)

## Baseline and final

| gate | result |
| --- | --- |
| baseline real file | `== exact 31 residual 1 unwritten 0`, guard passes |
| final real file (unchanged) | `== exact 31 residual 1 unwritten 0`, guard passes, park drift 0, fake scan 0 leads |

`git diff --stat -- source/models/model_animations.c` is empty. No credit is claimed. The inherited 100K-lane functions were not touched.

## `_inverse_kinematics_adjust_matrices` (parked, instruction-scheduling)

Target 864 / 7 / `e1cd1ac47b76f423`; candidate 864 / 7 / `d9fc11bebc1ea09f`. Both have 338 instructions and frame 0x34.

### Evidence

The only difference is the x87 commutative operand choice in the inlined second shoulder `cross_product3d(&shoulder_matrix->up, shoulder_forward, shoulder_left)`. The table lists the operand loaded with `fld` in each product pair.

| term | January | ours |
| --- | --- | --- |
| k | up.i, up.j | up.i, fwd.i |
| j | fwd.i, fwd.k | up.k, up.i |
| i | fwd.k, fwd.j | fwd.k, up.k |

- The leaves and stack discipline are the same.
- The first shoulder cross and both elbow crosses are exact.
- The choice follows none of: source order, leaf first-appearance order, or addressing kind. Elbow cross 1 loads the parameter-field operand in all six terms, while January's shoulder cross 2 mixes.
- The HCEA PPC reconstruction (`blam/bitmaps/inverse_kinematics_adjust_matrices.c`, later build, scalar snapshots) carries no operand-order evidence.

### Shapes (3 new; 6 total with waveC ik1-ik3)

| shape | result |
| --- | --- |
| s1: only `shoulder_forward` named (left/up inline) | 864/7 `c345faa1`, first cross 3 terms off |
| s2: no basis pointer locals | 864/7 `0e2c32bb`, register and first-cross drift |
| s3: declaration order forward-before-left (like the elbow block) | 864/7 `3eb690c1`, first cross 1 term off, second unchanged |

Declaration order is not inert for this tie; it perturbs the ordering.

### Classification and reopen criterion

Classification is unchanged: an instruction-scheduling tie. Reopen only for authentic evidence of January's `cross_product3d` spelling or operand order that fixes the shoulder block without perturbing the elbow block.

## Do-not-repeat

ik1-ik3 (waveC), s1, s2, s3.
