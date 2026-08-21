# `rasterizer_geometry.obj` Jonas compressed-vertex accessor wave (2026-08-21)

## Result and fixed boundary

This Jonas-owned source-only wave starts from exact canonical commit
`ffd8962fce5b766982c1d6fb32a70d8a8cd87b8c` on isolated branch
`jonas/rasterizer-geometry-accessor-five-20260821`. The fixed boundary was the
following five public ordinary-C bodies and no others. All five were present
together in the first and only natural code-producing candidate compile, all
five were independently strict immediately, and all five are retained without
a retry or code-generation tune.

`rasterizer_geometry.obj` advances from 3/19 to 8/19 exact functions and from
51/4,054 to 639/4,054 meaningful code bytes. It remains `NonMatching`. The
strict net gain is 588 meaningful bytes, 624 padded bytes, and 46 exact
relocations. No data credit is claimed.

| Fixed retained body | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_environment_vertex_compressed_get_point` | `0x0016FD50` | `103 / 112` | 8 | `422ed56a44b2b6eea91e984a52987163bac6e59c74f3b50126fc6c9155a8665d` |
| `_environment_vertex_compressed_get_normal` | `0x0016FDC0` | `124 / 128` | 9 | `ea7cc0a60709c0310e4f59c2324c5b7937bf2a1426c9d0c546fb9e8948dceb60` |
| `_environment_vertex_compressed_get_texcoord` | `0x0016FE40` | `98 / 112` | 8 | `aa14c02f57fdc23f55183c2804d5b2c16824dde32c59f3741cc92e3bb2331272` |
| `_environment_lightmap_vertex_compressed_get_incident_radiosity` | `0x0016FEB0` | `123 / 128` | 9 | `2bf9486ce2c1a183e94bc214e54b4017a26cd66bd3cb7b752101df2f31a673e7` |
| `_environment_lightmap_vertex_compressed_get_texcoord` | `0x0016FF30` | `140 / 144` | 12 | `5fb82963b6c9e014802b61529ce70a855fb730120cfb23e2d28974657e8559ab` |

## Immutable target, source, and compiler provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/rasterizer/rasterizer_geometry.obj` SHA-256:
  `d6357486072be1c8fb98fed92bc36eb0c1d6c7e68386df5c9ce164f0627ebb49`.
- Exact-HEAD baseline `source/rasterizer/rasterizer_geometry.c` Git blob:
  `78681b2c0a7be4fdb91dc198bf427f104b6f6b6c`, 3,607 payload bytes,
  with Git-blob payload SHA-256
  `51ed085cdcecf4f79038f4f4d6ee2adb4875293dea09c0f89b5513a5045fbb9a`.
- Retained translation-unit Git blob:
  `d12046aee23f4c5c9e7418ccecb095cafa154036`, 6,087 payload bytes,
  with SHA-256 of the exact staged `git cat-file blob` payload
  `78c6d52e34d12a358fa562bc2a1b2d21976e32774c6d29fe6bcd06593a8f5670`.
  These identities cover Git-blob bytes, not the CRLF working-tree view.
- The shared `source/rasterizer/rasterizer_geometry.h` is unchanged at blob
  `b80e3b2ec396faafce1a88d40d65dfc162b1b8b3`, 1,565 payload bytes,
  payload SHA-256
  `2bab945f8e4317ec6a1d2d48bb575258958a84f28fbaecce5daa284c5b4065e7`.
- The retained pre-commit candidate object is 4,112 bytes with SHA-256
  `63572bdcb0968adc5b325ea5137337433264b97a46ad185a19a2d2a18063e9e4`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural repository flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`
  with the unchanged XDK 3911 include graph.

The clean local HCEA semantic reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Every cited file below is
directly tree-reachable from that commit. Hashes are over exact Git-blob
payloads, independently authenticating the Git object identifiers.

| HCEA evidence | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| point body | `a1a2b5c2fe7f17662a850419cc0b4bfa77384b15` | 380 | `30695672758f7e7c30a7519418f9f68e795a96c2240a59d76ed175283b0fa381` |
| normal body | `7958ded7ae8668c4f805d91772c28b92cbce6fa4` | 1,234 | `9f01477792ec8765862688f700937b08058771d9aa0742dd6cb030e0548458ec` |
| environment texcoord body | `e6e0490d2275dccc10243a350330c7763d789a6c` | 400 | `d41d4e02f6918886cb73deee323dd7ce6a8590bb45802c8d071d12aa3614c4cc` |
| incident-radiosity body | `11f9a0a1078dca8db0a333c421a0f695622f5dc0` | 1,165 | `c6e43a89f85f93ca2a80298bf89a8671019552c6b15a6433ffb1a08794bfd926` |
| lightmap texcoord body | `5199b6543f094cb1d2038c8b03d5e16f30e9c059` | 700 | `4b3f535e601d54d849e807175c8ea68d8c33334594e9a70e27ab65126dd1dc5a` |
| environment compressed layout | `770a88dbee99031ddaa0eef33bf850b7658a327e` | 562 | `a7b15827a20dce506c19a3ddcf80397890740df7cbabe65b1f48e07167a00cb4` |
| lightmap compressed layout | `360f0e93138ae0e4f9e310e3914dfade8b443b7d` | 502 | `a31e137b3b29b9fb01d00423c94cbacb652307aae6ca0994c2f4465be532afd4` |

HCEA supplies readable public intent, field names, and compressed-layout
corroboration. It is not the PC byte authority. In particular, the HCEA
normal and incident-radiosity donors inline their decode, whereas January PC
COFF calls `_uncompress_int32_to_real_vector3d` and copies the helper-returned
temporary. January therefore governs helper topology, all assertion metadata,
sign extension, constants, relocation order, padding, and acceptance. No
Stian source was needed for this wave.

Canonical history has no earlier committed definition of any fixed body and
no prior matching ledger for this accessor set. Before dispatch, repository
and live-worktree collision checks found no competing `rasterizer_geometry.c`
implementation. The January target defines each screened public symbol once.

## Exact typed ABI, callers, and layout

The retained interfaces are source-local and self-typing:

```c
void environment_vertex_compressed_get_point(
	struct environment_vertex_compressed const *vertex,
	real_point3d *point);
void environment_vertex_compressed_get_normal(
	struct environment_vertex_compressed const *vertex,
	real_vector3d *normal);
void environment_vertex_compressed_get_texcoord(
	struct environment_vertex_compressed const *vertex,
	real_point2d *texcoord);
void environment_lightmap_vertex_compressed_get_incident_radiosity(
	struct environment_lightmap_vertex_compressed const *vertex,
	real_vector3d *normal);
void environment_lightmap_vertex_compressed_get_texcoord(
	struct environment_lightmap_vertex_compressed const *vertex,
	real_point2d *texcoord);
```

Each January callee consumes two full 32-bit pointer arguments from ordinary
stack slots and returns with plain `ret`; each caller pushes both pointers and
performs caller cleanup. The undecorated C identifiers naturally emit the
target leading-underscore COFF symbols with no stdcall suffix. This proves an
ordinary cdecl boundary, not a private or register ABI.

January has three direct calls to point from
`_structure_render_surface_from_point_and_leaf`; three normal and three
incident-radiosity calls from `_lights_distant_lighting_at_point`; three
environment-texcoord calls from `_sample_diffuse_texture`; and three
lightmap-texcoord calls from `_sample_lightmap`. Repository-wide reconstructed
source contains no independent caller or shared-header declaration, so the
definitions self-type locally and no shared header is changed.

The final private translation-unit layouts are readable typed C:

- `struct environment_vertex_compressed` is `0x20` bytes: `real_point3d`
  position at `+0x00`; packed `unsigned long` normal, binormal, and tangent at
  `+0x0C`, `+0x10`, and `+0x14`; and `real_point2d` texcoord at `+0x18`.
- `struct environment_lightmap_vertex_compressed` is eight bytes: packed
  `unsigned long` incident radiosity at `+0x00`, then signed 16-bit lightmap
  coordinates at `+0x04` and `+0x06`.

Those offsets and signed widths are independently demanded by January's
loads and exact emitted bytes, and agree with the c168 layout blobs. Existing
`real_point3d`, `real_vector3d`, and `real_point2d` definitions supply ordinary
float aggregates. The source-local helper declaration has the authenticated
typed signature

```c
real_vector3d *uncompress_int32_to_real_vector3d(
	real_vector3d *result,
	unsigned long compressed);
```

January's callee, caller pushes, returned `EAX` pointer, and surrounding
aggregate copy prove that return and parameter contract. The current target
body for the helper itself is outside this fixed wave and remains undefined
from the candidate object.

## Semantic, assertion, and relocation proof

Every accessor checks both incoming pointers through the repository's natural
`match_assert` expression. January supplies the exact path
`c:\halo\SOURCE\rasterizer\rasterizer_geometry.c`, predicate strings, and
source lines: point `438/439`, normal `450/451`, environment texcoord
`462/463`, incident radiosity `474/475`, and lightmap texcoord `486/487`.
Each assertion lowers to the exact path/predicate COMDATs followed by
`_display_assert` and `_system_exit` in January's order.

Point copies the three position floats. Environment texcoord copies its two
floats. Normal and incident radiosity pass the packed `unsigned long` plus a
typed local result buffer to `_uncompress_int32_to_real_vector3d`, then copy
through the returned pointer exactly as January does. Lightmap texcoord
sign-extends the two `short` fields and naturally evaluates for each channel
`((float)value * 2.0f + 1.0f) * (1.0f / 65535.0f)`. No raw access or donor-only
decode is involved.

The hardened comparator proves identical meaningful and padded code,
normalized hashes, and every relocation's offset, COFF type, destination,
addend, and order. The exact relocation schedules are:

- point: `+0x14,+0x19,+0x1E,+0x25,+0x3B,+0x40,+0x45,+0x4C`;
- normal: `+0x17,+0x1C,+0x21,+0x28,+0x3E,+0x43,+0x48,+0x4F,+0x5F`;
- environment texcoord:
  `+0x14,+0x19,+0x1E,+0x25,+0x3B,+0x40,+0x45,+0x4C`;
- incident radiosity:
  `+0x17,+0x1C,+0x21,+0x28,+0x3E,+0x43,+0x48,+0x4F,+0x5E`;
- lightmap texcoord:
  `+0x14,+0x19,+0x1E,+0x25,+0x3B,+0x40,+0x45,+0x4C,+0x61,+0x67,+0x7B,+0x81`.

The first eight relocations of every body are the two exact assertion groups.
The ninth normal/incident relocation is the helper call. The four remaining
lightmap-texcoord relocations are the target-exact compiler-owned float
literal references for its two calculations. Nothing is accepted from a
similarity score alone.

All three inherited accepted bodies remain independently strict:

| Inherited body | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_uncompress_int8_to_real` | `21 / 32` | 1 | `c8d445f22bb94140d08cc78f9d312c2d55474343075bc7ddcf3caa8d620da8ca` |
| `_uncompress_int16_to_real` | `29 / 32` | 2 | `55efeed54cdddbcddd62789b26e398aa30207db3dd5e87fa475a2787fc7b9886` |
| `_rasterizer_geometry_byte_swap_vertices` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

Direct hardened comparison proves the complete eight-body inventory after
retention.

## One-shot and ownership boundary

The final include, private types, helper declaration, and all five complete
bodies were fixed before code emission. An initial VC7 `/Zs` parse-only pass
correctly diagnosed that the aggregate math definitions were not yet in the
translation unit; adding the canonical `math/real_math.h` include was a
type-check prerequisite and emitted no candidate code. A second `/Zs` pass
succeeded. The candidate object was verified absent, and exactly one natural
five-body candidate compile was then executed. All five bodies were strict in
that object. There was no alternate expression, declaration, topology,
qualifier, literal, local-lifetime, scheduling, or compiler-switch attempt.

The target object has 864 ordinary data bytes. The candidate owns no writable
`.data`, `.bss`, or COMMON symbol. Its runtime `.rdata` ownership is limited
to inherited float literals plus the compiler-owned exact assertion path,
predicate strings, and lightmap arithmetic literals required by accepted
code. Runtime `.rdata` totals 89 bytes; it receives zero standalone data
credit. The only undefined runtime externals are `__fltused`, `_system_exit`,
`_display_assert`, and `_uncompress_int32_to_real_vector3d`.

The candidate's defined external runtime owners are exactly the eight
accepted code COMDATs and their compiler-owned literal/string support. It
introduces no storage owner and leaves the target's remaining data and
nonmatching functions outside scope.

The retained implementation is readable typed defined C. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, cast overlay, pointer or union pun, inactive-union access,
undefined signed overflow, data-dependent representation trick,
private/register ABI, volatile scheduling device, force-inline annotation,
optimizer pragma or barrier, synthetic anchor, or byte-forcing expression.

## Scope and pre-commit validation

Only `source/rasterizer/rasterizer_geometry.c` and this new Jonas-owned ledger
are tracked changes. No shared header, configuration, semantic exception,
parked record, completion label, storage owner, pre-existing Markdown, or
tracked path deletion is changed. The five protected sources remain
untouched: `source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, `source/ai/actions.c`, and
`source/units/units.c`.

- One natural five-body code-producing candidate compile: all five strict on
  the first shot and retained independently; all three inherited bodies still
  strict.
- Complete `halobetacache_build` plus `libcmt_build`: pass, 568/568 Ninja
  edges, with no candidate-source warning.
- Direct hardened final comparison: all eight emitted functions strict,
  including exact padded bytes and relocation semantics.
- Semantic audit: 470 units, 4,149 functions evaluated, 3,580 missing-base
  symbols, 4,009 semantic exact, 101 hidden exact / 61,876 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 local symbols
  skipped, 4,070 accepted exact, and zero unit errors.
- Semantic campaign progress: 375/833 complete objects, 4,057/11,060 exact
  functions, 482,447/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo semantic progress: 273/468 objects, 3,890/7,574 functions,
  469,533/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- `rasterizer_geometry.obj`: 8/19 functions and 639/4,054 meaningful code
  bytes; it remains nonmatching and claims no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, changed-path, source-policy, protected-five, storage,
  and deleted-path checks are clean.

The clean committed-state replay below is intentionally recorded only after
the implementation-and-ledger commit exists. No push, amend, or history
rewrite is performed.

## Clean committed-state replay

Pending the additive replay-only ledger commit.
