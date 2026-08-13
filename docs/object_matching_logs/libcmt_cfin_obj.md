# `libs/libcmt/cfin.obj` exact-match ledger

This atomic lane starts at campaign commit `08f86f9b`. Before editing,
`cfin.obj` was `MISSING` at config index 656, unparked, absent from all refs,
logs, and worktrees, and cleared against every active peer claim.

## Plain-English behavior and provenance

`_fltin2` converts a decimal C string into the CRT's encoded double result. It
records how many bytes were consumed and translates three parser outcomes—no
digits, overflow, and underflow—into the flags expected by formatted input.

The primary provenance is `obj\i386\cfin.obj` from the authentic XDK 3911
`libcmt.lib` (archive SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`;
member SHA-256
`086449a89357ab11903fa8a0de9218da0095792e5122ee7eee25ce0852ba16f9`).
Its CodeView names Microsoft source
`d:\xbox-finalfre\private\sdktools\crt\fpw32\conv\cfin.c` and compiler ID
`0x001c2426`. The public Xbox source is pinned at Git blob
`6d722d94a5a50abf80b6586477f68ee67035cbcb`; its matching internal type
definitions are pinned at blobs `09e8ca9500638a15144c2dfb534e2e4e79dba64d`
and `971e0166e136b26061b18f96785f3a2f410b2529`.

The archive member is strict-identical to January before reconstruction: one
133-byte `_fltin2` function, two REL32 relocations (`___strgtold12` at +29 and
`__ld12tod` at +65), normalized SHA-256
`0d0ab166e005e1e23f55b0afc554f32cd06d89b881491d01010a02e969d27ff7`,
and no runtime data.

## Measured source shapes

| ID | One changed factor | Strict result | Decision |
|---|---|---|---|
| E01 | Faithful control flow, but represent the temporary as a C `double` and assign `0.0` | 130/133 bytes, 2/2 relocs; VC7 emitted `fldz`/`fstp` and needed only two saved registers | Rejected: the authentic source explicitly avoids x87 exceptions while copying/zeroing the representation |
| E02 | Preserve the same readable control flow, but model the double as two named 32-bit representation words, including its ABI alignment slot | 133/133 bytes, 2/2 exact relocs, identical normalized SHA | Accepted |

The word representation is semantic, not byte forcing: Microsoft's own source
states that it copies a `DOUBLE` wrapper rather than executing floating-point
instructions specifically to avoid raising IEEE exceptions.

## Validation and house/Berth audit

- XDK 3911 CL 13.00.9254.1 used unchanged `/O1 /Gy` flags.
- Hardened `coff_compare` reports `all_equal: true`; no aliases or data owners
  exist in this object.
- Full-build, progress/semantic, regression, admission, and test results are
  recorded after the final clean commit/rebase.
- The source uses named status enums, checked structure layout, vertical
  prototypes, and ordinary readable C. It contains no assembly, `volatile`,
  forced inline/noinline, undefined aliasing, byte patch, flag change,
  comparator exception, or waiver. The explicitly named representation words
  preserve an intentional original behavior, not a bug; no behavioral bug was
  found that needs a fix note.
