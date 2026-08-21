# `rasterizer_xbox_dynavobgeom.obj` Jonas public trio (2026-08-21)

## Result and fixed boundary

This Jonas-owned source-only wave starts from exact clean canonical commit
`a336a437e38ceab3ad334486f12f260aa4a51447` on isolated branch
`jonas/rasterizer-dynavobgeom-trio-20260821`. The fixed boundary contained
exactly three ordinary-C bodies. All three were independently strict in the
first and only natural three-body code-producing candidate compile and are
retained:

| Retained C body / COFF symbol | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_hud_begin` / `__rasterizer_hud_begin` | `0x0014EA40` | `9 / 16` | 1 | `8b1f5f34f4c0d2ca6b680313b07c6b9e71c86375df3c9a2784367506b294816e` |
| `_rasterizer_hud_end` / `__rasterizer_hud_end` | `0x0014EA50` | `9 / 16` | 1 | `8b1f5f34f4c0d2ca6b680313b07c6b9e71c86375df3c9a2784367506b294816e` |
| `_rasterizer_dynamic_screen_geometry_draw` / `__rasterizer_dynamic_screen_geometry_draw` | `0x0014ED90` | `33 / 48` | 4 | `748306c90c14ca2c6acddd522d65c08c9737f4a44bf4207fb1f21573415db6f7` |

The unit advances from 1/17 to 4/17 exact functions and from 1/5,112 to
52/5,112 meaningful code bytes. It remains `NonMatching`. The strict net gain
is 51 meaningful bytes, 80 padded bytes, and six exact relocations. No data
credit is claimed.

## Immutable target, source, and compiler provenance

- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Fresh January csplit
  `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.obj`: 16,941 bytes,
  SHA-256
  `5ace09256f8908f347c225b784fc2fff59c8469d07cf9871e22af012f5374825`.
- Exact-base translation-unit Git blob
  `ce30a063b2de2dc4c2a25431894eee1437a3c65a`, 2,255 raw payload bytes,
  payload SHA-256
  `00b8d7a36afcdac93a90bb68f541065fffa8f85e0863969e873a64dfc1474989`.
- Retained staged translation-unit Git blob
  `59998ac653193eab009533938261656e897e1e44`, 2,858 raw payload bytes,
  payload SHA-256
  `9e5eeaf7dc1356dbef48412b996f41fce4923f0cade9bb5134584b2e41171c9a`.
  These hashes cover raw Git-blob payloads, not the CRLF working-tree view.
- The immutable first-shot and final pre-commit object is 2,166 bytes with
  phase-specific SHA-256
  `6bebcc200901e1d70561621fd7c7afd857d0102ff3148c7b3700575e3f8a950f`.
  The pristine inherited-only object was 778 bytes with SHA-256
  `4b911b261b0ffe6a44a81bda262d6ced252b964e30f5cd5fd9ebaf308ed617cb`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Natural repository flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`
  with the unchanged XDK 3911 include graph.

The current declaration and macro owners were pinned before emission:

| Current source evidence | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `source/cseries/cseries.h` | `3c91ac46ab275894a18ac4e839b38ae13022d91b` | 10,070 | `e994e965cd3f48cd47cebf0a39f41a1aac460e8b25544452a1fc4b302a0b7043` |
| `source/rasterizer/rasterizer.c` | `8b67d32b5efdcfd78c7119d547eb9d919850958f` | 57,284 | `7e120cbdfc1e8839aaffd6dab46eabf38091687f74300fa4f1305c622f88775a` |
| `source/rasterizer/rasterizer.h` | `1273896ad17902147078205e248461450de65d34` | 17,744 | `a45c30da6c661379befb19b2b463541d83667091e1dce9961ab39cd47fe505ae` |
| `source/rasterizer/xbox/rasterizer_xbox.h` | `c41ec24a916b27759cda6d856783152e11292b2f` | 637 | `063f7f6b1f04557435c6f57c15e15c8b2ba2cc724eda1967ea3f28b30224bd74` |

The clean HCEA reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Every identity below is directly
tree-reachable at that commit, and each SHA-256 covers raw
`git cat-file blob` bytes:

| Clean c168 HCEA evidence | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `src/_rasterizer_hud_begin.c` | `fd6fde4c10f2c89a9565b61381aa893b5652e1ce` | 235 | `cbee17c616dcbb1cd788acaf8d806c15a45519d9238a4aa12b89a34093b07143` |
| `src/_rasterizer_hud_end.c` | `3613ef8bc8123fde19bbe1b9098c5b2be9321d33` | 182 | `68f0500c2836049b1d1eb5c3fbf748e4b2dca6b46b8a2802f439c224c6876e47` |
| `src/_rasterizer_dynamic_screen_geometry_draw.c` | `3efb117c67ad969eb245ece1b23dc0f65fe362f2` | 582 | `ddfa0da8f85eb2262c9f08be0f1f185a37b8948f4b6a466f939a9d5ca54eacd4` |

HCEA supplies readable provenance only. Its HUD leaves are donor-platform
no-ops and its dynamic-screen leaf has a different parameter domain and is a
bare stub. None of those donor-only bodies was promoted. January PC COFF
alone governs profile calls, profile ID 27, the four-long signature, assertion
metadata, code extents, padding, and relocation order.

All-revision history contains no prior definition of any of the three fixed
bodies in this translation unit. Canonical history changes the file only to
add the disjoint inherited dynamic-lit leaf. Repository-wide source and live
scope checks found no competing implementation of this trio. Every other
target body in `rasterizer_xbox_dynavobgeom.c` was explicitly excluded before
emission.

## Typed ABI and complete caller proof

The retained contracts are:

```c
void _rasterizer_hud_begin(void);
void _rasterizer_hud_end(void);
void _rasterizer_dynamic_screen_geometry_draw(
	long primitive_type,
	long vertex_type,
	long dynamic_vertex_buffer_index,
	long vertex_count);
```

On this 32-bit target ABI, every `long` formal is a 32-bit stack value. The
target leaves return with ordinary `ret`; none consumes a hidden or
register-carried formal. A complete relocation census over every January
split object finds exactly one direct caller for each body, all in
`source/rasterizer/rasterizer.obj`: `_rasterizer_hud_begin` calls
`__rasterizer_hud_begin` through `REL32 +0x01`, `_rasterizer_hud_end` calls
`__rasterizer_hud_end` through `REL32 +0x01`, and
`_rasterizer_dynamic_screen_geometry_draw` calls the double-underscore leaf
through `REL32 +0x05`. The first two wrappers tail-jump. The four-argument
wrapper establishes a frame, restores it, and tail-jumps while preserving all
four caller stack slots. Current reconstructed wrapper declarations and calls
use these exact signatures. C identifiers beginning with `_rasterizer`
naturally produce the target's double-underscore COFF decorations.

The authenticated `rasterizer_xbox.h` declarations give both profile helpers
the ordinary `void(short)` contract. Existing reconstructed Xbox rasterizer
translation units use equivalent source-local prototypes, so this source-only
owner does the same without altering a shared header. January's HUD bodies
each push signed immediate 27, call the respective helper, clean the single
stack argument, and return. Their exact schedules are `REL32 +0x03` to
`_rasterizer_profile_begin` and `_rasterizer_profile_end` respectively.

The dynamic-screen body deliberately ignores its four ordinary arguments.
January unconditionally reports the exact information string
`"_rasterizer_dynamic_screen_geometry_draw not supported no mo'"` at exact
path `"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_dynavobgeom.c"`,
line 255, fatal `TRUE`, and then calls `system_exit(-1)`. The current
`match_vassert(file, line, FALSE, information)` macro expands to exactly that
defined control flow. Its complete relocation schedule is:

| Offset | COFF type | Destination |
| ---: | --- | --- |
| `+0x08` | `DIR32` | 61-byte path-literal COMDAT |
| `+0x0D` | `DIR32` | 62-byte information-literal COMDAT |
| `+0x12` | `REL32` | `_display_assert` |
| `+0x19` | `REL32` | `_system_exit` |

Direct hardened comparison proves exact padded bytes plus every relocation's
address, type, destination, addend, and order. No similarity score is used as
an acceptance oracle.

## Preserved body and ownership boundary

The inherited body remains independently strict:

| Exact function | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_dynamic_lit_geometry_draw` | `0x0014EA60` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| **Final four-body total** | | **`52 / 96`** | **6** | |

The final object defines exactly these four external code COMDATs. Its only
runtime non-code owners are the exact 61- and 62-byte compiler-owned assertion
literal COMDATs required by the retained dynamic-screen body. It defines no
writable `.data`, `.bss`, or COMMON owner. The only undefined runtime
externals are `_rasterizer_profile_begin`, `_rasterizer_profile_end`,
`_display_assert`, and `_system_exit`. The literals receive zero standalone
data credit, and all 525 target non-code bytes remain unclaimed by this wave.

The retained implementation is readable typed defined C. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, cast overlay, pointer or union pun, inactive-union access,
undefined signed overflow, private/register ABI, volatile scheduling device,
optimizer pragma or barrier, synthetic anchor, or byte-forcing expression.

## One-shot discipline and pre-commit validation

The include, two source-local helper prototypes, and all three complete bodies
were frozen before emission. Exactly one VC7 `/Zs` parse-only check passed and
left the 778-byte pristine object unchanged. Exactly one natural three-body
code-producing candidate compile followed. All three bodies and the inherited
body were strict in that immutable object. No candidate was removed, tuned,
rewritten, or recompiled. The subsequent full production build merely reused
and reproved the unchanged first-shot object.

Only `source/rasterizer/xbox/rasterizer_xbox_dynavobgeom.c` and this new
Jonas-owned ledger are tracked changes. No shared header, configuration,
storage owner, semantic exception, parked record, completion label,
pre-existing Markdown, or tracked deletion is changed. The five protected
sources remain untouched: `source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, `source/ai/actions.c`, and `source/units/units.c`.

- Complete `halobetacache_build` plus `libcmt_build`: pass, all 568 remaining
  production edges. Diagnostics are inherited repository warnings only.
- Direct hardened comparison: `all_equal: true` for the complete four-body
  final inventory after the full graph.
- Semantic audit: 470 units, 4,161 functions evaluated, 3,568 missing-base
  symbols, 4,021 semantic exact, 101 hidden exact / 61,876 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 locals skipped,
  4,082 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,069/11,060 exact functions,
  482,978/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes.
- Halo progress: 273/468 objects, 3,902/7,574 functions,
  470,064/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- This unit: 4/17 exact functions and 52/5,112 meaningful code bytes; it
  remains nonmatching and claims no standalone data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass. `git diff --check`, changed-path,
  deleted-path, protected-five, source-policy, and storage checks are clean.

The clean committed-state replay is recorded additively only after the
implementation-and-ledger commit. No push, amend, or history rewrite is
performed.

## Clean committed-state replay

Pending implementation-and-ledger commit and one-unit forced replay.
