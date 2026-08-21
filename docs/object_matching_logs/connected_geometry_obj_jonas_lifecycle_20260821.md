# `connected_geometry.obj` Jonas lifecycle wave

## Result

This bounded natural-C wave starts at authoritative integration commit
`1d593855fe455ea12b636e1909eff9f2f122ac33` and recovers both requested
lifecycle functions in `source/tool/connected_geometry.c`. Each function was
independently strict on the first and only code-producing candidate compile.
The incomplete unit advances from 0/10 to 2/10 exact functions, gaining 122
meaningful and 128 padded code bytes. It remains `NonMatching`, and no data
credit is claimed.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 | Decision |
| --- | ---: | ---: | --- | --- |
| `_connected_geometry_new` | `43 / 48` | 3 | `d85f2504e2ffcdd236b7e29ae2beda66e0f3ad33a7b3cef3bc6292946d4200c1` | **retained: strict exact** |
| `_connected_geometry_delete` | `79 / 80` | 5 | `e74305d7f27a6a8ad73684af18843186b6e4f76971a75f30828609519dbb1c7f` | **retained: strict exact** |
| **Wave delta** | **122 / 128** | **8** | | |

The hardened COFF comparator proves equal padded bytes, relocation
count/order/type, semantic relocation destinations and addends, and normalized
bytes for both retained functions.

## Scope and provenance

- Baseline `connected_geometry.c` blob:
  `bd0916cb02d2cb9b291f25661119d1f7a8b3107d`.
- Final `connected_geometry.c` blob/committed-blob SHA-256:
  `aa946fd89e24827de07ee3b2dd2f8747994fedde` /
  `865bce2e860d5b9c7c9e1c64283deadb40b22ebf45527f470d581fc7425a4866`.
- Unchanged `memory/array.h` blob:
  `72b8e1ecfdf9541c3b8dc70448b04dbdf8bdff8c`.
- Unchanged `structure_lens_flares.c` blob:
  `e25c9086649f62b762eb664a8acc4394565250c0`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `connected_geometry.obj` SHA-256:
  `bd9bcffee0f23780214492f62593ad478315de2246564c4e8d562cfc9b424429`.
- January PDB2.00 symbol payload SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
  It independently labels `_connected_geometry_new` at RVA `0xF2CD0` and
  `_connected_geometry_delete` at RVA `0xF2D00`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and include paths.

January labels and COFF, the current lens-flare caller, and the existing array
API jointly authenticate the complete public signatures:

```c
void connected_geometry_new(struct connected_geometry *geometry);
void connected_geometry_delete(struct connected_geometry *geometry);
```

The current caller allocates a `connected_geometry` on the stack, initializes
it before triangle construction and grouping, accesses its three typed dynamic
arrays, and deletes it afterward. Its local declarations establish the array
order and offsets, the 0x1C edge layout including `point_indices` at +0xC, and
the 0x18 triangle layout. `memory/array.h` independently supplies the 0xC
`dynamic_array` layout and ordinary typed API declarations. Source-local
compile-time assertions prove all relied-on sizes and offsets without changing
a shared header.

## Authenticated lifecycle behavior

`connected_geometry_new` initializes the arrays at geometry offsets +0x00,
+0x0C, and +0x18 with element sizes 0x0C, 0x1C, and 0x18 respectively. Its
three `REL32` relocations target `_dynamic_array_new` at section offsets
+0x0B, +0x16, and +0x21.

`connected_geometry_delete` iterates the signed-long edge index against the
edge-array count at geometry +0x10. Each iteration calls
`dynamic_array_get_element` on geometry +0x0C with element size 0x1C, then
deletes the edge's embedded triangle-index array at edge offset zero. After
the loop it deletes the geometry arrays at +0x00, +0x0C, and +0x18. Its
relocations target `_dynamic_array_get_element` at +0x1A and
`_dynamic_array_delete` at +0x20, +0x32, +0x3B, and +0x44.

Both typed bodies were emitted together once. Both passed direct strict COFF
comparison immediately; there was no rejected body, source variant,
declaration/expression permutation, or code-generation tuning. No call to
`plane3d_from_points` was introduced.

## Storage, ownership, and policy boundary

The final candidate object contains only `.drectve`, `.debug$S`, and the two
function `.text` COMDATs. It defines no `.bss`, `.data`, COMMON storage, or
runtime `.rdata`. `dynamic_array_new`, `dynamic_array_get_element`, and
`dynamic_array_delete` remain ordinary undefined function externals. The
January target's one `.bss` byte and 136 `.rdata` bytes remain wholly
uncredited.

Only `source/tool/connected_geometry.c` and this new Jonas-owned ledger change.
No shared header, storage definition, caller, configuration, frozen-five
source, semantic exception, parked record, pre-existing Markdown, or
Claude-owned file is changed; no path is deleted. The retained implementation
uses TU-local typed structures, named fields, compile-time layout checks, and
the existing array APIs. It contains no assembly, `volatile` scheduling,
force-inline annotation, optimizer pragma/barrier, raw address, byte-offset
dereference, pointer or union pun, inactive-union read, undefined behavior,
synthetic anchor, or object-byte/code-generation forcing.

## Validation and disposition

- Complete `halobetacache_build` and `libcmt_build`: pass, 568 remaining
  actions after the sole candidate-object compile.
- Direct strict comparison: 2/2 retained functions pass.
- Strict semantic audit: 470 units, 4,056 functions evaluated, 3,916 semantic
  exact, 101 hidden exact / 61,876 hidden code bytes, 3,977 accepted exact,
  and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,964/11,060 functions,
  476,018/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes. Halo contributes 3,797/7,574 functions and
  463,104/1,770,166 code bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check` and the final banned-policy scan are clean.

A clean committed-state snapshot/check followed by deletion and forced rebuild
of `connected_geometry.obj` is required before handoff and is recorded there.
No push is performed.
