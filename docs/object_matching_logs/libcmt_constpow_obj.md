# `libs/libcmt/constpow.obj` exact-match ledger

This isolated atomic lane starts from campaign commit `9b666b7d`. Before any
production edit, `constpow.obj` was confirmed `MISSING` at config index 654,
absent from `config/parked.json`, and free of topic branches, worktrees, logs,
or active peer ownership.

## Plain-English behavior

The CRT's decimal conversion code sometimes has to scale a number by a very
large or very small power of ten. This object stores two lookup tables for
that job. Each entry is a 12-byte extended-precision value: the normal
ten-byte x87 representation plus two extra precision bytes. One table holds
positive powers from 10^1 through 10^4096; the other holds the corresponding
negative powers.

## Primary provenance and January inventory

The authentic XDK 3911 archive member is `obj\i386\constpow.obj`. Its CodeView
records identify Microsoft source `..\constpow.c`, compiler ID `0x001c2426`,
and two arrays in ordinary C. The matching Microsoft 1991-2001 CRT source is
also preserved in the public XP SP1 source tree at
`base/crts/fpw32/conv/constpow.c`; it documents that the representation was
ported from assembly in 1991 and names the same `_pow10pos` and `_pow10neg`
tables.

The archive member and January target are strict-identical before any source
reconstruction:

- one 700-byte initialized writable `.data` section with eight-byte alignment
  and flags `0xC0400040`;
- external `_pow10pos` at offset `0x000`;
- external `_pow10neg` at offset `0x160` (four bytes of compiler-inserted
  alignment follow the first 348-byte table);
- no relocations, code, `.rdata`, or `.bss`;
- normalized SHA-256
  `1be6b9bcfbebc370831ae298fcf28d4fbfe45d9537d311ef7b9ab09ac525e9f0`.

## Measured source-shape experiments

| ID | One changed factor | Strict measurement | Result |
|---|---|---|---|
| E01 | Authentic little-endian Microsoft table initializers, expressed as two arrays of a named 12-byte structure | Target/candidate/archive are 700/700/700 bytes, 0/0/0 relocations, identical normalized SHA, external symbols at offsets `0` and `0x160`, and identical section flags | Accepted on the first build |

No alternative source shape was tried because E01 reproduced every owned
runtime byte and ownership property. The four bytes between arrays are normal
VC7 global-data alignment, not an explicit padding field or byte-forcing
directive.

## Strict validation

- XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the source with the unchanged
  generated `/O1 /Gy /I"xbox/include"` command.
- `section_infos_equal` is true for January versus candidate and January
  versus the authentic archive member. The complete 700-byte owner span,
  both external symbols, section flags, and absence of relocations agree.
- Full `halobetacache_build` and `libcmt_build` succeeded.
- `progress` credited one complete libcmt object and all 700 data bytes. In
  this isolated baseline, libcmt reached 86/212 objects, 130/476 functions,
  8,895/55,015 code bytes, and 2,428/8,637 data bytes.
- `semantic_progress` and the explicit semantic audit completed with zero unit
  errors. The audit evaluated 3,567 functions and accepted 3,483.
- Admission audit found no candidate or revocation for this object. Its sole
  contradicted `source/shell/shell_xbox` result is unrelated and pre-existing.
- All 179 tooling tests passed; JSON parsing, `git diff --check`, prohibited-
  source review, and scope review were clean.
- The clean-commit regression snapshot/check returned `ok: true` with no
  failures or warnings after forcibly deleting and rebuilding the candidate
  object; the complete non-code owner remained unchanged.

## House, Berth, readability, and bug audit

The source preserves Microsoft's named 12-byte representation and names both
tables by purpose. Every encoded value is annotated with the mathematical
power it represents, so the hexadecimal bytes are reviewable data rather than
unexplained magic numbers. These are exact extended-precision constants that
cannot safely be rewritten as host `long double` literals without changing
their representation or extra precision.

No assembly, `volatile`, forced inline/noinline directive, undefined behavior,
explicit alignment directive, object-byte patch, flag change, comparator
exception, or codegen-only cast is used. The source contains no control flow,
pointer arithmetic, tag/datum accesses, or speculative structure offsets. No
original behavioral bug was found, so no bug-preservation comment is needed.
