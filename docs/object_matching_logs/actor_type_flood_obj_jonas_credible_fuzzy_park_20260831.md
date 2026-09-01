# `actor_type_flood.obj`: credible Flood-shamble restoration and fuzzy park

## Result

The previously unwritten `_actor_type_flood_desire_shamble` is now restored as
ordinary typed C and retained under the 2026-08-31 fuzzy-park rule. The object
moves from `1 exact / 0 residual / 1 unwritten` to
`1 exact / 1 residual / 0 unwritten`:

| Function | January | Candidate | Result |
| --- | --- | --- | --- |
| `_flood_decide_action` | 368 padded bytes / 33 relocations | exact | strict exact preserved |
| `_actor_type_flood_desire_shamble` | 1296 padded bytes / 44 relocations | 1312 padded bytes / 44 relocations | credible fuzzy park, 84.69773% objdiff |

The residual function keeps all 44 relocation destination identities in the
January order. Target-owned `.data` and `.rdata` remain exact, and the full
build reports no unit error.

## Source and semantic evidence

The reconstruction is the measured Jonas recovery preserved by commit
`26a19348d04e7fbd388b9f450fbc9fdae3a1cb15`, cleaned to current house rules.
Its independent semantic oracle is authenticated HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
`src/blam/ai/actor_type_flood_desire_shamble.c`.

The retained source uses:

- canonical `actor_variant_definition` fields instead of a speculative padded
  firing-variant structure;
- named action goals, movement-switching values, combat threshold, prop-state
  bounds, unit animation state, and actor-variant/damage flag bits;
- typed actor, unit, prop, point, and vector accessors; and
- the existing owner-header declarations for
  `actor_combat_get_firing_variant_definition` and `actor_has_ranged_weapon`.

The cross-translation-unit declaration for
`actor_type_flood_desire_shamble` now lives in `actors.h`; the duplicate
foreign declaration was removed from `actor_perception.c`.

HCEA is used as a semantic oracle, not copied as an exact-source claim. The
January disassembly proves one behavior that differs from HCEA's recovered
topology: for movement-switching type 2, a missing target or a target outside
`combat_range_upper_bound` continues into the established switching-update
body rather than taking the early false result. The retained function follows
January.

## Residual boundary

The target and retained candidate measure:

| Measurement | January | Candidate |
| --- | ---: | ---: |
| Padded function bytes | 1296 | 1312 |
| Relocations | 44 | 44 |
| Normalized SHA-256 | `067d3d1a80f86aaa9906529ad2cfe9f187bb77f1168c60c0208ddc98a69f14bc` | `47bcacd072921ce5ae46bb953f730061896e6db84e7a8e2a2c5fdfd82136f548` |
| Objdiff fuzzy score | 100% | 84.69773% |

January keeps both early boolean epilogues inside the initial decision region.
VC7 tail-sinks the false epilogue to the end of the candidate. Five short
branches consequently become near branches, expanding the function by 16
padded bytes and changing the downstream register/live-range schedule. Prior
bounded natural-C topology, scope, declaration, and expression families did
not place that epilogue without artificial compiler steering.

No `volatile`, `register`, optimizer barrier, raw address or byte-offset access,
speculative structure overlay, pointer reconstruction, representation punning,
inline assembly, fake dependency, dead branch, or object-byte patch is
retained. Reopen only for authenticated January source/local records or new
natural same-compiler evidence that controls the early epilogue placement.

## Verification

- full 576-step Ninja build: pass;
- strict gate: `1 exact / 1 residual / 0 unwritten`;
- `_flood_decide_action`: strict exact preserved;
- common-target sweep across 571 built objects: zero exact-function losses;
- all 44 residual relocation destinations: same identities and order;
- target-owned non-code: exact;
- fake-match scan over the three touched source/header files: zero leads.
