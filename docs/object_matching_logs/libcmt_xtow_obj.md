# `libs/libcmt/xtow.obj` exact-match ledger

This atomic lane starts from campaign commit `0261a4b3`. At lane start the
object was `MISSING` at config index 812, absent from `config/parked.json`,
absent from every pre-existing Git ref, branch, worktree, object ledger, and
production source, and explicitly announced to the other active lanes before
the first production edit.

## Plain-English behavior

These five wrappers turn signed or unsigned 32-bit and 64-bit integers into
wide-character strings. Each wrapper first asks the corresponding narrow CRT
routine to format the integer in the requested radix, converts that temporary
ASCII string to wide characters, and returns the destination supplied by the
caller.

## Provenance and January inventory

The primary provenance is `obj\i386\xtow.obj` from the authentic XDK 3911
`libcmt.lib`. The member has SHA-256
`dff1257febefde3aaf63cb045cb24a82d329b9c66066d5ff2ed0baade4a41c12`,
an August 2001 timestamp, compiler identifier `0x001c2426`, ordinary C
CodeView, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\xtow.c`.
Its five runtime function COMDATs are already strict-identical to January,
so the archive member supplies both source provenance and an independent
machine-code oracle. There are no target-owned runtime data sections or
aliases.

| Function | Size | Relocs | Normalized SHA-256 | Relocation destinations |
|---|---:|---:|---|---|
| `__itow` | 43 | 2 | `ed0f767befbd88ede210efccbd2464a4a9d5167ebedbdedbf887a3e8037b67f6` | `__itoa`, `_mbstowcs` |
| `__ltow` | 43 | 2 | `7dcac30f9129eaac51562a842a9e902cfb093fc903fbe83a6a84d423fa3e0682` | `__ltoa`, `_mbstowcs` |
| `__ultow` | 43 | 2 | `7dcac30f9129eaac51562a842a9e902cfb093fc903fbe83a6a84d423fa3e0682` | `__ultoa`, `_mbstowcs` |
| `__i64tow` | 46 | 2 | `6ba639a84d06657e1135e8d5854a9e4c303941f182a1b33d4b90ff2ed9c1597b` | `__i64toa`, `_mbstowcs` |
| `__ui64tow` | 46 | 2 | `6ba639a84d06657e1135e8d5854a9e4c303941f182a1b33d4b90ff2ed9c1597b` | `__ui64toa`, `_mbstowcs` |

CodeView and the function frames prove the narrow scratch-buffer sizes: 20
bytes for `int`, 40 bytes for `long`/`unsigned long`, and 80 bytes for the
64-bit variants. Every wrapper passes that exact buffer size to `mbstowcs`
and returns its caller-owned wide destination.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | XDK-attested narrow conversion, fixed local buffer, `mbstowcs`, destination return | `43/43`; `43/43`; `43/43`; `46/46`; `46/46` | `2/2` each | `ed0f767b...`; `7dcac30f...`; `7dcac30f...`; `6ba639a8...`; `6ba639a8...` | none | `5/5` exact; no runtime data | accepted on first build |

## Rules and disposition

Every source-shape change, if E01 misses, changes one provenance-backed factor
and records strict measurements here. No assembly, `volatile`, forced
inlining, undefined behavior, byte forcing, optimizer pragma, flag change,
raw object patch, or comparator waiver is permitted. The object is not
eligible for `Matching` until all five functions and their COFF ownership pass
the hardened comparator and all project gates.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the retained source with the configured
`/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports
`section_infos_equal` for all five functions, with the exact sizes, normalized
hashes, relocation addresses/types/destinations/addends listed above.
Independent COFF inspection confirms five external, one-byte-aligned,
pick-no-duplicates `.text` COMDATs. The candidate and target own no runtime
`.rdata`, `.data`, `.bss`, or aliases; compiler-only directives and CodeView
are discardable metadata.

A forced rebuild reproduced the strict result. Both full
`halobetacache_build` and `libcmt_build` pass. Regenerated progress reports
335/833 completed objects overall, 267/468 Halo objects, and 68/212 libcmt
objects. Libcmt reports 98/476 exact functions and 5,665/55,015 exact code
bytes; this object contributes one completed object, five functions, and 221
meaningful code bytes. The semantic audit scans 437 units, evaluates 3,535
functions, accepts 3,451 strict functions, and reports zero unit errors. The
object-admission audit finds no candidate, revocation, or contradiction for
this object; its sole contradiction is the unrelated pre-existing
`source/shell/shell_xbox` entry. All 179 tooling tests pass.

## House/Berth and original-bug audit

The retained implementation is readable C directly reflecting the XDK
member's CodeView, frame sizes, calls, and return values. Parameters are
explicitly typed and vertically formatted. The only numeric buffer sizes are
source-level storage capacities proven independently by the frames and the
matching `mbstowcs` third arguments; they are not arbitrary code-generation
magic. The source contains no assembly, `volatile`, forced inlining, undefined
behavior, byte forcing, optimizer pragma, compiler-flag exception, alignment
trick, raw object patch, or comparator waiver.

The five wrappers preserve their original CRT contracts. The caller remains
responsible for supplying an adequately sized wide-character destination, as
with the historical APIs; that documented contract is not a newly discovered
original bug. No target-proven defect warrants a `BUG (original)` annotation.

## Disposition

The complete object is strict exact and eligible for `Matching`. No fuzzy
percentage, semantic approximation, partial-object credit, or allowlist is
used. The clean-commit fail-closed regression snapshot, forced rebuild, and
check classify all five functions as `still_exact`, with no failures,
warnings, changed non-exact functions, newly exact surprises, or ownership
changes.
