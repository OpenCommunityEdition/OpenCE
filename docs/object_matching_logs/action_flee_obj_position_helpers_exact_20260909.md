# `action_flee.obj` position-helper packet (2026-09-09)

## Result

Two previously unwritten January functions are now strict COFF matches in a
single, caller-connected translation-unit reconstruction:

| January owner | Recovered private name | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | ---: | --- |
| `_code_00003210` | `action_flee_find_flee_position` | 629 | 640 | 23 | `058dad92555bc68a3f7deb04f41c3b1995d70808b593c142c7f534383e18b0b7` |
| `_code_000039e0` | `action_flee_at_flee_position` | 318 | 320 | 13 | `85c493abe1c07f9d923b165c98d0ec59098a6c21ef04a0954f8c5451507804fa` |

The packet therefore contributes **947 meaningful exact code bytes** and
**960 padded exact bytes**.  The focused gate reports 9 exact, 3 residual and
1 unwritten function in `action_flee.obj`; the inherited baseline was 7 exact.
No whole-object completion is claimed.

## Name and behavior evidence

The independent HaloCEA reconstructions at
`research-cache/halocea-full-blobs-20260830/src/blam/ai/` recover the three
private names and their caller relationships:

- `action_flee_find_flee_position.c`
- `action_flee_current_position_exposed.c`
- `action_flee_at_flee_position.c`
- `action_flee_setup.c`
- `action_flee_perform.c`

January remains the byte and branch-semantics authority.  The first exact
owner carries January's line-522 non-swarm assertion, selects and commits a
cover/panic firing position, and computes an approach point through the real
path subsystem.  It uses `actor_get`, `actor_definition_get`, `prop_get` and
`TAG_BLOCK_GET_ELEMENT`, plus the owner-header declarations in `actors.h`,
`ai.h` and `path.h`; no raw datum/tag casts or local foreign prototypes were
introduced.

The three ACTOR_PERCEPTION.C situation declarations are exposed from the end
of the associated `actors.h` only to consumers that opt in with
`ACTORS_EXTERNAL_SITUATION_ROUTINES`. Their earlier placement in the middle
of that heavily shared header perturbed C2's declaration-position-sensitive
allocation in `units.obj` and reopened one otherwise exact owner. The opt-in
owner declaration keeps ActionFlee fully typed while preserving all 189
exact `units.obj` functions.

The second exact owner carries January's line-470 assertion, validates the
current encounter firing position, honors the already-at-destination fast
path, and compares the squared body distance against the actor's destination
tolerance.  January's final branches show that a tracked prop with line-of-
sight value clear or occluded prevents this fallback arrival result; no prop,
or a different line-of-sight state, permits it.  This differs from the later
HaloCEA lift's final polarity, so the donor was used only as a semantic/name
map and the January branch behavior was preserved.  The final boolean is
normalized with the ordinary `result = !result; return result;` idiom; the
open Marathon cseries source uses the same toggle-then-return spelling in
`cseries.lib/macintosh_utilities.c`, so this is not an invented dependency or
optimizer directive.

## Honest residuals retained with zero credit

The coherent callers needed to make the private static owners emit are kept as
the best currently reviewed typed reconstructions, but they are not exact and
receive zero matched-byte credit:

| Function | January | Candidate | Relocations (target/candidate) | Objdiff |
| --- | ---: | ---: | ---: | ---: |
| `action_flee_setup` | 272 padded / 265 meaningful | 256 padded | 12 / 12 | 80.265305% |
| `action_flee_current_position_exposed` | 352 padded / 347 meaningful | 352 padded | 13 / 13 | 83.46342% |
| `action_flee_perform` | 864 padded / 857 meaningful | 848 padded | 30 / 30 | 77.46595% |

Their normalized target/candidate SHA-256 pairs are respectively:

- setup: `1a415a0adece0dd58822de4fab9d12085b404e3531000124330031c56d9effa8` /
  `cfc7ace7bcb857227c8c0559c80681a4f9ff2b142fa9df3e57c59af75960a898`
- current-position exposure: `498dc2c0460731080f45893a5b2f648e99742a6d5733c010626f4466c0b31bb5` /
  `bbfe7ce7c3bafd235c0528f0480f5836b1d5016ec27e364ef77db6381c616c60`
- perform: `d62f05304574f88e07577a3c0942f0fa591776cc95c210216383895494921f39` /
  `e4b2a3cecd2525f5989ea94abd238fd8cc687627248f91ba39f597fe4ae66f7e`

These are ordinary typed C with named constants and subsystem APIs.  They
contain no volatile/register forcing, pragma, assembly, raw offset,
representation pun, fake dependency, undefined behavior or nonsensical
branch.  They should be revisited only with authoritative January source/local
records or another natural same-compiler donor, not expression-order search.

## Verification boundary

The final PID-isolated campaign gate compiled only `source/ai/action_flee.c`,
compared every target function, and rejected any emitted
`_point_from_line3d` owner.  The guard passed.  Because the semantic symbol
renames are part of this packet while the shared split object intentionally
remained untouched, the focused gate used temporary in-memory aliases from
the recovered private names to the old `_code_*` split labels.  No Ninja,
configure, shared build output or unrelated source file was touched.

The ordinary `real_math.h` distance expression also emits select-any fallback
copies of `_distance_squared3d` (64 bytes), `_vector_from_points3d` (48 bytes)
and `_magnitude_squared3d` (48 bytes).  Each has zero relocations and exactly
matches the corresponding January-selected owner in `action_alert.obj`
(`f42fa39f289bfa50`, `905f56dbe4597d2b`, and `0d04579096eeab3a`
respectively).  They receive no target-owned or progress credit; this packet
does not alter their ownership.  No `_point_from_line3d` definition or
reference is emitted.
