# `physics.obj` unrooted rotation helper and withdrawn integrator research

Date: 2026-09-08

## Outcome

This packet adds no production exact-match credit. It preserves one complete
private helper in source whose January section is diagnostically exact when
the withdrawn caller roots it:

- `_rotate_vectors3d_by_angular_velocity`: **417 meaningful / 432 padded
  exact code bytes**, 24 relocations. Target and candidate normalized SHA-256
  are both
  `00064ac648e168b3a7e6fd9ae2204d39cdb17daebb9ee7ad13011021d1f42d61`.

The production per-TU inventory remains **8 exact, 2 residual, and 7
unwritten**. VC7 eliminates the unreferenced private rotation helper after the
fuzzy caller is withdrawn, so both recovered bodies receive **zero production
exact credit**.

## Diagnostic-exact unrooted rotation helper

The helper uses its authentic private name and static linkage. Its five typed
parameters, normalized-axis rotation, January assertion expressions and source
lines, forward/up normalization, and Gram-Schmidt correction agree with the
January disassembly. The HCEA reconstruction independently corroborates the
algorithm. No inline assembly, forced caller, raw offset, or compiler-steering
construct is present.

With the research `physics_update_new` caller temporarily present, the strict
gate found equal 432-byte sections, 24 ordered relocation records, normalized
bytes, and relocation identities. The target report supplies the 417-byte
meaningful size; the remaining 15 bytes are ordinary alignment NOPs. In the
production translation unit, however, the helper is unrooted and is not
emitted. These bytes are therefore diagnostic evidence only.

## Zero-credit `physics_update_new` research reconstruction

The preserved research body is a complete typed reconstruction of force/torque
integration, world inverse-inertia construction, angular-frame rotation,
four-pass penetration correction, and object rest/media flag maintenance.
January disassembly and the HCEA implementation independently support that
control flow, including the shipped behavior that uses the same water tally for
both on-media flags.

One owner boundary remains deliberately unresolved. January writes the
collision-pass mass-point bitmask to the vehicle datum at absolute offset
`0x478`. The current vehicle owner does not expose that field through
`vehicles.h`. The research body therefore omits that final write instead of
defining a guessed physics-side overlay, spelling a raw offset, or changing a
vehicle layout. No vehicle source or header was touched.

Fresh measurements:

| Side | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January | 1,976 | 1,984 | 66 | `8cc42cd03258ede247eca9dbe551ae2981bfb1f8573502d9932d747c6445b049` |
| Candidate | 1,921 | 1,936 | 65 | `6f62f8642db6d076287389ac4b87e285466a2b5d6a480482f22d87859ed979a7` |

Historical objdiff reports **82.58637%**. The function is neither compiled nor
parked, and contributes no Matching credit. Its complete source is preserved at
`research/reconstructions/physics_update_new_20260908.md`. Reopen it when the
vehicle owner exposes the authenticated stuck-mass-point field and a full-tree
compiler-context-safe declaration path is known.

## Ownership and verification

The research body initially required a `units/vehicles.h` include, a typed
`debug_physics_disable_penetration_freeze` definition/declaration, and a genuine
math-owner declaration for `matrix3x3_multiply`. All of those active changes
were withdrawn together with the fuzzy body. `physics_update_new` has no
production declaration and no active park entry.

The orchestrator's 473-unit full-tree sweep proved why the header withdrawal is
required: publishing `matrix3x3_multiply` through `math/real_math.h` regressed
the accepted exact `_rasterizer_dynamic_geometry_initialize`. It also produced
one accidental 880-byte bitmap exact and made three parks stale, confirming a
transitive compiler-context perturbation rather than a safe isolated gain. The
zero-regression policy takes precedence.

Verification performed:

- `python -B tools/campaign/gate.py source/physics/physics --all
  --forbid-emitted-symbol _point_from_line3d`
- `python -B tools/fake_match_scan.py source/physics/physics.c
  source/physics/physics.h source/math/real_math.h --format text`
- `git diff --check`
- orchestrator 473-unit full build and stable exact regression sweep

The current per-TU gate passed the forbidden-symbol guard, preserved all eight
inherited exact functions, and reports the rotation helper unwritten. The
fake-match scan produced zero review leads. The full-tree sweep rejected the
transitive header/fuzzy packet, so the final production exact delta is zero;
the helper and integrator remain useful source-reconstruction evidence only.
The final shared build passes with 1,147 tooling tests, two skips and 26
subtests; all 351 active parks validate with zero stale or invalid records.
