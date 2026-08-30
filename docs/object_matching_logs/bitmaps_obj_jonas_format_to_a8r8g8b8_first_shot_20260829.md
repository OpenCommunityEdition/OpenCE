# `bitmaps.obj` format-to-A8R8G8B8 first-shot boundary

## Result

`_bitmap_format_to_a8r8g8b8` was evaluated as one history-unattempted,
natural typed C89 candidate.  The candidate is not strict semantic-COFF
exact, so its body, owner prototype, format enumeration, and include were
inverse-reverted immediately.  The eight inherited exact functions remain
the only accepted functions in `bitmaps.obj`; this wave claims no code or
data credit.

| Function | January target | Frozen candidate |
| --- | ---: | ---: |
| Padded code bytes | `544` | `560` |
| Relocations | `22` | `22` |
| Normalized SHA-256 | `393db9b9a129a1c388c69259dd20b6dc91bbf6e5943ebc5d398375e9da4ef806` | `845386a8bec3f628e778d3b848bd928da6ba33cd2572523f3463d38364492abf` |

The candidate whole-object SHA-256 was
`c9056b966d93f0c622cd962fb670b876986235a52b4cf4fdefd83be6b8812ed5`.
The January split-object SHA-256 is
`6240548bf1ee72e09f15d1b48284092b17b6c096c11e8d294bbf71ae266acb57`.
Whole-object hashes include debug metadata; the normalized packet hashes,
padded extents, and ordered relocation identities above are the rejection
authority.

## Provenance and prior-work census

All applicable `bitmaps.obj` ledgers, the campaign methodology and audit
tooling notes, and the repository's Claude/agent documentation were read
before source work.  Current ancestry contains only the initial empty symbol
skeleton for this function.  A scan of all 1,524 registered worktrees found
no source definition, and no existing object ledger records a prior
production candidate for this symbol.  The frozen palette-match and
slice/face-copy boundaries were read and respected; this format decoder is a
separate leaf.

Two public reconstructions independently establish the format semantics:

- HCEA commit `c168af2e747d3095d9a29418ae401f3a39544863`, file
  `src/bitmap_format_to_a8r8g8b8.c`, blob
  `fd0ffa45c857f30ddb36f3c6c2f7d66c80200e9b`, payload SHA-256
  `d50a8c4f7316b4477e26912fbf568a1a27c5fb8c903db13fd395d695be3a2cfb`;
- Stian Eklund commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`, file
  `src/halo/bitmaps/bitmaps.c`, blob
  `c60dee7d67edef7ff75c92ebe0066552e3107ec2`, whole-file SHA-256
  `7f9eb3bf7dd01b5fdb2e365425791a04c04f68c231c48e5c734ad0c4db2df2ba`.

Both sources expand A8, Y8, AY8, A8Y8, R5G6B5, A1R5G5B5,
A4R4G4B4, X8R8G8B8, A8R8G8B8, and P8-bump texels into a 32-bit pixel and
use the same 256-entry vector palette for the P8 case.  January disassembly
independently confirms the null assertion at line `0x22B`, the 18-way switch,
the format-specific scalar loads and channel replication, the palette lookup,
the unsupported-format assertion at line `0x254`, and the final return.

The single candidate used an owner-header format enumeration, a public typed
prototype, the source-owned `pixel32 global_vector_palette[256]`, ordinary
format-governed byte/word/dword buffer views, and explicit defined returns.
It contained no raw address, structure offset, representation pun, or
compiler-control device.

## Preprocessing reconciliation

The first build invocation stopped in preprocessing because the newly
included `bitmaps.h` contains pre-existing prototypes using `real`, while it
had initially been placed before `bitmap_group.h`, which supplies the required
real-math include.  MSVC emitted no object and therefore no candidate bytes.
The coordinator authorized exactly one declaration-only correction:
`bitmap_group.h` was moved before `bitmaps.h`; the frozen function body was
left byte-for-byte unchanged.  The next invocation was the sole
code-producing candidate compile.  No further declaration, source, type,
branch, expression, or control-flow variant was attempted.

## Measured boundary

All 22 relocation kinds and ordered destinations are present, but their owner
offsets do not match.  The first four target relocations are at
`+0x13,+0x18,+0x1D,+0x24`; the candidate places them one byte later at
`+0x14,+0x19,+0x1E,+0x25`.  The target palette relocation is at `+0x1B0`,
while the candidate emits it at `+0x1BB`.  The unsupported-format file,
literal, assert, and exit relocations move from
`+0x1BF,+0x1C4,+0x1C9,+0x1D0` to
`+0x1CB,+0x1D0,+0x1D5,+0x1DC`.  The eleven internal switch-table
relocations move with the enlarged packet.

The structural divergence begins in the prologue.  January saves only ESI,
tests the mipmap pointer, and enters the switch; it saves EDI locally inside
the A4R4G4B4 arm.  The policy-safe candidate's explicit terminal return keeps
the format value live and makes VC7 save EDI before the null branch.  The
candidate also algebraically folds the R5G6B5 masks and shifts differently
from January.  It therefore grows by one 16-byte alignment quantum and cannot
pass complete padded-byte or relocation-address identity.

This is a durable first-shot boundary, not a tuning invitation.  Do not retry
the rejected typed switch, remove the defined terminal return, reproduce the
public reconstruction's undefined fall-through, or permute masks, locals, or
case expressions from this miss.  Reopen only with authenticated original
January C source or an equally strong ordinary-C contract that predicts both
the prologue lifetime and R5G6B5 expression schedule.

## Restoration, policy, and validation

The candidate and declaration edits were inverse-applied with the patch tool.
The final source/header blobs are exactly their canonical inputs:
`d902025d4b269a4686d77989fc1f1591a34913bc` and
`ee575d962585c9b8fea818168304fcf06b213485`.  The restored rebuilt
`bitmaps.obj` SHA-256 is
`dec326869b8f4bddbbcec715515f52a1db366064cf04c63ff5926d61c45d6a21`.
The whole-TU regression manifest reports zero failures, warnings,
changed-nonexact functions, or newly exact functions and preserves all eight
inherited exact packets.

The rejected implementation introduced no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, optimizer barrier, raw address or structure
offset, pointer/integer reconstruction, union pun, undefined behavior, object
patch, compiler-option change, semantic exception, comparator exception, or
code-generation forcing.  Final production source contains none of the
candidate code.

The complete `halobetacache_build`, `libcmt_build`, report, semantic report,
and progress graph pass.  The semantic audit scans 470 units, evaluates 4,919
functions, finds 4,802 semantic-exact and 4,812 accepted-exact functions, and
reports zero unit errors.  Campaign progress remains 384/833 linked objects,
4,773/11,060 functions, and 606,876/2,198,102 code bytes; Halo remains
282/468 linked objects, 4,606/7,574 functions, and 593,962/1,770,166 bytes.
The strict board remains 277/619 objects, 4,784/8,246 functions, and
663,783/1,922,669 padded bytes.

Admission is 0 candidates / 0 contradictions / 0 revocations.  The parked
inventory is 12 active / 0 stale / 0 invalid.  All 212 tooling tests pass.
The regenerated report and semantic-report SHA-256 values are respectively
`b13ef78fdc5dce935a82cdf93d76a474c3273dc54148975139f7c3114b2d43d0`
and
`51d605379dee1e1d44685c048d51150e13d504ae617a5ee1d1cb450d93fb431b`.

The protected Units sentinel `_unit_preprocess_node_orientations` remains
strict exact at 1,920 padded bytes, 87 ordered relocations, and normalized
SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Only this additive ledger is retained; nothing is pushed.
