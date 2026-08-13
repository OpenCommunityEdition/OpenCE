# `libs/libcmt/wcsncat.obj` exact-match ledger

## Target inventory

- January object: `libs/libcmt/wcsncat.obj`
- Function: `_wcsncat`
- Padded code size: `0x3D` (61 bytes)
- Relocations: none
- Owned data: none

The target scans to the destination terminator, copies at most `count` wide
characters, returns immediately after copying a source terminator, and writes
one terminator when the count expires first.

## Source and house-rule notes

The implementation is the ordinary readable Microsoft CRT loop shape. It uses
the repository's established local `typedef unsigned short wchar_t` and the
object's original `/O1 /Gy` build recipe. Parameters are formatted one per line
and the function has an explicit final return.

There is also one return inside the copy loop. This is an evidence-required
exception to the preferred single-return house style: replacing it with a
single-return `goto done` preserved the exact 61-byte CFG but made VC7 assign
the destination/character pair to EDX/ECX instead of the target's ECX/EDX.
Restoring the canonical early-return spelling produced the target register
assignment and exact bytes. The compiler still merges both source returns into
one machine-code epilogue at offset `0x3A`.

No assembly, volatile forcing, `__forceinline`, undefined behavior, byte
patching, compiler-flag changes, or comparator exception is used.

## Experiment record

| ID | Source shape | Size | Relocations | Result |
|---|---|---:|---:|---|
| E01 | Single-return `goto done` | 61/61 | 0/0 | Correct CFG; ECX/EDX mirror |
| E02 | Canonical loop-local early return | 61/61 | 0/0 | Strict exact |

## Strict verification

- size: `61/61`
- relocations: `0/0`
- normalized SHA-256, both sides:
  `e5d84fe788de297e3724ac6466990085984a0f4c618a835dab80b209a16aa24b`
- `section_infos_equal`: `true`

With zero relocations and no owned data, the target and candidate function
sections are byte-for-byte identical. The whole object is safe to mark
`Matching`.

## Current-campaign revalidation and house audit

The historical result was treated as untrusted and transplanted onto campaign
tip `ce1993b7`. A fresh XDK 3911 build under the canonical libcmt
`/O1 /Gy /I"xbox/include"` configuration reproduced the same 61-byte hash and
zero-relocation result. The target owns exactly one external `_wcsncat` function
COMDAT and no runtime data; the candidate adds only normal compiler metadata.

The complete Halo and libcmt builds, semantic/progress reports, clean per-object
regression snapshot/check, and full tool tests were rerun at this campaign
state. House/Berth review confirms vertically formatted parameters and readable
C. The source-level early return is retained only because the measured
single-return spelling creates an ECX/EDX mirror; VC7 still emits one shared
machine epilogue. There is no assembly, volatility, forced inlining, undefined
behavior, alignment directive, raw offset, byte patch, semantic exception, or
flag workaround, and no original target bug was identified.
