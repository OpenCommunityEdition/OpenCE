# `bitmap_utilities.obj` `rgb_colors_interpolate` first-shot boundary (2026-08-30)

## Result

One frozen, ordinary typed C89 candidate was compiled for the previously
unwritten `source/bitmaps/bitmap_utilities.obj::_rgb_colors_interpolate`.  It
was a strict miss and has been inverse-reverted in full.  This lane retains no
production source or header change and claims no function, byte, data, or
object credit.

| Evidence | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `541 / 544` | `33` | `d153348f632899bd347cd0f1aa75798bac2850c0a9bb836da6c1ff2c713b4aa7` |
| Frozen natural candidate | `560 / 560` | `33` | `6cfc01dfabad8ffc6149e16d3c8d341b9c7424166caeb13f2a93dd9e0ffeb3b1` |

The January split object has raw SHA-256
`efb68953ba917a1a9e758d175b9778fefb638b43f31bdfece342dcbb02a0579e`.
The frozen candidate object is preserved outside the production graph as
`build/audit/bitmap_utilities_rgb_colors_interpolate_first_shot_20260830.obj`,
raw SHA-256
`053d0180223b76c1608eed1eac6a7c53aeae4044fbaeb9be6250279d60fd4c45`.
Whole-object hashes include compiler metadata; the table records the complete
padded function packet and ordered semantic relocations.

## Documentation, history, and provenance

The matching methodology, exact-match acceleration playbook, object-ledger
README, authenticated Stian `CLAUDE.md` and `AGENTS.md`, and both reachable
`bitmap_utilities.obj` ledgers were read before source work.  A census of all
1,524 registered worktrees found zero definitions.  Ref-reachable canonical
history contains only the initial symbol inventory and owner-header
declaration, and no object ledger names this function.  This was therefore the
first emitted campaign candidate rather than a retry of hidden tree work.

HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` provides exact-name source
`src/blam/effects/rgb_colors_interpolate.c`, Git blob
`98faaf97eabf3a442ad44f69efdf046b97c82e6e`.  It authenticates the natural
semantic shape: compute `1-u`; validate the bounds; select direct per-channel
RGB interpolation or HSV interpolation; choose the requested hue arc; wrap a
hue above one; convert the HSV result back to RGB; validate and return the
result.  January PC disassembly independently fixes the three assertion line
numbers (`0x939`, `0x93A`, and `0x95D`), named-field order, branch topology,
floating-point grouping, calls, and the complete 33-relocation schedule.

The frozen transcription used owner-header declarations, named interpolation
flag bits, the existing typed color unions and conversion functions, normal
control flow, one parameter per line, and an explicit return.  It used no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, barrier, raw
address or byte offset, pointer/integer reconstruction, representation pun,
undefined behavior, byte forcing, compiler change, tool exception, or
admission exception.

## Fixed boundary

The candidate reproduces the entry calculation and both complete input
assertion blocks byte for byte through the second HSV conversion call.  The
first structural difference is already visible in the frame allocation:
January uses `sub esp, 0x28`, while the donor-shaped candidate uses
`sub esp, 0x2C`.  January keeps both hue values live on the x87 stack after the
two conversions (`fld`, `fld`, `fld st(0)`, `fsub st(2)`); the readable scalar
locals in the candidate force one hue through an extra stack slot and reload.

All 33 ordered relocation identities are the same.  The first 19 relocation
addresses agree through both HSV conversion calls, after which the altered hue
lifetime shifts the half-turn literal from target `+0xF6` to candidate
`+0xFB`.  Extra compare/reload/store instructions grow the remaining HSV arm;
the direct-RGB and final-assert suffixes are semantically concordant but begin
later.  Closing this boundary requires another source/lifetime spelling.
Because the lane permits exactly one natural candidate, no declaration order,
hue expression, comparison, temporary, or control-flow variant was compiled.
Do not retry the donor-style scalar-hue transcription without new independent
January PC source or same-compiler provenance that predicts the target's x87
stack lifetime.

## Restoration and validation

The body, include, flag enum, and support declaration were inverse-removed.
Final production source and header are restored to Git blobs
`c2cac170f88adfaeaf55b7f04eebd4628c96f6e5` and
`ee575d962585c9b8fea818168304fcf06b213485`.  A normal restoration rebuild and
the frozen whole-translation-unit regression gate pass.  The complete exact
set remains unchanged, including `_bitmap_fill`, `_bitmap_alpha_to_rgb`, both
integer HSV/RGB conversions, both real HSV/RGB conversions, the brightness
helper, both packed-color conversions, and both pixel32 conversions.

The complete Halo/libcmt/report/semantic/progress graph passes.  Semantic
audit scans 470 units and 4,919 functions, reports 4,802 semantic-exact and
4,812 accepted-exact functions, and has zero unit errors.  Campaign progress
remains 384/833 linked objects, 4,773/11,060 functions, and
606,876/2,198,102 code bytes; Halo remains 282/468 linked objects,
4,606/7,574 functions, and 593,962/1,770,166 code bytes.  The strict board
remains 277/619 objects, 4,784/8,246 functions, and 663,783/1,922,669 padded
bytes.  Admission is 0 candidates / 0 contradictions / 0 revocations; parks
are 12 active / 0 stale / 0 invalid; and all 212 tooling tests pass.  The
regenerated report and semantic-report SHA-256 values are respectively
`b13ef78fdc5dce935a82cdf93d76a474c3273dc54148975139f7c3114b2d43d0`
and
`51d605379dee1e1d44685c048d51150e13d504ae617a5ee1d1cb450d93fb431b`.

The protected `_unit_preprocess_node_orientations` sentinel remains strict
exact at 1,920 padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
