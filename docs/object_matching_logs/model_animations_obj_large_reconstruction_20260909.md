# `model_animations.obj` large-function reconstruction (2026-09-09)

## Scope and evidence

This packet continues the coherent `model_animations.obj` recovery after
`model_animations_obj_fable_coherent_reconciliation_20260909.md`. It changes
only `source/models/model_animations.c`, this ledger, and the parked-function
manifest.

The recovered algorithms were reconstructed from the January target
disassembly and checked against two independent source maps:

- the HCEA reconstructions of the continuous overlay and inverse-kinematics
  algorithms; and
- the Stian Halo history's recovered `aiming_screen_apply` algorithm and
  semantic local roles.

The independent sources were used as semantic evidence, not copied as an
exact-match claim. The retained code uses genuine animation/model types,
named enum values, tag-block access macros, and descriptive local names.

## Strict exact credit

The focused gate advanced from 22 exact, three residual, and seven unwritten
functions to 24 exact, six residual, and two unwritten functions. Only the
two normalized-byte-and-relocation exact functions receive credit:

| function | meaningful bytes | padded bytes |
|---|---:|---:|
| `animation_set_frame_size` | 121 | 128 |
| `aiming_screen_apply` | 1,555 | 1,568 |
| **total** | **1,676** | **1,696** |

The exact aiming-screen closure comes from the natural January grid-index
reuse: each signed cell index is converted in place to the nonnegative grid
coordinate after its clamping checks. It contains no spill steering,
synthetic control flow, fake casts, or target-byte emission.

## Honest fuzzy frontier (zero credit)

Three complete semantic reconstructions are retained as the best reviewed
frontier, but none is counted as matching:

| function | target meaningful | target padded | candidate padded | objdiff | verdict |
|---|---:|---:|---:|---:|---|
| `inverse_kinematics_adjust_matrices` | 852 | 864 | 864 | 99.190186% | commutative shoulder-basis operand scheduling differs |
| `overlay_animation_apply_continuous` | 953 | 960 | 960 | 86.224920% | complete algorithm; VC7 local allocation and instruction schedule differ |
| `overlay_animation_apply_continuous_scaled` | 1,012 | 1,024 | 1,008 | 82.324715% | complete algorithm; section size and VC7 schedule differ |

The IK function retains ordinary vector algebra and named shoulder/elbow
bases. The continuous overlay functions retain the real compressed and
uncompressed frame paths, orientation blending, scale blending, and source
assertions. No source was distorted merely to improve similarity. These
functions are registered in `config/parked.json` with zero credit so future
work can resume from the best semantic versions without mistaking them for
exact matches.

`animation_update_internal` remains unwritten. Its target body and independent
source map indicate a 32-bit update-kind parameter and 32-bit result, while
the current shared declaration exposes a signed 16-bit contract used by exact
callers. A natural body reached 98.432205% under the 32-bit contract, but the
required owner-header correction perturbed exact consumers. Neither the body
nor a duplicate TU-private type was admitted; this needs an authentic shared
ownership reconciliation first.

`animation_graph_node_matrices_from_orientations` remains unwritten. A prior
candidate depended on duplicated, guessed TU-private node layouts; it was not
admitted because authentic shared ownership has not yet been reconciled.

## Ownership and house-rule audit

- The final packet leaves `model_animation_definitions.h` byte-identical to
  the accepted baseline. A proposed ABI/owner cleanup was rejected after its
  direct/transitive consumer sweep exposed exact-function losses.
- Functions use project `real`, geometry and orientation types, semantic enum
  constants, and tag-block access macros.
- Each parameter is on its own line and every reconstructed function has an
  explicit terminal `return;` (in addition to any intentional early return).
- No address-derived private symbol, raw typed `tag_get` cast, raw object
  access, forced inline, inline assembly, or source-pressure tuning was added.
- The January inline schedule is preserved and the focused gate confirms that
  `_point_from_line3d` is not emitted.

## Verification

```text
python -B tools/campaign/gate.py source/models/model_animations --all \
  --forbid-emitted-symbol _point_from_line3d --out scratch/model-final.obj
```

Result: `exact 24, residual 6, unwritten 2`; the emitted-symbol guard passes.
The two new exact functions compare equal across their complete padded
sections and ordered symbolic relocations. The three new fuzzy bodies remain
explicit residuals, and the three previously parked compressed-keyframe
residuals remain residuals with no exact credit.

The final packet also passes the source fake-match scan and `git diff --check`.
The final owning header is byte-identical to the baseline after the proposed
shared declaration edit failed its focused direct/transitive consumer sweep.
No inherited exact function in `model_animations.obj` is lost.
