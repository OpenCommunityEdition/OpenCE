# `libs/libcmt/wtox.obj` exact-match ledger

This isolated atomic lane starts from authoritative campaign commit
`254e9ad6`. Configuration index 814 was `MISSING`; the object was absent from
`config/parked.json`, production history, prior matching logs, active
worktrees, and the other agents' claims.

## Plain-English behavior

The object provides Microsoft's wide-character decimal conversion family.
`_wtol` skips Unicode whitespace, accepts one optional plus or minus sign,
then accumulates every recognized Unicode decimal digit in a signed `long`.
`_wtoi` is the corresponding `int` wrapper. `_wtoi64` performs the same
operation with a 64-bit accumulator. Like the original CRT routines, these
functions do not diagnose overflow and stop at the first non-digit.

## Primary provenance and January inventory

The exact `obj\i386\wtox.obj` member was extracted from the local XDK 3911
`libcmt.lib` before production source was written. It compares strict-equal to
the January split object for every runtime section. CodeView names the build
input `..\wtox.c`, identifies Microsoft's template source as
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\atox.c`, and carries
ordinary `.bf`, `.lf`, and `.ef` records for all three functions. This proves
compiled C rather than assembly.

The corresponding Microsoft 1989-2001 `atox.c` was recovered from the pinned
Easy-Build-2k NT source mirror at commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`. Its `_UNICODE` template expands
directly to `_wtol`, `_wtoi`, and `_wtoi64`; the reconstruction keeps that
algorithm and uses the XDK's typed wide-character interfaces.

| January symbol | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `__wtol` | 95 | 2 | `3b8afcdd1dc923a7fd2841367a545f4b3f86b1bc792b9e04d9ece91a1553d98a` |
| `__wtoi` | 5 | 1 | `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449` |
| `__wtoi64` | 142 | 3 | `0901640fc1ac21fb453d482d1ba5c691cff1c6b9b15d634fc3a24d384eb2365a` |

January owns exactly those three `.text` COMDATs. It owns no `.data`, `.bss`,
or `.rdata` in this object. The rebuilt object's other sections are compiler
directive/debug metadata and are not linked runtime ownership.

## Measured source-shape experiments

All production measurements used XDK 3911 CL 13.00.9254.1 under the inherited,
unchanged `/O1 /Gy` command.

| ID | Readable-C shape | Result |
|---|---|---|
| E01 | Faithfully specialize Microsoft's authenticated `atox.c` template for wide characters: typed `wchar_t` input, `iswspace`, `_wchartodigit`, the original sign local, multiply-by-ten accumulator, and the original final sign branch | Accepted on the first compile. All three functions match size, normalized bytes, and every relocation identity exactly. |

The six relocations match in address, type, destination, and addend:
`__wtol` calls `_iswctype` at `+0x15` and `__wchartodigit` at `+0x47`;
`__wtoi` tail-jumps to `__wtol` at `+0x1`; and `__wtoi64` calls `_iswctype`
at `+0x19`, `__allmul` at `+0x4A`, and `__wchartodigit` at `+0x6B`.

## Original-behavior and bug audit

No confirmed implementation bug was found. The absence of null-pointer and
overflow checks is intentional historical CRT behavior, so the reconstruction
does not add modern defensive branches that would change semantics. Unicode
digit recognition remains delegated to `_wchartodigit`, including the
non-ASCII digit ranges implemented by the neighboring admitted object.

## House, Berth, and readability audit

The final source is ordinary typed C derived from authenticated Microsoft
source. Parameters are vertically formatted, returns are explicit, constants
are readable, and the helper prototype is typed. There are no opaque offsets,
invented structures, or misleading pseudocode.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
codegen cast, alignment coercion, compiler-flag change, optimizer pragma,
comparator exception, or prebuilt-object substitution is used. The source
preserves the original behavior while remaining useful to future readers.

## Final validation

- The object was deleted and force-rebuilt with XDK 3911 under unchanged
  `/O1 /Gy`; the post-rebuild hardened comparison kept all three functions
  strict exact.
- Whole-object runtime ownership is exact: three expected code COMDATs and no
  owned data, BSS, or read-only data.
- Full `halobetacache_build` and `libcmt_build`: successful.
- `progress` and `semantic_progress`: successful; the semantic audit reports
  zero unit errors and accepts all three functions by strict COFF evidence.
- Admission audit: no unmarked completion candidate. Its sole contradicted
  `shell_xbox` result is the pre-existing project-wide finding and is unrelated
  to this object.
- Tooling tests: 179/179 passed.
- `git diff --check`: clean.
- Clean committed per-unit regression snapshot/check: force-rebuilt the unit,
  retained `__wtol`, `__wtoi`, and `__wtoi64` as `still_exact`, and reported
  no changed non-exact function, failure, or warning.
