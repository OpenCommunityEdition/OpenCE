# `bitmaps.obj` 3D-slice and cube-face copy first shot

## Result

The four still-unwritten copy helpers were evaluated as one bounded natural
cluster.  None is strict semantic-COFF exact, so every candidate body and all
support-only declarations/constants were inverse-reverted immediately.  The
eight inherited exact functions remain the only accepted functions in
`bitmaps.obj`; this wave claims no code or data credit.

| Rejected function | Target meaningful / padded / relocs | Target normalized SHA-256 | First natural candidate meaningful / padded / relocs | Candidate normalized SHA-256 |
| --- | ---: | --- | ---: | --- |
| `_bitmap_3d_slice_extract` | `636 / 640 / 54` | `2255e736a9c54d5f589555a538416f77a9a1046f1ba3321b2fa5ddba827509ca` | `646 / 656 / 54` | `6cd45c7ef911fa643852bdba36b06e5a820d8adcf5fb505f7ad54ffbbde84744` |
| `_bitmap_3d_slice_insert` | `633 / 640 / 54` | `d1561eb79af72f53f927a8c631d7a0c877e1020a671ebda653a9dcd8b3224031` | `643 / 656 / 54` | `f96a74d3ca0f3b528b98d75a9b518f14ffeba53c9480999a271278d67ce443dc` |
| `_bitmap_cube_map_face_extract` | `636 / 640 / 54` | `d7bd09ce972ac9a5bc2dee9daa35d4d8b20902b0cd219a015ee3e51faca03adc` | `641 / 656 / 54` | `fd8cb127ae7d5c0c174c85f887987828bc8e4312f16810f1c9ee72bb027bb10a` |
| `_bitmap_cube_map_face_insert` | `633 / 640 / 54` | `a909efe24dd74ba28bb0f7f36999f832dcddb5e14e36a7dbf8b527ddea71aa89` | `638 / 640 / 54` | `7fe8486dc4ecfab2ed12724d602ce965ac4e41f5873b145a5bf9e3a0f282b63d` |

The candidate cluster's raw object SHA-256 before restoration was
`fe8b3cc570657909ffef0889e47db578f284d579e390cc511f25f65250288106`.
Whole-object hashes include debug metadata; the normalized per-function
hashes and ordered relocation identities above are the rejection authority.

## Evidence and natural candidate

The applicable `bitmaps.obj` ledgers were read before source work.  None
recorded an earlier implementation or attempt for these four symbols.  HCEA's
clean exact-name files provided the ordinary typed copy semantics:

| HCEA source | Payload SHA-256 |
| --- | --- |
| `src/blam/bitmaps/bitmap_3d_slice_extract.c` | `f8b92890d197b72d0c4815f2fb37fcd0ee88c9d073de03bd16c8c20b0a81fbea` |
| `src/blam/bitmaps/bitmap_3d_slice_insert.c` | `eb0bba7e2357f5d045d34d63ed4b3658e3d0b5cd728c1c003a648505704fdbbf` |
| `src/blam/bitmaps/bitmap_cube_map_face_extract.c` | `5392b0a308936d61e95e86a0522f5d9950f91faabc8121610e3aa9192272da5b` |
| `src/blam/bitmaps/bitmap_cube_map_face_insert.c` | `7a6e13044d4d1e188dcdfd52d06fbfd8c54a93a526c20d17baad34b252a6386f` |

Each natural body verified the typed source and destination bitmaps using
January's authenticated file/line strings, checked type, format, mip level,
face or slice, dimensions, mip count, and swizzle state, obtained addresses
through the named bitmap address APIs, and copied the computed pixel-data
size with `csmemcpy`.  Stian Eklund's pinned retail reconstruction at commit
`41c10616b69b982700e0913f21a5137807a03d0e` did not contain these four
definitions and therefore supplied no additional body evidence.

The January split-object SHA-256 is
`6240548bf1ee72e09f15d1b48284092b17b6c096c11e8d294bbf71ae266acb57`.
The compiler was Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
under the repository's unchanged XDK 3911 command and flags.

## Observed boundary

All four candidates preserved the target's 54 relocations, but not their
ordered addresses.  The first extract-side divergence is relocation index 9:
the target file-literal relocation is at `+0x75`, while the candidate emits it
at `+0x7A`.  The first insert-side divergence is relocation index 30: the
target file-literal relocation is at `+0x145`, while the candidate emits it at
`+0x14A`.  The same two five-byte scheduling drifts respectively occur in the
cube-map extract and insert bodies.  Three candidates also exceed the target
padded extent; the fourth has the same padded extent but different normalized
bytes and relocation addresses.

This is sufficient rejection evidence.  No declaration, spelling,
expression-order, control-flow, or compiler-control retry was made.  Reopen
these functions only with independently authenticated ordinary-C source or a
new ABI/topology contract; do not tune them from these rejected objects.

## Policy and restoration

The candidates were readable C89 using named `bitmap_data` fields and public
typed helpers.  They introduced no assembly, pragma, intrinsic, attribute,
volatile/register scheduling device, raw address or byte offset, pointer or
union pun, undefined behavior, object patch, compiler-option change, or tool
exception.  The cluster was compiled once.  After rejection,
`source/bitmaps/bitmaps.c` was restored byte-for-byte through an inverse patch;
only this additive ledger remains from the wave.

The restoration object rebuild passes.  Direct hardened comparison reports
`all_equal: true` for all eight inherited exact sections:
`_bitmap_type_get_string`, `_bitmap_format_get_string`, `_bitmap_changed`,
`_bitmap_delete`, `_bitmap_byte_swap_pixels`,
`_bitmap_get_max_mipmap_count`, `_bitmap_get_pixel_count`, and
`_bitmap_get_pixel_data_size`.

The complete Ninja graph, report, semantic report, and progress regeneration
pass.  The semantic audit scans 470 units, evaluates 4,917 functions, finds
4,800 semantic-exact and 4,810 accepted-exact functions, and reports zero
unit errors.  Campaign progress remains 384/833 linked objects, 4,771/11,060
strict functions, and 606,187/2,198,102 strict code bytes; Halo remains
282/468 linked objects, 4,604/7,574 functions, and 593,273/1,770,166 bytes.
Admission is 0 candidates / 0 contradictions / 0 revocations.  The parked
inventory is 12 active / 0 stale / 0 invalid.  All 212 tooling tests pass and
`git diff --check` is clean.  The regenerated report and semantic-report
SHA-256 values are respectively
`19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`
and
`f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.

The protected Units sentinel remains exact at 1,920 padded bytes, 87
relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
