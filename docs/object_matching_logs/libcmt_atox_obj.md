# `libs/libcmt/atox.obj` exact-match ledger

This atomic lane starts from campaign commit `1b270ed1`. At lane start the
object was `MISSING` at config index 829, absent from `config/parked.json`,
absent from every Git ref and worktree, and explicitly cleared by the other
active agents.

## Plain-English behavior

The object converts a decimal character string to a 32-bit `long`, an `int`,
or a signed 64-bit integer. It skips multibyte-aware whitespace, accepts one
optional sign, accumulates consecutive ASCII digits, and stops at the first
non-digit. Like the original CRT routines, it deliberately does not detect
overflow.

## Provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\atox.obj` member has SHA-256
`414a48ba34115653d6328cdabb630913ec2d3e7724465bbbc3cfc058de4db33d`,
an August 11 2001 timestamp, compiler identifier `0x001c2426`, and CodeView
path `d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\atox.c`.
The member has ordinary C line records and an `nptr` parameter for every
routine, so this is readable Microsoft CRT C rather than vendor assembly.
The historical Microsoft VC6 `ATOX.C` source supplies the same three-function
topology and named locals; the XDK member and January bytes remain authoritative
where the Xbox source differs in multibyte whitespace handling.

The archive member is strict-exact to all January runtime sections:

- `_atol`: `0x58`, one REL32 relocation at `+0x0d` to `__ismbcspace`,
  normalized SHA-256 `55c29fdec506235848b808ca77a69bab73380b0706768c90e92af43388bb0274`;
- `_atoi`: `0x5`, one REL32 relocation at `+0x01` to `_atol`, normalized
  SHA-256 `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449`;
- `__atoi64`: `0x79`, REL32 relocations at `+0x10` to `__ismbcspace` and
  `+0x4f` to `__allmul`, normalized SHA-256
  `0adf23c553c71c684e4c1258f7d32d915bbbcf3effc89f83db0a5084c3e308d9`.

All three are external, one-byte-aligned, pick-no-duplicates `.text` COMDATs.
There are no target-owned runtime data sections and no aliases.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Historical Microsoft topology, explicit multibyte whitespace call, direct ASCII digit range | `_atol` `0x58/0x4c`; `_atoi` `0x5/0x5`; `__atoi64` `0x79/0x7e` | `1/1`; `1/1`; `2/2` | `_atol` `134e7a0c4a13...`; `_atoi` exact; `__atoi64` `72977a8d4e33...` | `_atol +0x2c`: folded range-loop topology; `__atoi64 +0x00`: extra frame | `_atoi` exact; object `1/3` | reverted |
| E02 | Attested Microsoft source spelling `while (isdigit(character))` with a direct CRT prototype | `_atol` `0x58/0x58`; `_atoi` `0x5/0x5`; `__atoi64` `0x79/0x83` | `1/2`; `1/1`; `2/3` | `_atol` `b41d2d9b62a3...`; `_atoi` exact; `__atoi64` `0e78624c34dd...` | extra `_isdigit` call relocations; wrong whitespace-call addresses | `_atoi` exact; object `1/3` | reverted |
| E03 | Convert each raw character to `0..9` or `-1`, but retain the historical body's second `-'0'` | `_atol` `0x58/0x59`; `_atoi` `0x5/0x5`; `__atoi64` `0x79/0x86` | `1/1`; `1/1`; `2/2` | `_atol` `633500376c94...`; `_atoi` exact; `__atoi64` `ade927290489...` | `_atol +0x46`: surplus second digit subtraction; `__atoi64`: corresponding live-range cascade | `_atoi` exact; object `1/3` | reverted; not behaviorally correct |
| E04 | Convert raw character once to a digit-or-`-1` sentinel and accumulate that converted digit | `_atol` `0x58/0x58`; `_atoi` `0x5/0x5`; `__atoi64` `0x79/0x79` | `1/1`; `1/1`; `2/2` | `_atol` `55c29fdec506...`; `_atoi` `e8ebd827d1f3...`; `__atoi64` `0adf23c553c...` | none | `3/3` exact; no runtime data | accepted |

## Do-not-repeat and disposition

No code-generation perturbation is permitted. If the source-shaped first build
misses, each evidence-backed difference will be changed one factor at a time,
measured here, and reverted unless it improves the strict whole-object result.
The direct ASCII range of E01 and the out-of-line `isdigit` call of E02 must
not be repeated. E03 is not behaviorally correct because it subtracts `'0'`
twice. No inline assembly, `volatile`, force-inlining, per-function flag,
undefined-behavior, or byte-patch experiment was attempted or is permitted.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled the retained source with the configured
`/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports
`section_infos_equal` for `_atol`, `_atoi`, and `__atoi64`, with the sizes,
hashes, relocation addresses, relocation types, destinations, and addends
recorded above. Independent COFF inspection confirms the same three external,
one-byte-aligned, pick-no-duplicates `.text` COMDATs. The candidate owns no
runtime `.rdata`, `.data`, or `.bss`; its extra `.drectve` and CodeView
sections are discardable compiler metadata.

A clean forced rebuild reproduced the exact three-function result. Both the
full `halobetacache_build` and full `libcmt_build` pass. Regenerated progress
reports 324/833 objects overall, 267/468 Halo objects, and 57/212 libcmt
objects. Libcmt reports 79/476 exact functions and 3,985/55,015 exact code
bytes; this object contributes one completed object, three functions, and 214
meaningful code bytes. The semantic audit scans 427 units, evaluates 3,516
functions, accepts 3,432 strict functions, and reports zero unit errors. The
object-admission audit finds no uncredited candidate and no revocation; its
one contradiction is the unrelated pre-existing `source/shell/shell_xbox`
entry. All 179 tooling tests pass. The clean-commit fail-closed regression
snapshot, forced rebuild, and check classify `_atol`, `_atoi`, and `__atoi64`
as `still_exact`, with no failures, warnings, changed non-exact functions,
newly exact surprises, or ownership changes.

## House/Berth and bug audit

The retained implementation is readable C, follows the historical Microsoft
topology and the XDK member's exact digit-sentinel evidence, and uses explicit
parameter and local types. It contains no assembly, `volatile`, forced
inlining, undefined behavior, byte forcing, opaque code-generation cast, raw
field offset, alignment trick, compiler-flag exception, or comparator waiver.
The routines intentionally inherit the original CRT contract that overflow is
not detected. That documented contract is not newly classified as an original
Halo bug, so no `BUG (original)` annotation is warranted.

## Disposition

The complete object is strict exact and eligible for `Matching`. No fuzzy
percentage, semantic approximation, or partial-object credit is used.
