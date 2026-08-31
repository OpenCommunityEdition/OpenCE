# `s3tc.obj` `DecodeBlockAlpha4` exact recovery and ownership retest

## Result

`source/bitmaps/s3tc/s3tc.obj::_DecodeBlockAlpha4` is retained as strict
semantic-COFF exact after a fresh ownership retest on the current integration
base. The candidate is the ordinary Microsoft reference-rasterizer body in
the authenticated source order, before `DecodeBlockAlpha3`:

| Evidence | January target | Retained candidate |
| --- | ---: | ---: |
| Meaningful / padded code bytes | `131 / 144` | `131 / 144` |
| Ordered relocations | `1` | `1` |
| Normalized SHA-256 | `f386c8c61c53347d44665a7f363d34f2099576f058d76ab3484b033b183026eb` | `f386c8c61c53347d44665a7f363d34f2099576f058d76ab3484b033b183026eb` |

The sole relocation is `IMAGE_REL_I386_REL32` at owner offset `+0x12`,
resolving to `_DecodeBlockRGB+0` in both objects. The emitted owner is an
ordinary `.text` COMDAT with flags `0x60501020` and selection `1`, exactly
matching January. No object-level `Matching` claim is made: the unit now has
2/18 strict functions, one credible fuzzy residual, and fifteen unwritten
owners.

This supersedes the 2026-08-30 retention conclusion in the original version
of this ledger. That run correctly established the function match and donor
provenance, but its complete-owner check reported changes around the inherited
Alpha3 owner. The fresh current-base retest does not reproduce that blocker:
both pre-existing emitted owners are byte-for-byte and ownership-view
identical, all non-code evidence is unchanged, and the only semantic symbol
delta is the intended new Alpha4 owner.

## Authenticated source and types

The controlling donor is Microsoft's preserved Direct3D reference rasterizer
at commit `5c6fe3db626b63a384230a1aa6b92ac416b0765f`, file
`multimedia/directx/dxg/ref8/common/dxtn.cpp`, blob
`7a2508f7c313dc77aca8debc2936bf000a83edab`. Its source declares `row` and
`col`, loads one 16-bit alpha row, traverses four rows and four columns, expands
each low nibble with

```c
((alpha & 0xF) << 4) | (alpha & 0xF)
```

and shifts the row word right by four after each output pixel. It calls
`DecodeBlockRGB` before alpha expansion. The retained typed C89 body preserves
that statement topology and loop order; it adds no source unrolling or
emission control.

HCEA independently reconstructs the same operation at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
`src/blam/bitmaps/DecodeBlockAlpha4.c`, blob
`3536bc2e6c1db0ab0237a0892ad5a22996a0e36c`. HCEA is corroborating binary
reconstruction evidence; Microsoft remains the source oracle.

`source/bitmaps/s3tc/s3tc.h` owns the public structure and declaration. The
named `s3tc_block_alpha4` has four `word` alpha rows followed by the existing
`s3tc_block_rgb`; compile-time assertions prove the RGB offset is `0x8` and
the complete block is `0x10`. The function prototype is in the same owner
header and precedes the Alpha3 prototype, matching the donor and January
function order.

The retained source/header Git blob hashes are respectively
`9e7481c487273e608cab14d916178871230b2802` and
`566ad7172ac62baae861d74736f757969907aba9` before orchestration commit.

## Whole-TU and ownership proof

The production-path pre-edit object was frozen before the edit and compared
with an XDK 3911 compile to the canonical production output path. This avoids
mistaking the CodeView-recorded output filename for a compiler/debug change.

| Artifact | Raw SHA-256 |
| --- | --- |
| January split target | `95623e800d131322e6a079f07f78af41166864d8d3a29b7fe88ef788bc76a9dc` |
| Pre-edit rebuilt TU | `400a813bca2be1e778c88b3f256e13c2096da900d817738b256a8b9107cd7e52` |
| Retained rebuilt TU | `9d94cc769671114219298b67525ad46dc07e65b404f4058fe18d70831feac23f` |

The inherited owners remain unchanged:

| Owner | Pre-edit versus retained |
| --- | --- |
| `_DecodeBlockAlpha3` | exact; 608 padded bytes, one relocation, normalized SHA-256 `0dd23ce088e93017f7424049431b394d4d44e2730f3e8b0835aaee58e79a6a0d` |
| `_DecodeBlockAlpha3__single_pixel` | identical retained residual; 448 padded bytes, one relocation, normalized SHA-256 `db57324424c861cd2aa46b18de4452ba9f44e124138cbc496e7884d616d70c79` |

The complete non-code fingerprints are identical before and after:

- `.drectve`: 42 bytes, zero relocations, normalized SHA-256
  `1bf8b9d961b573933e6b8f24821b55ae9c81a53e2c2e7fa81841fd557014cad8`;
- `.debug$S`: 201 bytes, zero relocations, normalized SHA-256
  `e5dc84d07404868a5dd4dee70fc60da071dd84db8a05261380d28702c2146cf6`.

The pre-edit symbol inventory has ten semantic records and the retained object
has twelve. The only additions are the `.text` section symbol and external
function symbol for `.text|owners=_DecodeBlockAlpha4`; there are no removed,
renamed, rehomed, or storage-class-changed symbols. Existing COMDAT section
numbers shift by one, but section numbers are object-local and the hardened
ownership view pairs them by owner identity. The generic regression gate's
additive `SYMBOL_SET_CHANGED` finding is therefore the expected review lead
for a newly written function, not an inherited ownership regression.

## Policy and gates

The implementation is readable, typed C89. It uses no assembly, `volatile`,
`register`, pragma, intrinsic, attribute, optimizer barrier, pointer or union
pun, raw address or byte offset, undefined behavior, synthetic caller, dead
branch, duplicate declaration, or compiler-option change. It was not moved
after Alpha3 and no post-donor reorder was tested.

- Focused campaign gate: `_DecodeBlockAlpha4` exact at 144 padded bytes;
  `_DecodeBlockAlpha3` remains exact at 608.
- Full TU campaign gate: 2 exact, 1 residual, 15 unwritten of 18 target owners.
- Hardened direct comparison: Alpha4 and inherited Alpha3 owners pass; the
  known Alpha3 single-pixel residual is unchanged from the frozen pre-edit
  object.
- Runtime non-code, debug, COMDAT selection, owner identities, and relocation
  destinations pass the current complete-owner retest.
- `git diff --check` passes for the retained source and header.

The orchestrator's consolidated build and global semantic, admission, park,
fake-match, tooling-test, and protected-Units gates all pass. No `Matching`
metadata or object-admission claim is part of this recovery.
