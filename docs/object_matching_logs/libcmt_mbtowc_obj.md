# `libcmt/mbtowc.obj` exact-match ledger

## Plain-English behavior

This single-byte-locale converter returns zero without output when its source
or capacity is absent. A null input character optionally writes a wide null;
any other byte is zero-extended into one wide character and reports one byte
consumed.

## January and XDK provenance

January owns one 55-byte external function COMDAT and no runtime data. XDK
3911 `libcmt.lib` contains `obj\\i386\\mbtowc.obj`; its COFF records identify
`..\\mbtowc.c`, the original Microsoft source path, and VC7. The archive
function is itself strict-equal to January. CodeView proves `wchar_t`; target
branches prove the source, destination, and count contract.

## Strict target evidence

- function: `_mbtowc`
- size: 55 bytes
- relocations: none
- normalized SHA-256:
  `5f6f69c1e0c5f26d9ffa024653b7e69ff10778f44c561f1938d11099a1c81e68`
- target-owned `.rdata`, `.data`, and `.bss`: none

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| M01 | Early null-source/count return, optional wide-null store, optional unsigned-byte store, return one | **STRICT EXACT** on the first fresh rebuild: 55/55 bytes, no relocations, identical hash |

## House/Berth audit

- The wide-character type and parameter roles are explicit; no raw offsets or
  opaque structures are needed.
- The early returns are the natural historical validation topology visible in
  the target and avoid inventing state for a one-function CRT leaf.
- No assembly, `volatile`, undefined behavior, forced inline, byte forcing,
  alignment directive, flag change, or comparator exception is used.
- No original bug was found in the function's intended single-byte-locale
  behavior.

The rebuilt object owns exactly the target's one external function COMDAT and
introduces no runtime `.rdata`, `.data`, or `.bss`. Admission requires strict
whole-object equality, full Halo and CRT builds,
zero semantic errors, a clean forced-rebuild regression check, all 179 tests,
and a clean worktree.
