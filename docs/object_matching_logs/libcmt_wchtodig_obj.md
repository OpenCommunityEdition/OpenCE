# `libs/libcmt/wchtodig.obj` exact-match ledger

## Plain-English behavior

`_wchartodigit` recognizes the decimal digits used by eighteen Unicode writing
systems. If its 16-bit character lies between a script's digit zero and digit
nine, it returns the numeric value 0 through 9. It returns `-1` for every other
character.

The ranges include ASCII, Arabic-Indic, Extended Arabic-Indic, Devanagari,
Bengali, Gurmukhi, Gujarati, Oriya, Telugu, Kannada, Malayalam, Thai, Lao,
Tibetan, Myanmar, Khmer, Mongolian, and fullwidth digits.

## Scope and provenance

- Source: `libs/libcmt/wchtodig.c`.
- January target: `libs/libcmt/wchtodig.obj`, object index 819.
- Compiler and unchanged flags: XDK 3911 CL 13.00.9254.1, `/O1 /Gy`.
- Primary binary provenance: `obj\i386\wchtodig.obj`, extracted from the exact
  XDK 3911 `libcmt.lib`.
- CodeView identifies `wchtodig.c`, the public function `_wchartodigit`, 26
  source-line records, and the original path
  `d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\wchtodig.c`.
- Microsoft's corresponding Xbox CRT source proves the ordered range-test
  macro, every Unicode digit-zero constant, the special placement of the
  fullwidth range, and the `wchar_t`/`int` prototype.
- The archive member is strict-identical to January before any source work.
  This proves the object is ordinary Microsoft C, not hand-written assembly.

## January inventory and strict result

The whole runtime object consists of one public code COMDAT and no runtime
data:

| Function | Size | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `__wchartodigit` | 374 | 0 | `ab061ecd5a973949dd2402d078342213950ccccad26ccb0bac8fe4017c56bbbe` |

Target and candidate have identical `.text` flags, 374 bytes, normalized
bytes, zero relocations, and the same external function symbol at section
offset zero with type `0x20` and storage class 2.

## Accepted readable source shape

- Each digit range performs two ordered unsigned 16-bit comparisons: reject a
  character below the range, then return `character - zero` if it is below
  `zero + 10`.
- The repeated source topology is expressed by a small range-test macro, as in
  Microsoft's authenticated source. This preserves readable intent without
  duplicating 54 lines of nearly identical control flow manually.
- All eighteen range starts and the count of ten digits have descriptive enum
  names. They are not unexplained magic numbers.
- Fullwidth digits are tested after the lower Unicode ranges. The surrounding
  `character < 0xFF10` split is authentic and important: it lets the compiler
  share the range-zero register while retaining the January comparison order.

## Experiment matrix

| ID | Source/build shape | Size T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E00 | January target vs extracted XDK archive member | 374/374 | 0/0 | identical normalized SHA-256 | provenance accepted |
| E01 | Authenticated ordered range macro, typed `wchar_t`, and named range constants under unchanged `/O1 /Gy` | 374/374 | 0/0 | strict exact, whole symbol ownership equal | retained |

No tuning or speculative shape search was necessary. No compiler flag was
added or changed.

## Whole-object ownership and alias audit

- Target and rebuilt object each own exactly one runtime `.text` COMDAT.
- Neither owns `.data`, `.rdata`, or `.bss`; there are no constants emitted as
  data, aliases, strings, imports, relocations, or candidate-only helpers.
- The named enum values are compile-time integers and create no data symbols.
- Candidate `.debug$S`, `.debug$F`, and `.drectve` sections are compiler
  metadata and do not contribute linked runtime ownership.

## Do-not-repeat list

- Do not replace the proven ordered chain with a lookup table, binary search,
  locale API, switch, or generic Unicode library call. Those change code,
  ownership, and often behavior.
- Do not reorder ranges merely by numeric value; the fullwidth split and the
  compiler's reuse of the current range-zero value are target-proven.
- Do not change `wchar_t` to signed `short` or widen it to `long`; the target's
  unsigned 16-bit branches and zero extensions prove the type.
- Assembly, byte patches, `volatile`, forced inline, optimizer pragmas, flag
  changes, undefined behavior, and fuzzy credit remain prohibited.

## House-rule, Berth-rule, readability, and bug audit

- The implementation is ordinary typed C with a descriptive parameter,
  semantic Unicode names, and explicit returns.
- All constants are named by their Unicode script/range meaning. The only
  arithmetic constant, ten, is named `unicode_digit_count`.
- Prototype, width, range ordering, and behavior are proven jointly by the
  Microsoft source, XDK CodeView, archive bytes, and January target.
- No original defect is present. Returning `-1` for characters outside the
  supported decimal ranges is the documented CRT contract, so no preserved-
  bug annotation is warranted.

## Residual classification and disposition

There is no residual. The complete object is strict exact and is admitted as
`Matching`.

## Final validation

- A forced rebuild of `build/base/libs/libcmt/wchtodig.obj` with XDK 3911 CL
  13.00.9254.1 and the unchanged `/O1 /Gy` flags succeeded.
- Hardened comparison reports 374/374 bytes, 0/0 relocations, identical
  normalized SHA-256, and `all_equal: true` for `__wchartodigit`.
- The generated unit report records 1/1 functions and 374/374 code bytes
  exact, 100% data, and the unit complete.
- Full `halobetacache_build` and `libcmt_build` both succeeded.
- Project progress after rebasing onto canonical `0261a4b3` and admitting this
  object is 335/833 complete objects, 3,442/11,060 exact functions,
  411,824/2,198,102 exact code bytes, and 1,793,834/4,176,062 exact data
  bytes.  The libcmt category is 68/212 objects, 94/476 functions,
  5,818/55,015 code bytes, and 1,368/8,637 data bytes.
- Semantic audit reports 3,363 semantic-exact functions, 3,447 accepted-exact
  functions, 93 hidden exact functions, and zero unit errors.  It explicitly
  places `__wchartodigit` in both `semantic_exact` and `accepted_ledger`.
- Admission audit reports zero candidates and zero revocations.  Its one
  contradiction is the pre-existing unrelated `source/shell/shell_xbox`
  baseline item.
- All 179 tooling tests pass.  Every JSON file in `config/` parses, the source
  contains no prohibited construct, and `git diff --check` is clean.
- The clean-commit regression gate forced a rebuild and returned `ok: true`,
  no failures, no warnings, no changed-nonexact functions, and
  `__wchartodigit` in `still_exact`.
