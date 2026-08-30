# `structures.obj` planar-fog definition-index boundary (2026-08-29)

## Result

`_structure_get_planar_fog_definition_index` received one frozen, natural,
typed C89 candidate compile.  It is **not** strict and production source is
fully restored.  The candidate reached the target's 208-byte padded section
size and five-relocation topology, but its meaningful code was 198 bytes
instead of 207 and both normalized bytes and relocation addresses differed.

This owner remains unwritten.  Do not retry this source topology through
declaration, spelling, branch, sentinel, or mask variations.  Any future shot
requires genuinely new provenance that fixes the original register ownership
and instruction schedule before compilation.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful bytes | 207 | 198 |
| Padded bytes | 208 | 208 |
| Ordered relocations | 5 | 5 |
| Normalized SHA-256 | `99f700ce52347a38e0cf6899dac3f8aebb532c2c178a993fd9b4919c5a86e9c3` | `bdca9d700bc4a35376164dbbfd98dbd8e2ecf386ec5a82a5c28599eb50b65b89` |

The section comparison found 179 differing bytes across the padded sections.
The complete candidate object SHA-256 was
`e666b92d52372f718dddc1696a5b1f70170f97ce9504e716a937425f41bcd4c5`.

## Evidence and ownership

The candidate was selected because three independent sources agree on the
semantic chain:

- January `source/structures/structures.obj` is the byte and relocation
  authority.  Its complete object SHA-256 is
  `0e0875524b3580c272bb51b1d5630540d58ce9e1e1ae6c3ef52ca5f30669a4d7`.
- Clean HCEA donor
  `src/blam/structures/structure_get_planar_fog_definition_index.c` at commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c` has Git blob
  `835c12010553ac80fc6e74b5f1e3d0af4def4a84` and file SHA-256
  `a2da29c0812e01862b4aa0e44e00e9867d0816b53227d3b157cd96470f1eff58`.
- Preserved Stian and Pastudan histories contain the same target-derived
  control-flow chain.  The inspected Stian checkout is
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, with `structures.c` blob
  `ef3b2a7b37a1e5000d957dadbe52ba572e3533f6`; Pastudan commit
  `918af885935ec470a31256ecce9a977b12b01f80` has `structures.c` blob
  `09b94a003c47c4339695dbe2621e30b1166862c7`.

A repository-history and ledger search found no earlier production attempt for
this owner.  It was link-closed: all five target relocations resolve to public
owners, four calls to `_tag_block_get_element_with_size` and one call to
`_scenario_get_sky`.  It was also a useful closeout dependency because the
adjacent 448-byte `_structure_get_planar_fog` calls it.

## Frozen natural candidate

The sole emitted candidate used existing typed definitions and named fields:

- `struct structure_bsp::clusters`, `fog_planes`, `fog_regions`, and
  `fog_palette`;
- `struct structure_cluster::fog_reference`;
- `struct structure_fog_plane::fog_palette_index` (the existing legacy field
  name for the plane's fog-region designator);
- `struct structure_fog_region::fog_palette_index`;
- `struct structure_fog_palette_entry::fog.index`; and
- `struct sky::indoor_fog_screen.index`.

The C89 body returned `NONE` for an absent cluster, absent sky, or broken fog
chain.  Otherwise it resolved the cluster's signed fog designator, optionally
followed the high-bit fog-plane indirection, then followed fog region and fog
palette tag blocks.  The candidate used only `TAG_BLOCK_GET_ELEMENT` and typed
field access.  It contained no assembly, `volatile`, `register`, pragma,
intrinsic, compiler attribute, raw address, raw offset, pointer/integer
reconstruction, representation pun, undefined behavior, or byte-forcing
device.  A non-emitting `/Zs` type check passed before the one production
compile.

## Fixed boundary

The five relocation destinations and their order were correct, but their
addresses were not:

| Relocation | Target address | Candidate address | Destination |
| ---: | ---: | ---: | --- |
| 1 | 39 | 38 | `_tag_block_get_element_with_size` |
| 2 | 56 | 55 | `_scenario_get_sky` |
| 3 | 111 | 106 | `_tag_block_get_element_with_size` |
| 4 | 148 | 143 | `_tag_block_get_element_with_size` |
| 5 | 182 | 185 | `_tag_block_get_element_with_size` |

The first irreducible divergence is visible at function entry.  January saves
EDI and owns the `NONE` sentinel in EDI for all failure paths; the natural
candidate materializes `0xFFFFFFFF` directly in EAX and never saves EDI.
January also lowers the signed designator test as `test ah, ah` / `jns`, while
the candidate lowers it as `test ax, ax` / `jge`.  Matching size and relocation
count therefore do not imply an admissible near match: the source lacks
evidence for the target's register-lifetime and condition-lowering schedule.
No tuning was attempted after this observation.

## Restoration and gates

The source inverse-revert is exact:

- `source/structures/structures.c` restored Git blob:
  `02bd525a880e1fecc82fb03a6f94ff7b25afcf13`;
- `source/structures/structures.h` restored Git blob:
  `3aba07a47350023a322c493d6d433c409abf9ba0`.

After restoration, the focused gate reports six exact, zero residual, and
eleven unwritten functions for `structures.obj`; all six previously exact
siblings remain exact.  This ledger is the only retained campaign change from
the shot.

The full validation packet also passes:

- `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress` complete successfully;
- semantic report: 470 units, 4,918 functions evaluated, 4,801 semantic
  exact, 4,811 accepted exact, 131 hidden exact, and zero unit errors;
- progress: 384/833 objects, 4,772/11,060 functions, 606,426/2,198,102 code
  bytes, and 1,856,050/4,176,062 data bytes;
- admission audit: zero candidates, contradictions, and revocations;
- parked audit: 12 active, zero stale, and zero invalid;
- test suite: 212 passed;
- protected Units sentinel: 1,920 padded bytes, 87 ordered relocations, and
  strict normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256:
  `b9cb28ea0bd32f8729fd7b241e83a6ab71e01b56f44350abc3796bc63ec844fb`;
- `build/semantic_report.json` SHA-256:
  `0f782f197d2e92d9837d8252e43b074b43bf1f2386da21a8d7211c81553ec542`.
