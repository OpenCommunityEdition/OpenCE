# `libs/libcmt/mbstowcs.obj` exact-match ledger

This atomic lane starts from canonical commit `1b270ed1`. At lane start the
object was `MISSING` at config index 826, absent from `config/parked.json`, and
had no production source, object log, topic branch, or active worktree. It
contains one 65-byte function and owns no runtime data.

## Plain-English behavior

`mbstowcs` converts the project's single-byte Xbox C locale into 16-bit wide
characters. When a destination buffer is supplied, it copies at most the
requested number of characters and stops at the first null terminator. When
the destination is null, it only measures the source string. A destination
with a zero maximum writes nothing and returns zero.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive member `obj\i386\mbstowcs.obj` records
ordinary C source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\mbstowcs.c`, compiler
identifier `0x001c2426`, Microsoft C line records, and the external function
`_mbstowcs`. That archive function is itself strict-equal to January:

- target/base size `65/65` bytes;
- target/base relocation count `0/0`;
- normalized SHA-256
  `10a53c6d2c7a4b1eb2bf0213ec7cbef4b63bac4d7d97799377dccde1906854b8`;
- one external, one-byte-aligned, pick-no-duplicates `.text` COMDAT;
- no target-owned `.rdata`, `.data`, or `.bss`.

Microsoft's public Xbox source archive also contains the original
`private/sdktools/crt/crtw32/convert/mbstowcs.c`. Its `_XBOX` branch confirms
the exact public contract, zero-count early return, copy loop, unsigned-byte
conversion, and null-destination `strlen` behavior. January and the archive
member remain the byte authority.

## Experiment matrix

| ID | One changed factor | Size T/B | Relocs T/B | Base hash/result | Decision |
|---|---|---:|---:|---|---|
| E01 | Direct readable copy loop plus a conventional `end` pointer length scan | 65/66 | 0/0 | `0e44addda9e4...`; copy loop close, length topology differs | Rejected |
| E02 | Add the proven zero-count guard, call ordinary `strlen` | 65/57 | 0/1 | Out-of-line `_strlen` relocation, unlike January | Rejected |
| E03 | Zero-count guard plus post-increment length loop and `end - source - 1` | 65/64 | 0/0 | `ad79aca6a4f1...`; same operations, shorter two-step subtraction | Rejected |
| E04 | Parenthesize `end - (source + 1)` | 65/64 | 0/0 | Byte-identical to E03 | Rejected |
| E05 | Name `source + 1` as a live base pointer | 65/65 | 0/0 | `6133f4f687e1...`; exact except ECX/EDX roles in the scan | Improved, rejected |
| E06 | Use the authenticated original Microsoft source with public `string.h` | 65/59 | 0/1 | Public project header emits an out-of-line `_strlen` call | Proven context difference; rejected |
| E07 | Temporarily declare `strlen` intrinsic to test the original internal-header context | 65/65 | 0/0 | Strict exact | Diagnostic only; pragma removed under project policy |
| E08 | Expand that intrinsic into readable C: named scanned `char`, do/while post-increment, live `source + 1` base | 65/65 | 0/0 | **Strict exact**, target hash above | Accepted |

E07 proves what the original Microsoft source and target did, but the project
does not retain compiler pragmas merely to force bytes. E08 expresses the same
string-length operation directly in normal C and reproduces the intrinsic's
instruction graph without a pragma, flag change, or invented behavior.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the accepted source with unchanged libcmt
flags `/O1 /Gy /I"xbox/include"`. The hardened comparator reports
`section_infos_equal`; all 65 bytes match and there are no relocations.
Independent COFF inspection confirms the sole external COMDAT and absence of
runtime data. The extra candidate `.drectve` and CodeView sections are
discardable compiler metadata.

The final forced rebuild remained strict-exact. Full Halo and libcmt builds
passed, including 529 compiled objects in this isolated lane. Progress reports
324/833 objects overall and 57/212 libcmt objects, with libcmt at 77/476 exact
functions and 3,836/55,015 exact code bytes. The semantic audit scanned 427
units, evaluated 3,514 functions, accepted 3,430 exact functions, and reported
zero unit errors. The clean-commit fail-closed regression snapshot/check lists
`_mbstowcs` as `still_exact`, with no failures or warnings. All 179 tooling
tests pass; `git diff --check` and final worktree status are clean.

## House/Berth and bug audit

The accepted source is readable C with named parameters, natural control flow,
and an explicit scanned-character local. It contains no assembly, `volatile`,
forced inlining, retained optimization pragma, undefined behavior, byte patch,
opaque code-generation cast, raw offset, alignment trick, compiler-flag
change, or comparator waiver. The early return is retained because both the
authenticated Microsoft source and January bytes prove it. No behavioral bug
was found, so no original-bug annotation is required.
