# `interface.obj` `interface_draw_bitmap_modulated_p32` frozen first shot (2026-08-29)

## Result

This fresh strict lane starts from canonical commit
`4b436fc01a4e4f08255280fb740134b9739c05cd` and tests exactly one
authenticated natural C89 reconstruction of
`_interface_draw_bitmap_modulated_p32`. The candidate reaches the January
function's full 345-byte meaningful / 352-byte padded envelope and reproduces
both ordered relocation records at their exact addresses, but the complete
normalized packet is not byte-exact. It is withdrawn without a source-shape
retry. `interface.obj` therefore remains at six strict-exact functions and no
code, data, function, or object credit is added.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `345 / 352` | `2` | `fe9142a548e766da73d68257affe03ea6287b7e0491d70eee971184ca4ba0354` |
| Frozen natural candidate | `345 / 352` | `2` | `4c37da50a351100f17b4bd287d2c236d486ca484024e2cbdcfa3518bb11a2000` |

The ordered relocation sequences agree completely:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+261` | `IMAGE_REL_I386_REL32` (`20`) | `_csmemset` |
| `+331` | `IMAGE_REL_I386_REL32` (`20`) | `_rasterizer_psuedo_dynamic_screen_quad_draw` |

After relocation normalization, 111 of 352 bytes differ. This is a strict
miss even though the size, meaningful boundary, relocation count, relocation
addresses, types, destinations, and order all agree.

## Scope and provenance

Before source work, the current matching methodology, all three applicable
`interface.obj` ledgers, campaign/audit tooling documentation, all-ref history,
and the applicable Stian and Claude/agent policy documents were read.
Repository-local `CLAUDE.md` and `AGENTS.md` do not exist at this commit.
History contains only the initial inventory mention of this function; no
earlier production body, strict attempt, or rejection was found.

The semantic reconstruction is grounded in HCEA commit
`c168af2e747d3095d9a29418ae401f3a39544863`, source blob
`e99234a436357bd91007ea3b1f4548d3d0e5cb6a`, file SHA-256
`aed78f14c06c66227ff395643d2fdb2b98c0d45c43df148dbd7a128a29df3d4a`.
That public source fixes the full-bitmap fallback rectangle, registration-point
rotation and scaling, four-vertex corner/texture-coordinate selection, raw
vertex color, parameter zeroing and unit map scales, and the final pseudo
dynamic screen-quad submission. Although the donor is a later retail
reconstruction, January's complete x86 disassembly independently confirms the
same seven-argument ABI, field accesses, four-vertex algorithm, raw pixel-space
texture coordinates, and two calls.

The single production candidate reconciles that evidence with the current
typed owners:

- `struct bitmap_data` supplies named width, height, and registration fields;
- `dynamic_screen_vertex` and
  `rasterizer_dynamic_screen_geometry_parameters` retain their asserted
  `0x14` and `0x8C` layouts;
- the rasterizer owner header supplies the quad-draw declaration;
- a signed `short` vertex index reflects January's `CX` loop state; and
- the canonical zeroed-parameter setup explicitly initializes the named
  meter, point-sampling, blend, map-scale, texture-scale, and map fields.

No local prototype shadow, raw object or tag access, raw address, raw byte
offset, cast, pointer/integer reconstruction, representation pun, or private
ABI guess is involved.

Immutable artifacts:

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`;
- January `interface.obj` SHA-256:
  `83f7953f8143a7f935c9ecceaeb2a8690c710b5665cf2a900040d901cb12f5f2`;
- source-identical pre-shot baseline `interface.obj` SHA-256:
  `6ff2bcbbc57d1ea1edcb5b4b2281af691df035575fdd4cc339385af1f3075e0e`;
- frozen first-shot `interface.obj` SHA-256:
  `411bbcfa373b53b8c84dfffa84f71ada1d7dab3eae57ac325fca008f8198beb3`;
- restored full-rebuild `interface.obj` SHA-256:
  `af7c41ca75280577c1e1f290804abc2f8e6f51b01ebbc34eff570a92bb6508d3`;
- XDK 3911 CL 13.00.9254.1 SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The retained production source is restored exactly to Git blob
`4e6e6e06d914e7601a6a8a1b9769322f3ee66698`; the candidate definition and
its two temporary includes are absent.

## Boundary and do-not-repeat rule

The first raw divergence is the stack-frame allocation at function offset
`+0x06`: January allocates `0xF0` bytes while the candidate allocates `0xEC`.
That four-byte lifetime/layout delta propagates through the fallback rectangle,
vertex array, and parameter-structure stack addresses. January also homes the
point X coordinate at `[EBP-4]`; the candidate reuses the dead point-argument
slot `[EBP+0x0C]`.

Two independent topology differences remain visible beyond those shifted
locals. January's X-coordinate x87 schedule forms
`local_x*cos + point_x - local_y*sin`; the natural donor expression emits
`local_x*cos - local_y*sin + point_x`. The results are mathematically
equivalent but their complete instruction packets differ. The signed-short
loop state also compares with unsigned `JB` in the candidate because
`NUMBEROF(vertices)` participates in the C integer conversion; January uses
signed `JL`. Finally, the candidate stores the framebuffer blend field before
the four unit-scale stores, whereas January delays it until after the cleared
meter and point-sampled stores.

These are source-level lifetime, expression-topology, integer-conversion, and
store-scheduling differences rather than missing semantics or relocation
identity. Do not retry this HCEA-shaped body, declaration reorderings,
`NUMBEROF`/literal loop-bound swaps, algebraic reassociation, parameter-store
reordering, or stack-lifetime steering. Reopen only with original January
source, a same-compiler natural C donor that independently fixes these exact
topologies, or new compiler provenance that predicts all four boundaries at
once.

No assembly, `volatile`, `register`, pragma, intrinsic, attribute, barrier,
undefined behavior, synthetic anchor, compiler substitution, flag change,
object patch, byte forcing, or comparator exception was used. The production
candidate was emitted exactly once; subsequent compilation only restored the
body-free canonical object.

## Validation

The complete post-revert validation is clean:

- complete `halobetacache_build`, `libcmt_build`, ordinary report, semantic
  report, and progress targets pass;
- all six inherited `interface.obj` functions remain strict exact, totaling
  304 padded bytes and 29 relocations;
- semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden code bytes, 4,810 accepted exact, and zero
  unit errors;
- campaign progress: 384/833 linked objects, 4,771/11,060 functions,
  606,187/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes; Halo is 282/468 objects,
  4,604/7,574 functions, and 593,273/1,770,166 code bytes;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: 12 active, zero stale, and zero invalid records;
- protected Units sentinel `_unit_preprocess_node_orientations`: strict exact
  at 1,920 padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- complete tooling suite: 212/212 tests pass;
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`;
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`;
- final `git diff --check`, tracked-path scope, deleted-path check, and
  source-policy review are clean.

The final tracked change is this additive ledger only. No production source,
header, configuration, semantic record, parked record, protected Units file,
or target artifact is changed. No push is performed.
