# `libs/libcmt/ismbspc.obj` exact-match ledger

This isolated atomic lane starts from authoritative campaign commit
`0261a4b3`. Configuration index 777 was `MISSING`; the unit was absent from
`config/parked.json`, production history, matching logs, branches, worktrees,
and every active peer claim.

## Plain-English behavior

`_ismbcspace` answers whether one character is whitespace. Values above 255
could represent a two-byte multibyte character, but this Xbox CRT build does
not classify those values here and returns false. For an ordinary byte it uses
the CRT's locale-aware `isspace` machinery. That machinery calls `_isctype`
when a multibyte locale is active and otherwise indexes the current byte-type
table directly.

## Provenance and January inventory

The exact `obj\i386\ismbspc.obj` member was extracted from the local XDK 3911
`libcmt.lib`. CodeView names the original source
`d:\xbox-finalfre\private\sdktools\crt\crtw32\mbstring\ismbspc.c` and carries
ordinary function/line records, proving compiled C rather than assembly.
Microsoft's corresponding 1985-2001 source was recovered from the pinned
Easy-Build-2k NT source mirror at commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`. It confirms the one-byte guard
and `isspace` classifier topology; the January machine code proves that this
Xbox build selects the false result for values above one byte.

The authenticated archive function compares strict-equal to January before
any production source is written:

- symbol: `__ismbcspace`
- size: 48 bytes
- relocations: 3
- normalized SHA-256:
  `d5e4dbfe19e55cfe1d691deb199708de1e4f36a222c64fc64285311d66155b8b`
- relocation identities: `+0x10 -> ___mb_cur_max`,
  `+0x1B -> __isctype`, and `+0x24 -> __pctype`

January owns one `.text` section and no data, BSS, or read-only data. The
archive's select-any CRT build-number metadata is not owned by January and is
not reproduced.

## Measured source-shape experiments

All measurements use XDK 3911 `CL 13.00.9254.1` under the inherited,
unchanged `/O1 /Gy` command.

| ID | Readable-C source shape | Result |
|---|---|---|
| E00 | Natural one-byte guard using named `UCHAR_MAX`, with only `<ctype.h>` included | Compile-time rejection: this historical header does not transitively declare `UCHAR_MAX`. No object was produced. |
| E01 | Add the proper `<limits.h>` dependency; keep `if (character > UCHAR_MAX) return 0; return isspace(character);` | Strict exact on the first successful build: 48/48 bytes, 3/3 relocations with identical address/type/destination/addend, and equal normalized hash. Accepted. |

No alternate control-flow, declaration-order, or codegen experiment was
needed. The source is the ordinary named-constant expression of the recovered
Microsoft algorithm.

## Original-behavior and bug audit

No original bug was found. Returning false for values above `UCHAR_MAX` is a
deliberate contract of this Xbox build, not a truncated Unicode test: this API
accepts an encoded multibyte character value, while the selected runtime path
supports only the byte classifier here. The reconstruction preserves that
choice exactly rather than attempting a modern Unicode interpretation.

## House, Berth, and readability audit

The function is ordinary typed C with its parameter on its own line, a named
standard-library bound, and explicit returns. It has no raw object/tag access,
opaque offsets, invented layout, or magic classifier bit in production source;
`isspace` exposes the intended CRT abstraction.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
codegen cast, compiler-flag change, alignment trick, pragma, comparator
exception, or prebuilt-object substitution is used.

## Final validation

- The candidate object was deleted and force-rebuilt with XDK 3911
  `CL 13.00.9254.1` under unchanged `/O1 /Gy`; the hardened comparison
  remained strict exact.
- Whole-object runtime ownership is exact: one 48-byte `.text` COMDAT, three
  relocations, and no data, BSS, read-only data, or unexpected external
  definition. Compiler directive/debug sections are not linked ownership.
- Full `halobetacache_build` and `libcmt_build`: successful.
- `progress`: successful; this unit contributes one function and 48 code
  bytes, with libcmt at 68/212 complete objects in this isolated baseline.
- `semantic_progress` and explicit semantic audit: zero unit errors and this
  function accepted by strict COFF evidence.
- Admission audit: no unmarked completion candidate or newly contradicted
  label. The sole `shell_xbox` contradiction is the known unrelated global
  finding.
- Tooling unit tests: 179/179 passed.
- `git diff --check`: clean.
- Clean per-unit regression snapshot/check: force-rebuilt the committed unit,
  retained `__ismbcspace` as `still_exact`, and reported no changed non-exact
  function, failure, or warning.
