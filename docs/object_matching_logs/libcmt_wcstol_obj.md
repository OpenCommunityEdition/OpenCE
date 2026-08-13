# `libs/libcmt/wcstol.obj` exact-match ledger

This atomic lane starts from campaign commit `254e9ad6`. The object was
`MISSING` at config index 817, absent from `config/parked.json`, every Git ref,
worktree, and object ledger, and explicitly cleared by the other active lanes.

## Plain-English behavior

The private parser converts wide text in bases 2 through 36, or detects
decimal, octal, or hexadecimal when the caller supplies base zero. It skips
wide whitespace, handles an optional sign and `0x` prefix, records where
parsing stopped, and reports range error on signed or unsigned overflow. The
two public wrappers select signed or unsigned overflow behavior.

## Primary provenance and January inventory

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\wcstol.obj` member has SHA-256
`d5830b2203a1e5f63759fefc82efc39d0738270c0c697e263d235e96ef2be846`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\wcstol.c`. The public
historical Microsoft Xbox CRT source is pinned by Git blob
`ef81a6ac7b8cce1b35d851aadbc1571a4da26180` and supplies the complete parser
topology. January bytes remain authoritative.

The object owns three one-byte-aligned, pick-no-duplicates code COMDATs and no
runtime data:

- private `_wcstoxl`: 453 bytes, five relocations, normalized SHA-256
  `fec4e2fd56420b14266bbc08dbbc011a855baeffda513dcbdf2d62ba805b2f41`;
- `_wcstol`: 23 bytes, one relocation, normalized SHA-256
  `01cc0863c03520883efa0eae860bfe29ba9436b257a1f973e43d185abd24b404`;
- `_wcstoul`: 23 bytes, one relocation, normalized SHA-256
  `bd7ff4ac36c685f4fdc739a8f7c9a1ae655997b07360cce2169a29136d5dc6c3`.

The private parser relocates to `_iswctype`, `__wchartodigit` three times, and
`__errno`, at the exact January addresses with zero addends. Both wrappers
relocate to `_wcstoxl`. The January placeholder `_code_001ccafc` is renamed to
the CodeView-attested `_wcstoxl` through `config/symbols.json`.

The authentic archive and rebuilt candidate retain `_wcstoxl` as a
file-private symbol. The linked-image split necessarily synthesizes the old
`code_*` owner as external because private COFF symbols do not survive the
link. This is the established csplit private-symbol representation, not a
source-linkage waiver: the original XDK member independently proves the
private linkage and exact COMDAT, while all runtime bytes and relocation
destinations compare strictly after the recovered name is applied.

## Experiment matrix

| ID | One source factor | Sizes T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Authentic Microsoft parser with a mistakenly underscored declaration spelling `_iswctype` | `453/453`, `23/23`, `23/23` | `5/5`, `1/1`, `1/1` | normalized bytes exact, but the private parser relocated to the distinct C symbol `__iswctype` | rejected; destination identity is wrong |
| E02 | Declare and call the standard C function `iswctype`, which decorates to January `_iswctype` | `453/453`, `23/23`, `23/23` | `5/5`, `1/1`, `1/1` | all three sections and every relocation identity/addend strict exact | accepted |

The corrected spelling is semantic C linkage, not a code-generation nudge.
No statement-order or compiler-control experiment was needed.

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt
`/O1 /Gy /I"xbox/include"` flags. `section_infos_equal` passes for all three
functions after the provenance-backed private-name recovery. The extracted
XDK archive member has the same three runtime sections, hashes, relocation
addresses/types/destinations/addends, and private/public linkage.

The full `halobetacache_build`, `libcmt_build`, and `progress` targets pass.
The lane reports 338/833 completed objects overall and 71/212 libcmt objects.
Libcmt has 99/476 exact functions, 6,442/55,015 exact code bytes, and
1,368/8,637 exact data bytes. This object contributes one completed object,
three functions, and 499 exact code bytes. The semantic scan reports zero unit
errors and the admission audit has no candidate or revocation for this object;
its only contradiction is the pre-existing unrelated `shell_xbox` item.

A clean-commit fail-closed regression snapshot/check followed by a forced
object rebuild retained `_wcstoxl`, `_wcstol`, and `_wcstoul` as
`still_exact`, with no failures or warnings. The complete tool suite passes
179/179 tests.

## House/Berth and original-bug audit

The retained implementation follows the authentic Microsoft C structure,
uses descriptive parser flags and limits, typed `wchar_t` pointers, named
interfaces, and vertically formatted parameters. It contains no assembly,
`volatile`, forced inlining, undefined sequencing, byte forcing, optimizer
pragma, flag change, opaque layout, raw offset, or comparator exception. The
early invalid-base return and final return are primary-proven and reproduce
the target control flow naturally.

No target-proven correctness defect was found. Invalid bases, no-digit input,
signed and unsigned overflow, and end-pointer behavior all implement the
documented CRT contract. The deliberate two's-complement unsigned handling of
the most-negative signed value is part of this compiler/runtime contract.
