# `bitmaps.obj` Jonas maximum-mipmap leaf

## Result

This bounded source-only wave starts from authoritative integration commit
`c8ab755c312be6e9c4eb3cd0d9e6c6aba643c45e`. Five HCEA-backed public mipmap
helpers were emitted together in one natural candidate compile. One was
independently strict and is retained. The four nonexact bodies and their
support-only constants were removed immediately, without a spelling,
declaration, control-flow, or compiler-control retry. `bitmaps.obj` advances
from 3/34 to 4/34 exact functions and remains `NonMatching`.

| Retained function | January RVA | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `_bitmap_get_max_mipmap_count` | `0x0006BCC0` | 135 | 144 | 7 | `3fecf1ff38f2c1b9b13088a97303447e2923b9ec9aa6567bcebc2ab8ea1d0725` |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends. The object's four
accepted functions now account for 366/10,010 meaningful code bytes. This
wave adds one function and 135 meaningful code bytes. The target's remaining
30 functions, 4,796 non-code bytes, and all runtime storage remain outside
this wave's credit.

## Stable provenance and ABI

- Baseline `source/bitmaps/bitmaps.c` blob/SHA-256:
  `780a92ff80582c7d729b9017acfcf2ac7f610e9b` /
  `381cd866c4a282de0d7c7e16ab03bba8ab8602e7582383b3cbf15bea95772da2`
  (12,742-byte committed payload).
- Retained source blob/SHA-256:
  `020122fa4d6026da574245fe11dff6ffe0b52a54` /
  `5ef110436f661f747abf281b5a54ab3623202f182a67e1fa238f81ff98238881`
  (13,257-byte committed payload).
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `6240548bf1ee72e09f15d1b48284092b17b6c096c11e8d294bbf71ae266acb57`.
- Pre-commit retained candidate-object SHA-256:
  `6078a1bc7b384310ed4125538addb75692c223153167ad1f7d1f393e5fe04d03`.
- Committed delete/rebuild candidate-object SHA-256:
  `b877999576947815ed8dc7704d12569667911fe4fa759a13bd4f2fff39cf297e`.
  These phase-specific whole-object hashes include compiler debug metadata;
  the normalized function hashes and relocation identities are the stable
  acceptance authority.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name source files
  are tree-reachable with these stable blob/payload identities:

  | HCEA file | Blob | Payload bytes | Payload SHA-256 |
  | --- | --- | ---: | --- |
  | `src/bitmap_get_max_mipmap_count.c` | `aaf4c12658a5bc96c0ef184cba2e1aee47faff7b` | 903 | `4890af786f3ec56137b310f940d86a8cd8a8c56034b3964afc15fe1782c25a31` |
  | `src/bitmap_mipmap_get_width.c` | `0fb448580269ad24cda71bf01fa2b652662e52a3` | 672 | `fdba81cb083df4d48717e5aef142c57b766ed81f04597ba439e0075f0ca3b24f` |
  | `src/bitmap_mipmap_get_height.c` | `5d8d1d8828b80e4eb12dc74dcae2ff7ea46529b7` | 682 | `141abacaf26883557201a6e0b2cfb5d5b827707660a717ff9c3dc2b408bcce99` |
  | `src/bitmap_mipmap_get_depth.c` | `3f7ad166ba4a23a5e58754a7d964a5186a4d9cf8` | 251 | `c02628e4faa2588b6d134e7c17d6b8941d39d6ac58895e50dac829c93e48d875` |
  | `src/bitmap_mipmap_get_pixel_count.c` | `b8bb3ea9370d5331b0a5ca210bf9ff4ed106d806` | 1,141 | `d1e8bb025514e52c09cfd9c68be9f9eb9eb8df94fa4dd39df4420dff64b3d6fd` |

- Stian's independently readable PC bitmap topology is pinned at commit
  `41c10616b69b982700e0913f21a5137807a03d0e`,
  `src/halo/bitmaps/bitmaps.c` blob
  `6410e8c031a8c2abf8c9b0ac13571ef6988a0770`, payload SHA-256
  `fa694d33004c24ebc53b96de0f72e45ef6bda0ca645a082226b7bddc94fea6f3`
  (26,583 bytes). It corroborates the public mipmap topology; no raw-address
  or byte-offset access is copied.
- The unchanged typed layout comes from `source/bitmaps/bitmap_group.h` blob
  `9d5ee26d625b9895ba8d88ca02f91b7066165057`, payload SHA-256
  `b86ddcd46e4090df435eee5e47c248efd6ca746c7596bd838304bd73d90e72b5`.
  It fixes `bitmap_data` as a real structure with signed-short width/height,
  unsigned-short depth/flags/mipmap count, and the remaining named fields.
  The unchanged `floor_log2(long)` declaration comes from
  `source/math/integer_math.h` blob
  `3b8a46d60cfb6440085cd5c818250edde30caf5e`, payload SHA-256
  `4819125a698f6c6c770c360790479775e9694f1cf9914f13bb94a7103b0a5cab`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  under the repository's unchanged XDK 3911 command and `/O2 /Oy- /DDEBUG
  /Dxbox` flags.

The retained signature is ordinary public `cdecl` C:

```c
short bitmap_get_max_mipmap_count(
	struct bitmap_data *bitmap);
```

No existing cross-TU caller or shared declaration names this currently absent
body. The local non-const `bitmap_verify(struct bitmap_data *, boolean)`
prototype agrees with the existing declaration in `bitmaps_quantitize.c` and
January's one-pointer/one-boolean call ABI. No header repair is needed.

## Authenticated behavior and relocation identity

The function initializes its result to zero, verifies the typed bitmap without
repair, and only computes a mipmap count when the authenticated power-of-two
dimensions flag is set. It passes the maximum of signed width, signed height,
and the explicitly signed depth value to `floor_log2`, then returns the signed
short result explicitly.

Its seven exact relocations are:

| Function offset | Type | Destination |
| ---: | --- | --- |
| `+0x0D` | `REL32` | `_bitmap_verify` |
| `+0x20` | `DIR32` | compiler file-name literal |
| `+0x25` | `DIR32` | `bitmap_verify(bitmap, FALSE)` literal |
| `+0x2A` | `REL32` | `_display_assert` |
| `+0x31` | `REL32` | `_system_exit` |
| `+0x5D` | `REL32` | `_floor_log2` |
| `+0x75` | `REL32` | `_floor_log2` |

The two `floor_log2` calls are January's natural nested-maximum emission, not
a source anchor. The final candidate object contains the four accepted code
COMDATs, existing source-owned palette/tables, ordinary compiler literals,
debug metadata, and unresolved typed calls. The wave adds no source-defined
global, `.data`, `.bss`, COMMON symbol, or runtime storage. Compiler-owned
assertion text receives no data credit; campaign matched data remains
1,835,088 bytes.

## One-shot rejection record

All five candidates were compiled together exactly once. The four misses were
removed without retry, and all four rejected symbols are absent from the final
COFF symbol table:

| Rejected function | January RVA | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | ---: | --- | --- |
| `_bitmap_mipmap_get_width` | `0x0006BD50` | `153 / 160 / 9` | `23d665ecb68ff9891e6bc9e06ceb67e57b1c33093164e19cd221c99ee4ecaf2f` | `160 / 9`, SHA `9bf33321a83b0e92f73740ac661cb233242e3ded4a21a55a22db1d49f1a40c3b`; its second assertion relocation block is five bytes later than January's. |
| `_bitmap_mipmap_get_height` | `0x0006BDF0` | `153 / 160 / 9` | `832b9bfe3bb3f5d40095d322069ed2ab8255ad6fe4cc5fee6d2ad48ba382f686` | `160 / 9`, SHA `8edb14689d3adb42ca49fc4fb5776145b8cc691bdf3d09bc2a57c969b042ab88`; it has the same five-byte second-assertion schedule drift. |
| `_bitmap_mipmap_get_depth` | `0x0006BE90` | `141 / 144 / 9` | `9a864fce4b366f198a5f4b9cfdb1d2df5753fb5c12473a8c9865c746f5eb4ff4` | `160 / 9`, SHA `e55a7edd529af1148e32ed0c26306923347bddf4ec3bdc2ca77bbcb0bdfd8ba7`; both padded extent and the second assertion schedule differ. |
| `_bitmap_mipmap_get_pixel_count` | `0x0006BF20` | `164 / 176 / 12` | `ad3adfeb2cdfb5185c6b515b40fc6ee5a3bcb05f974319280aba43352ae988f4` | `176 / 12`, SHA `980581d88d02e8e75a46829efe3736e01933bafd9353ea3c65f8193d8d6092a5`; VC7 evaluates depth/height/width while January's call order is width/height/depth, in addition to the assertion drift. |

For width and height, the candidate's second assertion relocation block is at
`+0x53,+0x58,+0x5D,+0x64`; January's is at
`+0x4E,+0x53,+0x58,+0x5F`. Reopen these four bodies only with independently
preserved January source or a newly authenticated ordinary-C contract; do not
tune their spelling or scheduling from the rejected object.

## Policy, scope, and validation

Only `source/bitmaps/bitmaps.c` and this new Jonas-owned ledger are changed.
No header, configuration, storage definition, semantic exception, parked
record, completion label, protected-five source, pre-existing Markdown,
Claude-owned file, or deletion is included.

The retained implementation is readable typed scalar C with one parameter per
declaration line and an explicit terminal return. It contains no assembly, raw
address or byte-offset dereference, pointer/integer reconstruction, pointer or
union pun, inactive-union access, undefined signed overflow, volatile
scheduling device, force-inline annotation, optimizer pragma/barrier,
synthetic anchor, or object-byte forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the new body and all three inherited accepted
  functions pass; all four rejected candidate symbols are absent.
- Semantic audit: 470 units, 4,113 functions evaluated, 3,973 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,034 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,021/11,060 exact functions,
  479,683/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds one function and 135
  meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check`, protected-
  source, scope, and banned-construct checks are clean.

A clean committed-state one-unit regression snapshot at
`ca880dbf7c83478112d1ee6125e4ae4e07adf294` and its initial check passed.
Verified deletion and forced rebuilding of `bitmaps.obj` then reproduced all
four accepted functions exactly; the post-rebuild regression check reports
`changed_nonexact: []`, all four under `still_exact`, and no failures or
warnings. No push is performed.
