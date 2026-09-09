# `decals.obj`: collision caller graph and exact math helpers (2026-09-08)

## Outcome

`decal_new_from_collision` now has a complete, typed, source-level
reconstruction.  It is deliberately recorded as fuzzy: the January section is
6,176 padded bytes with 210 relocations, while the retained candidate is 6,112
padded bytes with 203 relocations.  No exact credit is claimed for that body.

Five January math helpers authenticated by the same call graph are newly
strict-exact.  The completed body also makes the previously residual
`decals_delete_permanent_from_cluster` section strict-exact through the natural
translation-unit layout.  Relative to the PID-gated baseline, the packet adds
six exact functions and 946 meaningful / 992 padded exact bytes, with no exact
losses:

| function | meaningful | padded | relocations | normalized SHA-256 |
|---|---:|---:|---:|---|
| `_projection_from_vector3d` | 72 | 80 | 0 | `70d35416dc81990b367872a8cb7c8b2326f1c79396a84350e27f326f4bdda8dd` |
| `_project_point2d` | 223 | 224 | 12 | `48bc73c53c840a9b7df638fe8ad8ab5f6249c188a4113e7d5d0af8acf4fe7fc1` |
| `_triple_product3d` | 68 | 80 | 0 | `afecbfe88d3597ab7a325a98bb31366f80ccd4d3b807cc891371168a28297c54` |
| `_plane2d_from_points` | 134 | 144 | 3 | `06e99bde6dafb53a3e42e160e2a7baefbb134718956cf0c647716285a2c678b1` |
| `_plane3d_from_point_and_normal` | 57 | 64 | 0 | `d80a2fe216253cf40b2ad8990a1900ebfe03bbf8b994cff6aeaa849a991d76de` |
| `_decals_delete_permanent_from_cluster` | 392 | 400 | 29 | `b3eeb427eeef1e02f793286b1011a5b89822e041f0b442759259255e1117220a` |

The baseline object was preserved as
`%TEMP%\decals_collision_baseline.obj`.  The five math helpers are absent from
that object, and its 400-byte `_decals_delete_permanent_from_cluster` section
has normalized SHA-256
`6fb830e33513eb040f0ff7cb46cc9167d30a96893dedb145370b416426025ffd`.
The baseline gate was 14 exact, 2 residual, and 17 unwritten functions.  The
retained candidate gate is 20 exact, 4 residual, and 9 unwritten functions.

The source base for this packet is
`4ebe0ff6ec889aa7ae5657086afb51b175694aff`.

## January collision-body evidence

The January COFF object and disassembly are authoritative.  The retained
collision body follows its assertion sequence, tag and bitmap lookups, basis
construction, random rotation and snap-to-axis branches, sprite selection,
texture-residency check, projection setup, BSP surface queue, deviant-surface
grouping, signed BSP plane handling, wrap transforms, geometry production,
cache/decal allocation, UV packing, datum initialization, quad fan, and all
failure cleanup paths.

The principal frozen comparison is:

| measurement | January | candidate |
|---|---:|---:|
| padded size | 6,176 | 6,112 |
| meaningful size | 6,162 | 6,098 |
| relocations | 210 | 203 |
| non-padding instructions | 1,782 | 1,770 |
| normalized SHA-256 | `38ad39f815f1cf529eb65b0f0e0f7350d42a7c2aabe6224b5f63b0c3839a5dd7` | `7b7f66505275ce81662ad167d9dcbc5a7401e48e690716490187fef44f0b423c` |

The `difflib.SequenceMatcher` ratio over the non-padding mnemonic streams is
82.432432%.  Size, relocation count, and hash all remain nonexact, so this is
explicitly zero-credit fuzzy evidence.  The residual was parked rather than
being source-shape tuned after the complete control/data graph was present.

The local `decal_definition` layout is constrained by January loads rather
than guessed padding: its total size is `0x10C`, its nested shader begins at
`0x94`, its bitmap-map `tag_reference` begins at `0xD8`, and
`shader.map.index` is at `0xE4`.  The January collision body loads
`[definition+0xE4]` immediately before the bitmap `tag_get` call.  The
`decal_geometry` layout is likewise fixed by the January offsets: 1,024
24-byte vertices, `decal_vertex_count` at `0x6000`, 1,024 short per-surface
counts, `decal_surface_count` at `0x6802`, and 1,024 long surface indices, for
a total size of `0x7804`.

## Donor and semantic census

The complete HCEA sources used as semantic evidence were:

- `scratch/_halocea-reference-20260907/src/blam/effects/decal_new_from_collision.c`;
- the HCEA decal definition, shader-decal, projection, geometry, vertex, datum,
  bitmap-group sequence/sprite, and bitmap-data headers;
- `scratch/_halocea-reference-20260907/src/blam/effects/decal_sprite_get_bounds.c`;
- `scratch/_halocea-reference-20260907/src/blam/effects/decal_projection_create.c`;
- `scratch/_halocea-reference-20260907/src/blam/effects/decal_clip_to_surface.c`;
- `scratch/_halocea-reference-20260907/src/blam/effects/decal_insert.c`;
- the HCEA math/structure implementations of the five newly exact helpers.

The x86 control-flow and operand census used
`scratch/pastudan-halo/src/halo/effects/decals.c`.  Raw offsets and anonymous
names from that donor were used only to corroborate January disassembly; none
were imported into the implementation.  The Fable/Claude/Opus history census
covered the visible `decals.c` revisions at `f9dd...`, `4277...`, `18e...`,
`aaff...`, `1612...`, and `e029...`.  The authenticated historical source at
`e02971dfd` supplied the natural control forms for the five exact math
helpers.  Marathon source was searched and had no direct decal/projection
analogue, so no Marathon semantics were imported.

Later HCEA-only initialization, PowerPC fused arithmetic, and donor-only
debug/type guards not present in January were excluded.

## Caller-connected private helpers

January contains the following raw private labels.  HCEA role/signature
evidence plus January call order and relocation identities establish these
semantic mappings:

| January label | semantic name | January padded size | relocations | normalized SHA-256 |
|---|---|---:|---:|---|
| `_code_00087190` | `_decal_sprite_get_bounds` | 384 | 16 | `e77622d865444b0cc3d5b40661452a0a948a15064c53972db1c46490de68eaf3` |
| `_code_00087f20` | `_decal_insert` | 704 | 44 | `3defca9d1e5f1f220feb75ebd0170b7aac87716be3ea7d170fabf2d2f3d80be9` |
| `_code_00088970` | `_decal_projection_create` | 672 | 14 | `ff6617f9b07e46d2c4ce1a5e9a2ee77e668a91f2f3ee389d89a9e948ea14a4db` |
| `_code_00088c10` | `_decal_clip_to_surface` | 1,776 | 59 | `d875457fa2f021767305a52fdf7879167eef3a756992184c611bfe0a8a3f1109` |

Across all 833 January split objects, all incoming references to these labels
are the collision body's six same-TU calls: one sprite-bounds call, one insert
call, two projection-create calls, and two clip-to-surface calls.  The full
cross-TU relocation census therefore supports their private/static ownership.

`decal_sprite_get_bounds` is retained as a complete static semantic helper.
Its candidate has the exact January size (384), meaningful extent (379),
relocation count (16), and 132 non-padding instructions, but has normalized
SHA-256
`9658324e079e3b4b0b03f7c604bc82633548bfdbcb90e25a9518ea7cafbe8ffc`.
The non-padding mnemonic ratio is 98.484848%.  Alignment-aware disassembly
isolates the residual to x87 operand scheduling for the two negated
registration-point extent products.  The natural HCEA/Pastudan expression is
retained and the helper receives zero exact credit.

`bsp3d_get_plane_from_designator` is emitted naturally by the genuine BSP
accessor used by the collision body.  Target and candidate are both 112
padded / 101 meaningful bytes with one relocation and 44 non-padding
instructions.  Target SHA-256 is
`9715a442e80abf39d135d2d21272aba32ea1ada03e0505843c0a67cf39703300`;
candidate SHA-256 is
`798bd1fe257e59ef84360defb21e53c80d24f3fcdfe0d7d0eadec092a0fd871b`.
The only instruction difference is January `jns` versus candidate `jge`
after the same `test` instruction.  It is a complete 97.727273% mnemonic
residual and receives zero credit.

A complete typed `decal_projection_create` experiment reached the exact
672-byte size, 14 relocations, and 246-instruction count, but retained register
allocation and x87 scheduling differences.  It also perturbed a previously
exact section through TU-wide optimization, so it was removed.  A natural C
`real_a_rgb_color_to_pixel32` experiment required the compiler's `__ftol2`
path while January uses its original out-of-line inline-assembly-style FISTP
packet; it was also removed.  No assembly, intrinsic, volatile, forced-inline,
or other byte-forcing substitute was retained.

## Publication reconciliation

The semantic configuration is now integrated rather than leaving anonymous
gate aliases in production metadata:

- `_code_00087190` -> `_decal_sprite_get_bounds`;
- `_code_00087f20` -> `_decal_insert`;
- `_code_00088970` -> `_decal_projection_create`;
- `_code_00088c10` -> `_decal_clip_to_surface`.

All 833 January split objects were scanned for incoming relocations to the four
raw labels.  The only hits are the six calls in
`_decal_new_from_collision`: one bounds call, one insert call, and two calls
each to projection-create and clip-to-surface.  The four corrected
`config/symbols.json` entries therefore carry `"static": true`.  The
implemented `_decal_sprite_get_bounds` is a static definition.  The local
declarations for the three still-unwritten helpers remain explicit gate
scaffolding; they are not public APIs and must become static definitions when
their bodies are reconstructed.

January's call sites also prove why fabricating external leaf stubs would be
wrong.  `_decal_projection_create` receives its first `basis` argument in EBX
and only the remaining two arguments on the stack; `_decal_clip_to_surface`
receives `geometry` in EAX and its remaining nine arguments on the stack.
The current unresolved declarations necessarily compile as ordinary cdecl
calls, including the otherwise absent pushes.  Restoring the real same-TU
static bodies is the legitimate path for C2 to recover those compiler-private
ABIs; no calling-convention annotation or fake callee is introduced here.

The static data correction required for the typed geometry is:

- `_decal_geometry`: file offset `4414840` (`0x435D78`), corresponding to
  `_bss_00435cb0+200`.

The neighboring established storage evidence is `_decal_globals` at file
offset `4414832` (`0x435D70`, aggregate `+192`).  January collision relocations
to aggregate offsets `+24776` and `+26826` are respectively
`_decal_geometry+0x6000` (`decal_vertex_count`) and
`_decal_geometry+0x6802` (`decal_surface_count`), not separate invented
globals.

The authenticated decal flag and type enums now live in their proper owner,
`source/effects/decal_definitions.h`, including the original
`SAPIEN_ONLY` names.  Its four direct C consumers preserve their current exact
counts: `effects` 31, `decals` 20, `rasterizer_xbox_decals` 27, and
`structure_runtime_decals` 7.

The complete decal tag record remains TU-private for now.  Its owner header is
included by four units, while `rasterizer_xbox_decals.c` still owns a distinct
load-bearing partial definition; publishing the complete record before that
consumer is reconciled would produce a C redefinition, not a legitimate
header cleanup.  The bitmap sequence/sprite records likewise remain
TU-private: `bitmap_group.h` has 39 C consumers and eleven units currently
carry load-bearing local definitions.  This is an explicit owner-unification
boundary, not permission to invent reserved facades or to perturb unrelated
C2 schedules.  Geometry/projection records and the four private helper
prototypes correctly remain local to `decals.c`.

`debug_decals` is now declared by `decals.h`, its subsystem owner, rather than
by a caller-local `extern` in `decals.c`.  All nine direct consumers of that
header were PID-gated and preserve their current exact-function counts.  The
packet is ready for the root integrator's shared configure/build/stable-sweep
admission; no whole-object-completion claim is made.

## Gate and house-rule audit

The reconciled object was compiled to a PID-specific `%TEMP%` path.  Because
the shared split tree had deliberately not been regenerated during parallel
work, this isolated gate used temporary old-name aliases solely to compare
against the stale target symbol table:

```text
python -B tools/campaign/gate.py source/effects/decals --all \
  --alias decal_sprite_get_bounds=code_00087190 \
  --alias decal_projection_create=code_00088970 \
  --alias decal_clip_to_surface=code_00088c10 \
  --alias decal_insert=code_00087f20 \
  --forbid-emitted-symbol _point_from_line3d \
  --out %TEMP%\decals_collision_final_candidate.obj
```

The gate remains 20 exact / 4 residual / 9 unwritten.  All six new exact
closures and all fourteen inherited exact sections survive reconciliation;
the exact delta remains 946 meaningful / 992 padded bytes.  The emitted-symbol
guard passes.

The source uses multiline signatures, project scalar/geometry types, typed tag
and datum access macros, named flags and enums, C89 declarations, and explicit
returns.  The compact two-bit color-mode extraction and packed-UV assertion
are retained because they are corroborated by the January instruction and
assertion-string evidence and `cseries.h` has no equivalent adjacent-bitfield
extractor.  They are not arbitrary bit steering.  The implementation
introduces no raw source address, anonymous executable function/global name,
pointer/integer reconstruction, raw reserved facade, assembly, `register`,
`volatile`, attribute, intrinsic, forced retention/export, or fake dependency.
The point/vector casts are limited to the project's identical three-real
representations at math API boundaries and reproduce genuine Bungie/HCEA
semantics.  `_point_from_line3d` is not emitted.  `symbols.json` parses and
`git diff --check` passes apart from Git's informational LF/CRLF warning.

No configure, Ninja build, staging, commit, or push was performed in this
parallel packet.  Those shared operations remain the root integrator's final
admission gate.
