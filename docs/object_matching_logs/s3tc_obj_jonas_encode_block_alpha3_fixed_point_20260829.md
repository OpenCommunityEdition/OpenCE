# `s3tc.obj` `EncodeBlockAlpha3` first-shot fixed point (2026-08-29)

## Result

This isolated lane started from canonical commit
`e2208e7d5a2891ed2fd1769ce257f12132b095be` and compiled exactly one frozen,
typed C89 production candidate for
`source/bitmaps/s3tc/s3tc.obj:_EncodeBlockAlpha3`. The candidate was not
strict and was removed immediately without a spelling, type, branch,
tail-merge, helper, or declaration retry. The final production source and
header are byte-for-byte identical to the canonical input, so this ledger
grants no function, byte, data, or object credit.

| Evidence | January target | Frozen candidate |
| --- | ---: | ---: |
| Padded code bytes | 624 | 832 |
| Relocations | 1 | 2 |
| Normalized SHA-256 | `09f8b6b93defb1674e4164ed6ea7f76f3efb7d0909ed7abc3c77587729b72880` | `16da114a22a49677aee53a2663d4df6932738afccf18d82f53c35d3080b617bd` |
| Whole-object SHA-256 at measurement | `95623e800d131322e6a079f07f78af41166864d8d3a29b7fe88ef788bc76a9dc` | `d4a385fd3e3a471fe2b8d6b816ca12d88eb3c9409d429583ebe4b448022db149` |

The target's sole `IMAGE_REL_I386_REL32` relocation is at owner offset
`+0x259` and resolves to `_EncodeBlockRGBColorKey+0`. The candidate emitted
two relocations to that same destination, at `+0x250` and `+0x328`, because
VC7 retained separate degenerate and nondegenerate call sites. Ordered
relocation identity therefore fails before any byte-level interpretation.

## Provenance and prior-work census

No existing object ledger mentions `s3tc.obj`, `_EncodeBlockAlpha3`, or its
source path. The canonical file is an empty symbol skeleton: the strict board
reports 0/18 functions and 0/6,880 padded bytes exact. `git log --all`, the
complete ref-reachable object census, and the registered-worktree source-blob
census found only the canonical source blob
`6d61720d4af2552e7673e46f647b5ca896e67fe9`; no unintegrated or dirty exact
implementation was available. The source header's canonical blob is
`00b1a259b6339864fc7ca00b55fbe55ad9eabbe6`.

Two independent public artifacts constrained the single candidate:

- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` supplies the
  exact-name DXT5 endpoint-selection and three-bit packing reconstruction.
  Its `EncodeBlockAlpha3.c` blob is
  `3965bd3e340b911587c1098ec89ae80b3f04d5f8`; the `S3TC_COLOR`,
  `S3TCBlockAlpha3`, and `S3TCBlockRGB` header blobs are respectively
  `bdb10322e158296930f2f2d9bdc66e04681bb875`,
  `c369ffde68729fd1df1edf030620206302275773`, and
  `2be564d1200d7bd1910a38d931a8b3bb2be8360b`.
- Microsoft's public NT5 DirectX source at commit
  `daad8a087a4e75422ec96b7911f1df4669989611`, file
  `Source/XPSP1/NT/multimedia/directx/dxg/ref8/common/dxtn.cpp`, blob
  `aa0550d751f228190f315fcc343fdddc6b7be1b3`, independently establishes the
  typed DXT block layouts and the eight-pixel rolling 24-bit alpha-bitmap
  cache used by the sibling decoder.

January COFF and disassembly remained authoritative. They prove the two
endpoint bytes at destination offsets `+0`/`+1`, six bitmap bytes at
`+2..+7`, the RGB sub-block at `+8`, a descending 15-to-0 selector loop,
signed quotient mapping, byte-at-a-time little-endian stores, and the final
RGB encoder call.

## Frozen candidate and measured boundary

The one candidate used ordinary named structures, external typed prototypes,
byte endpoint owners, two natural 16-pixel extrema scans, the HCEA six-alpha
versus eight-alpha selection, one descending rolling-bitmap loop, and an
explicit C89 return. It deliberately contained no private ABI declaration,
raw offset, representation cast, or compiler-control device.

The miss is structural at entry. January reserves `0x0c` stack bytes; the
candidate reserves `0x10`. In the first scan January keeps source alpha in a
byte register and compares with unsigned `jbe`/`jae`. The candidate's named
`int alpha` widens every texel with `movzx`, then emits signed 32-bit
`cmp`/`jle`/`jge` sequences. January groups the fifteen noninitial texels as
three five-way iterations, while the candidate groups the scan as two
eight-way iterations. January reaches one shared RGB call and terminal `ret`
at `+0x266`; the candidate has an early degenerate `ret` at `+0x25d`, a second
call at `+0x327`, and its final `ret` at `+0x335` before padding to `0x340`.

Do not retry this typed body, its direct per-pixel `int alpha` comparisons,
its early degenerate return, or cosmetic restatements. Reopen only with an
authenticated C89 Microsoft/Halo encoder body or equally strong compiler
provenance that specifically predicts January's narrow comparisons,
three-by-five first scan, `0x0c` frame, and single shared call tail. A byte
local or common-tail rewrite inferred only from this miss is tuning and is not
authorized by this ledger.

## Revert, policy, and validation

The candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, undefined behavior, raw address, byte-offset
access, pointer/integer pun, artificial anchor, or byte/code-generation
forcing. Both candidate edits were inverse-applied with the patch tool. The
final source/header Git hashes equal their canonical index hashes, and the
reverted base object rebuilt successfully through the production Ninja edge.

The pre-edit fail-closed whole-TU regression manifest passes after the revert:
zero failures, warnings, changed nonexact functions, or newly exact functions.
The object remains 0/18 strict functions. No configuration, semantic ledger,
parked record, protected object, test, tool, or compiler setting changed.

Final gates pass:

- Full report, semantic audit, and progress complete with zero unit errors:
  4,917 functions evaluated across 470 units, 4,810 accepted exact, and
  384/833 linked files.
- Strict campaign board remains 277/619 objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes; `s3tc.obj` remains 0/18.
- Object admission reports zero candidates, contradictions, or revocations.
- Parked validation reports 12 active, zero stale, and zero invalid.
- The protected `_unit_preprocess_node_orientations` owner remains strict at
  1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Pytest passes 212/212; `git diff --check` passes.

The committed scope is this additive ledger only. Nothing is pushed.
