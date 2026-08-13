# `libs/libcmt/isctype.obj` exact-match ledger

This atomic lane started from authoritative campaign commit `70e301d1` and
was finally rebased and revalidated on `b6d5887c`. The assignment's index
`824` had become stale through concurrent admissions; the current
configuration places this unit at index `828`. Before editing, it was
confirmed `MISSING`, absent from `config/parked.json`, absent from prior source
and object-ledger history, absent from active worktrees, and unclaimed by the
other active agents.

## Plain-English behavior

The CRT stores a bit mask for every single-byte character in `_pctype`.
`_chvalidator` returns the requested classification bits for a caller that has
already validated its character. `_isctype` performs the public range check:
it accepts the special end-of-file value `-1` and every value through
`UCHAR_MAX`, returns the requested table bits for those values, and returns
zero for anything else.

The range check converts `character` to unsigned before adding one. Unsigned
arithmetic is defined modulo its range: this maps the special `-1` value to
zero while leaving every other out-of-range value above the accepted limit.
Putting the cast after `character + 1` would instead permit signed overflow at
`INT_MAX`, so the cast position is part of the recovered semantics.

## Primary provenance and January inventory

Member `obj\i386\isctype.obj` was extracted directly from the XDK 3911
`libcmt.lib`. It identifies its original source as
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\isctype.c`; its ordinary
C function and line records prove it is compiled C, not handwritten or
vendored assembly.

Before source work, both archive functions compared strict-equal to January:

- `__chvalidator`: 19 bytes, one `DIR32` relocation at `+0x06` to
  `__pctype + 0`, normalized SHA-256
  `f04430081a1622486c9d8f68346850c313eaa9961ae4ad7a6f57016a0753cac3`;
- `__isctype`: 33 bytes, one `DIR32` relocation at `+0x11` to
  `__pctype + 0`, normalized SHA-256
  `8882ad8eaa087645d3e6dda998a36b16b65f6de0ee69ced5cf3b5f679d69587a`.

The archive also contains three select-any D3D lookup tables instantiated by
its internal XDK header environment. January's linked object owns none of
them: it contains only the two function COMDATs above. They are discarded
duplicate definitions, not source-owned data to recreate.

The XDK `ctype.h` public declarations prove both two-`int` prototypes, the
`unsigned short *_pctype` table type, and all classification masks. Adjacent
exact `tolower.obj` and `toupper.obj` independently use the same table and
`_isctype` contract.

## Source-shape experiment record

| ID | Readable C shape | Measured result |
|---|---|---|
| E01 | Direct typed table lookup for `_chvalidator`; `_isctype` accepts `character >= -1 && character <= UCHAR_MAX`, then performs the same lookup | `_chvalidator` strict exact; `_isctype` 34/33 bytes because VC7 retained two signed comparisons and shifted the relocation by one byte |
| E02 | Normalize with `(unsigned int)character + 1 <= UCHAR_MAX + 1`, casting before the addition so overflow is defined | Strict exact: 33/33 bytes, one identical relocation to `_pctype + 0`, normalized SHA-256 `8882ad8eaa087645d3e6dda998a36b16b65f6de0ee69ced5cf3b5f679d69587a` |

No assembly, byte patch, opaque offset, undefined behavior, `volatile`, forced
inline, optimization pragma, codegen-only cast, or compiler-flag change is
permitted in this lane.

## Acceptance evidence

The candidate was built with the configured XDK 3911 compiler and the
object's `/O1 /Gy` flags. Both candidate functions compare strict-equal to
both the January split target and the extracted archive member, including
sizes, normalized instruction bytes, relocation addresses/types/targets and
symbol ownership:

- `__chvalidator`: 19/19 bytes, one relocation at `+0x06`, hash
  `f04430081a1622486c9d8f68346850c313eaa9961ae4ad7a6f57016a0753cac3`;
- `__isctype`: 33/33 bytes, one relocation at `+0x11`, hash
  `8882ad8eaa087645d3e6dda998a36b16b65f6de0ee69ced5cf3b5f679d69587a`.

January and the candidate each own exactly those two external function
COMDATs and no runtime `.data`, `.bss`, or `.rdata`. The archive-only D3D
tables are correctly absent from both linked objects. Forced clean Halo and
libcmt builds passed, as did the semantic audit (zero unit errors; both
functions admitted by strict COFF evidence), the admission audit (no
candidate, contradiction, or revocation for this unit), progress generation,
and all 179 tooling tests. The final clean-commit regression snapshot/check
then rebuilt the unit and reported `__chvalidator` and `__isctype` as
`still_exact`, with zero failures, warnings, changed-nonexact functions, or
unreviewed newly-exact functions.

## House, Berth, readability, and bug audit

The reconstruction is ordinary typed C: each parameter is on its own line,
returns are explicit, `UCHAR_MAX` names the range limit, and `_pctype` has its
real `unsigned short *` type. The unsigned cast expresses the recovered
range semantics and prevents signed overflow; it is not an opaque codegen
cast. There is no assembly, `volatile`, forced inline, undefined behavior,
byte patch, raw structure offset, nonstandard flag, pragma, comparator
exception, or ownership fiction. Both functions require parameters, so the
project's explicit `(void)` rule for zero-argument functions is inapplicable.
No original-game bug was found in this object, so no preserved-bug comment is
needed.
