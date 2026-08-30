# `s3tc.obj` `DecodeBlockAlpha4` exact ownership boundary (2026-08-30)

## Result

The first and only natural production candidate for
`source/bitmaps/s3tc/s3tc.obj::_DecodeBlockAlpha4` was strict function-exact:

| Evidence | January target | First candidate |
| --- | ---: | ---: |
| Meaningful / padded code bytes | 131 / 144 | 131 / 144 |
| Ordered relocations | 1 | 1 |
| Normalized SHA-256 | `f386c8c61c53347d44665a7f363d34f2099576f058d76ab3484b033b183026eb` | `f386c8c61c53347d44665a7f363d34f2099576f058d76ab3484b033b183026eb` |

The relocation is `IMAGE_REL_I386_REL32` at owner offset `+0x12` and resolves
to `_DecodeBlockRGB+0` in both objects. Direct hardened comparison reports
`all_equal: true` for the new decoder and for the inherited exact
`_DecodeBlockAlpha3` owner (606 / 608 bytes, one relocation, normalized
SHA-256 `0dd23ce088e93017f7424049431b394d4d44e2730f3e8b0835aaee58e79a6a0d`).

The candidate is nevertheless not retained. The fail-closed whole-TU
regression manifest reports the intended `_DecodeBlockAlpha4` as
`NEWLY_EXACT`, but also reports changed accepted-function evidence for
`_DecodeBlockAlpha3`, changed `.debug$S` evidence, and changed symbol
ownership. Adding the authentic Alpha4 body before Alpha3 changes the section
and symbol inventory that owns the previously accepted Alpha3 COMDAT. Under
the campaign's complete-ownership rule, direct function equality is necessary
but not sufficient.

Production source and header were inverse-restored. This ledger grants no
function, byte, data, or object credit.

## Source provenance and sole candidate

The integration base was
`ff673c88c698326382a9d7ddcd1e0a8442f4c314`. All current S3TC ledgers,
ref history, worktree source census, campaign methodology, comparator notes,
and locally reachable Claude documentation were checked before the edit. No
previous production attempt or ledger named `_DecodeBlockAlpha4`.

Two independent source oracles agree on the same ordinary decoder:

- Microsoft's preserved Direct3D reference rasterizer at commit
  `5c6fe3db626b63a384230a1aa6b92ac416b0765f`, file
  `multimedia/directx/dxg/ref8/common/dxtn.cpp`, blob
  `7a2508f7c313dc77aca8debc2936bf000a83edab`;
- HCEA reconstruction commit
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
  `src/blam/bitmaps/DecodeBlockAlpha4.c`, blob
  `3536bc2e6c1db0ab0237a0892ad5a22996a0e36c`, payload SHA-256
  `e8089e67300f7a64e1dc34bc6a4bba8d64d7f87f5e47d26f8d5457e338bef510`.

The Microsoft source is the controlling provenance. It defines a 16-byte
Alpha4 block with four 16-bit alpha rows followed by the eight-byte RGB block,
calls `DecodeBlockRGB`, then uses nested four-iteration row and column loops.
Each low nibble is expanded with `((alpha & 0xF) << 4) | (alpha & 0xF)` before
the row word shifts right by four. The candidate translated those exact types,
statements, loop order, and source order into the repository's named C89
structures. The header asserted both the RGB offset (`0x8`) and full block size
(`0x10`).

The candidate source/header Git blobs were respectively
`8a7a5ecd607680fbfaea18b8f1c1b20e986f7979` and
`48a2dab1d47c57e390a05ff561a2c184d201c0d6`. The first compiled object raw
SHA-256 was
`b6cdb153da94966ce23196b3dba817bb61b6b1c25a3bb7a5bf3f80fe2e46d0f3`.

The implementation was readable typed C89 with one parameter per line and an
explicit terminal `return;`. It used no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, barrier, raw address or offset access, pointer or
union pun, undefined behavior, compiler-option change, synthetic anchor, or
byte/code-generation forcing.

## Retention boundary and restoration

The authentic Microsoft order places `DecodeBlockAlpha4` before
`DecodeBlockAlpha3`; January's symbol inventory has the same order. Moving the
new body after the inherited exact function might preserve the old candidate
owner numbering, but it would be a second code-generation/ownership tuning
shot and would contradict the authenticated source order. It was not tried.
No section directive, duplicate declaration, synthetic caller, ownership
adjudication, or comparator exception was considered.

Both production edits were inverse-applied. The final source and header Git
hashes equal the integration base exactly:

- `source/bitmaps/s3tc/s3tc.c`:
  `5a7a2015ff994a1462a8a7a0767adb5ca51fb879`;
- `source/bitmaps/s3tc/s3tc.h`:
  `711c9a9342463681c4a314a1f9c436590399391f`.

After restoration, the unit returns to 1/18 strict functions with 17 unwritten
owners. `_DecodeBlockAlpha3` remains direct-comparison exact at 608 padded
bytes and one ordered relocation. Reopen Alpha4 only if the campaign adopts a
reviewed ownership-preserving insertion mechanism that does not alter source
topology or if the complete-TU ownership policy itself changes. Do not retry a
post-Alpha3 reorder or any emission-control spelling under the current rules.
