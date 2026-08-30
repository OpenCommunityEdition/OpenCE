# `bitmaps.obj` palette-match first shot

## Result

`_palette_find_closest_match` was evaluated as one history-unattempted,
link-closed natural C89 candidate.  The candidate is not strict
semantic-COFF exact, so its body was inverse-reverted immediately.  The eight
inherited exact functions remain the only accepted functions in
`bitmaps.obj`; this wave claims no code or data credit.

| Function | Target meaningful / padded / relocs | Target normalized SHA-256 | First natural candidate meaningful / padded / relocs | Candidate normalized SHA-256 |
| --- | ---: | --- | ---: | --- |
| `_palette_find_closest_match` | `245 / 256 / 4` | `bff02f627a7490e89413a9c25091762766ee6c4d5a197b895d3f9b7675573b9e` | `233 / 240 / 4` | `a1b2f349fe4ab83c914149510b5c264567f416debd006d6a190904878d681357` |

The rejected candidate's raw whole-object SHA-256 was
`46289cb9c40580c7cdc693f47c7c4c29b265928e9f482a492b9988189ae1d51d`.
Whole-object hashes include debug metadata; the normalized function hash,
padded extent, and ordered relocations are the rejection authority.

## Provenance and natural candidate

All applicable `bitmaps.obj` ledgers and repository tooling and methodology
documents were read before source work.  Canonical ancestry contains no
earlier production implementation or candidate for this symbol.  The old
`jonas/bitmaps-palette-20260820` branch adds palette data only and likewise
contains no function body.

Two independently readable public reconstructions establish the natural
behavior:

| Source | Commit | Blob / payload evidence |
| --- | --- | --- |
| HCEA, `src/palette_find_closest_match.c` | `c168af2e747d3095d9a29418ae401f3a39544863` | blob `72aa2acb2b59ed0590d9f6532b01ca026797b2ca`; SHA-256 `c882be3d035b6ea2c390eed79aa28b1d72056bbe3466588c7ac111c03a6cfbe6` |
| Stian Eklund, `src/halo/bitmaps/bitmaps.c` | `fc11d861865735877d5b8b4c408b1a0c06dde3cf` | blob `c60dee7d67edef7ff75c92ebe0066552e3107ec2`; whole-file SHA-256 `7f9eb3bf7dd01b5fdb2e365425791a04c04f68c231c48e5c734ad0c4db2df2ba` |

Both sources agree on the alpha-zero early return, palette traversal, squared
RGB distance, and nearest-index update.  Stian's body additionally agrees
with January's target assertion and return type.  The one production
candidate used this repository's typed `pixel32` values, initialized the
closest index and distance before the alpha guard, iterated the palette in
ordinary signed C, computed the three channel deltas with named masks and
shifts, updated the nearest match, and retained January's authenticated
`closest_match_index!=NONE` assertion.

The public Stian body views a packed pixel through a `byte const *`.  That
representation cast is not admissible under the current typed-source policy,
so the sole candidate used shifts for every channel instead.  No
representation pointer, union pun, raw byte offset, or other byte-forcing
device was introduced.

The January split-object SHA-256 is
`6240548bf1ee72e09f15d1b48284092b17b6c096c11e8d294bbf71ae266acb57`.
The compiler was Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`,
SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
under the repository's unchanged XDK 3911 command and flags.

## Strict boundary

All four relocation destinations, types, and order agree, but every candidate
relocation is fourteen bytes early:

| Ordered destination | Target offset | Candidate offset |
| --- | ---: | ---: |
| compiler file literal `c:\\halo\\SOURCE\\bitmaps\\bitmaps.c` | `+0xD5` | `+0xC7` |
| assertion literal `closest_match_index!=NONE` | `+0xDA` | `+0xCC` |
| `_display_assert` | `+0xDF` | `+0xD1` |
| `_system_exit` | `+0xE6` | `+0xD8` |

The candidate allocates a 12-byte frame where January allocates 8 bytes and
keeps the palette index in EBX where January keeps the input color.  January
forms a palette-entry address and loads its red and green channels as named
bytes; the policy-clean candidate instead emits integer shifts and masks.
That difference accounts for the shorter 233-byte candidate and the uniform
fourteen-byte relocation drift.  Complete normalized comparison reports 243
different byte positions once the extent mismatch is included.

This is a durable first-shot boundary, not a tuning invitation.  Do not retry
the rejected all-shifts body.  Do not introduce a byte-pointer cast, union or
channel representation pun, raw byte offsets, undefined behavior, or
compiler-control device to force January's loads.  Reopen only with original
January source or a policy-approved owner-defined packed pixel-channel type
or helper that naturally emits the target.

## Restoration and gates

The readable candidate introduced no assembly, pragma, intrinsic, attribute,
volatile/register scheduling device, raw address or offset, pointer or union
pun, undefined behavior, object patch, compiler-option change, tool
exception, or comparator exception.  It was compiled once.  After rejection,
`source/bitmaps/bitmaps.c` was restored exactly to canonical blob
`d902025d4b269a4686d77989fc1f1591a34913bc`; only this additive ledger
remains.  The restored rebuilt raw `bitmaps.obj` SHA-256 is
`bc906bf91e8c1b6fb8d97bcfe52430f6619d28eed3c37d94f3b5280386e6e655`.

Direct hardened comparison preserves all eight inherited exact sections:
`_bitmap_type_get_string`, `_bitmap_format_get_string`, `_bitmap_changed`,
`_bitmap_delete`, `_bitmap_byte_swap_pixels`,
`_bitmap_get_max_mipmap_count`, `_bitmap_get_pixel_count`, and
`_bitmap_get_pixel_data_size`.

The complete `halobetacache_build`, `libcmt_build`, report, semantic report,
and progress graph passes.  The semantic audit scans 470 units, evaluates
4,917 functions, finds 4,800 semantic-exact and 4,810 accepted-exact
functions, and reports zero unit errors.  Campaign progress remains 384/833
linked objects, 4,771/11,060 strict functions, and 606,187/2,198,102 strict
code bytes; Halo remains 282/468 linked objects, 4,604/7,574 functions, and
593,273/1,770,166 bytes.  Admission is 0 candidates / 0 contradictions / 0
revocations.  The parked inventory is 12 active / 0 stale / 0 invalid.  All
212 tooling tests pass.  The regenerated report and semantic-report SHA-256
values are respectively
`19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`
and
`f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.

The protected Units sentinel `_unit_preprocess_node_orientations` remains
strict exact at 1,920 padded bytes, 87 relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
