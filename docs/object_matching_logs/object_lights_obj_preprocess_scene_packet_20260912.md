# `object_lights.obj` preprocessing packet (2026-09-12)

## Result

This packet restores the coherent `lights_preprocess_scene` owner graph in
`source/objects/object_lights.c`.  A focused compile and hardened COFF sweep
closed three previously uncredited functions with no regression among the 35
inherited exact owners:

| Function | Meaningful bytes | Padded bytes | Status |
| --- | ---: | ---: | --- |
| `_light_mark` | 89 | 96 | strict exact |
| `_light_get_bounding_sphere` | 224 | 224 | strict exact |
| `_render_debug_light` | 271 | 272 | strict exact |

The strict gain is **3 functions, 584 meaningful bytes, and 592 padded
bytes**.  `_light_mark` closed naturally when its genuine callback owner was
restored; the two private helpers now have their real same-TU caller and retain
private linkage.

## Complete best fuzzy owner

`_lights_preprocess_scene` is retained as complete, plausible reconstruction
but receives **zero exact credit**:

| Measure | January target | Candidate |
| --- | ---: | ---: |
| Meaningful bytes | 2,388 | 2,388 |
| Padded bytes | 2,400 | 2,400 |
| Relocations | 165 | 165 |
| Capstone instructions | 741 | 740 |
| Normalized SHA-256 | `726548627571357009de646c259615ae7b6df8ca5abe3e1d13018ee126e28b52` | `32951ca676c566965d56b67a33c516271944eb950d93c6e02e67304edf985368` |
| objdiff similarity | — | 95.16% |

The remaining mismatch is systematic local allocation and instruction
scheduling (the candidate frame is `0x3e0`, versus January's `0x3dc`), not a
missing control-flow block or relocation.  It is parked rather than pressured
with artificial locals, `register`, `volatile`, `noinline`, or equivalent
code-generation tricks.

## Reconstruction evidence

January target disassembly and relocations are the byte authority.  The HCEA
`lights_preprocess_scene.c` recovery was used only as a semantic map.  The
January object specifically establishes behavior that differs from that map:

- expired lights disconnect from the cluster partition and are deleted,
  without the extra HCEA destroy call;
- the visibility callback topology uses the same-TU marker, bounds, and
  cluster traversal helpers;
- marker submissions preserve the compressed window and scale fields;
- preprocessing terminates with `rasterizer_lights_end` and its `lights`
  profile section;
- a `NONE` attachment is not passed to the typed object getter;
- color-range and lens-flare identifier assertions are present in January;
- first-person flashlight and weapon-marker adjustment paths are present.

The implementation uses named flags, real project types, typed tag/object
accessors, private function names and linkage, and an explicit final `return;`.
It adds no shared-header or build-configuration dependency.

## Verification

The final focused command was:

```text
python tools/campaign/gate.py source/objects/object_lights --all --forbid-emitted-symbol _point_from_line3d --out scratch/object_lights_preprocess_packet.obj
```

It reported **38 exact, 5 residual, 0 unwritten** across all 43 target owners,
and the forbidden emitted-symbol guard passed.  The four pre-existing
residuals remained residual and all 35 inherited exact functions remained
exact.  `git diff --check` and a scan for code-address/BSS-address names and
code-generation pressure markers were clean.
