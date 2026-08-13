# `libs/libcmt/strtol.obj` exact-match ledger

This atomic lane starts from campaign commit `a466cc97`. At lane start the
object was `MISSING` at config index 824, absent from `config/parked.json`,
absent from every pre-existing Git ref, worktree, and object ledger, and
explicitly cleared by both other active agents.

## Plain-English behavior

The private parser converts text in bases 2 through 36, or detects decimal,
octal, or hexadecimal when the caller passes base zero. It skips whitespace,
handles an optional sign and `0x` prefix, returns where parsing stopped, and
sets range error on signed or unsigned overflow. The two public functions
select signed or unsigned overflow rules and delegate to the same parser.

## Provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\strtol.obj` member has SHA-256
`65520c7133c862db511b335f488804fe33c73977d3e20186ba13b33d57bb6aea`,
an August 2001 timestamp, compiler identifier `0x001c2426`, ordinary C
CodeView, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\strtol.c`.
CodeView recovers the private name `strtoxl`, parameters `nptr`, `endptr`,
`ibase`, and `flags`, and local `number`. The authentic Microsoft Xbox CRT
source supplies the complete topology; the January object and archive member
remain the byte authority.

The target contains three external, one-byte-aligned, pick-no-duplicates
`.text` COMDATs and no runtime data or aliases:

- `_strtoxl`: `0x1a9`, five relocations;
- `_strtol`: `0x17`, one relocation;
- `_strtoul`: `0x17`, one relocation.

The private function relocates to `___mb_cur_max`, `__isctype`, `__pctype`
twice, and `__errno`. Each wrapper relocates to the private function. The
January placeholder `_code_001ca805` is renamed to the CodeView-attested
`_strtoxl` through `config/symbols.json`; this is symbol recovery, not an
invented name.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Authentic Microsoft Xbox CRT control flow with explicit readable equivalents of internal ASCII macros | `_strtoxl` `0x1a9/0x1a9`; wrappers `0x17/0x17` each | `5/5`; wrappers `1/1` each | `_strtoxl` `46aecae9873c...`; `_strtol` `01cc0863c035...`; `_strtoul` `bd7ff4ac36c6...` | none | `3/3` exact; no runtime data | accepted on first build |

## Rules and disposition

Every later experiment, if needed, changes one provenance-backed source factor
and records strict measurements here. No assembly, `volatile`, force-inlining,
undefined behavior, byte forcing, flag change, raw object patch, or comparator
waiver is permitted. No alternate source shape was required: the primary XDK
member, CodeView, and authentic source converged on the first compile.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the retained source with the configured
`/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports
`section_infos_equal` for `_strtoxl`, `_strtol`, and `_strtoul`. The sizes,
relocation counts, relocation addresses/types/destinations/addends, and
normalized hashes are exactly those recorded above. Independent COFF
inspection confirms three one-byte-aligned, pick-no-duplicates `.text`
COMDATs. The candidate owns no runtime `.rdata`, `.data`, or `.bss`; its
additional `.drectve` and CodeView sections are discardable compiler metadata.

A forced object rebuild reproduced the strict three-function result. Both the
full `halobetacache_build` and full `libcmt_build` pass. Regenerated progress
reports 331/833 completed objects overall, 267/468 Halo objects, and 64/212
libcmt objects. Libcmt reports 90/476 exact functions and 4,902/55,015 exact
code bytes; this object contributes one completed object, three functions,
and 471 meaningful code bytes. The semantic audit scans 434 units, evaluates
3,527 functions, accepts 3,443 strict functions, and reports zero unit errors.
The object-admission audit finds no candidate, revocation, or contradiction
for this object; its sole contradiction is the unrelated pre-existing
`source/shell/shell_xbox` entry. All 179 tooling tests pass.

## House/Berth and original-bug audit

The retained implementation is readable C derived from authentic Microsoft
Xbox CRT source and the XDK member's CodeView. Parameters are explicitly typed
and vertically formatted, source constants and parser flags have descriptive
names, and no raw field offsets or opaque layout assumptions are present. The
source contains no assembly, `volatile`, forced inlining, undefined behavior,
byte forcing, optimizer pragma, flag exception, alignment trick, raw object
patch, or comparator waiver. The multiple returns belong to the attested
Microsoft source topology and are byte-proven; the house preference for a
single final return is not grounds to falsify primary provenance.

The parser preserves the original CRT behavior for invalid bases, no-digit
inputs, signed and unsigned overflow, and the end pointer. No target-proven
defect requiring a `BUG (original)` annotation was found. In particular, the
historical overflow behavior is part of the documented conversion contract,
not a newly discovered Halo defect.

## Disposition

The complete object is strict exact and eligible for `Matching`. No fuzzy
percentage, semantic approximation, partial-object credit, or representational
allowlist is used. The clean-commit fail-closed regression snapshot, forced
rebuild, and check classify `_strtoxl`, `_strtol`, and `_strtoul` as
`still_exact`, with no failures, warnings, changed non-exact functions, newly
exact surprises, or ownership changes.
