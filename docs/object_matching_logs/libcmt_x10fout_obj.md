# `libs/libcmt/x10fout.obj` exact-match ledger

## Plain-English behavior

This object converts Microsoft's ten-byte extended floating-point value into
the decimal digit buffer used by the CRT's formatted-output code. It handles
zero, infinity, indefinite values, and both NaN classes, estimates the decimal
exponent, scales the value, generates digits, and performs decimal rounding.

## Scope and provenance

- Atomic base: campaign commit
  `fc8e1b076965ed489f291ee233eaad3e1ffaf419`.
- Config index 647; initially `MISSING`, unparked, and absent from production
  source, object logs, all refs, and all worktrees.
- The authentic XDK 3911 archive member reports CodeView source
  `d:\xbox-finalfre\private\sdktools\crt\fpw32\conv\x10fout.c` and compiler
  ID `0x001c2426`.
- XDK 3911 `libcmt.lib` SHA-256:
  `60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
- The matching historical Microsoft source is repository
  `xerohour/xbox_leak_may_2020`, commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, blob
  `7e9106227dc9543ac5bca9a216e0fdf1b0b486cc`.
- Before any production edit, the authentic archive function is strict-exact
  against January: 631 bytes, 12 relocation identities/addends, normalized
  SHA-256
  `680a37145127a3fc46fe86768557819c28826e310a95b69f9b5133f108d03a6d`.
  The only runtime data consists of the four special-value string COMDATs
  `1#SNAN`, `1#IND`, `1#INF`, and `1#QNAN`.

## Measured source shapes

| ID | One changed factor | Strict result | Decision |
|---|---|---|---|
| E01 | Preserve the authentic declaration, branch, call, and loop order while expressing the internal types and access macros in the project's existing readable CRT style | 638 bytes and 14 relocations: VC7 left the two shared `strcpy` tails out of line | Rejected: correct behavior, but two compiler-dependent library calls remained |
| E02 | Restore the historical private-header typedef, member, macro, and declaration grouping exactly | Identical 638-byte/14-relocation output | Rejected: proves identifier and header spelling were not the control |
| E03 | Express the known six- and seven-byte special-value copies as fixed-size aggregate assignments | **EXACT**: 631/631 bytes, 12/12 relocation identities/addends, and the normalized hash agrees | Accepted: directly expresses the compile-time copy size and reproduces the authentic `movsd`/`movsw`/`movsb` lowering without an intrinsic or flag change |
| E04 | Replace the aggregate copies with standards-library `memcpy` calls of the same constant sizes | 651 bytes and 14 relocations; the two shared copy tails remained external `_memcpy` calls | Rejected and restored: confirms the accepted shape is not hiding an available library-call solution under the unchanged flags |

## Validation and house/Berth audit

- Only XDK 3911 CL 13.00.9254.1 with the unchanged libcmt `/O1 /Gy` flags is
  permitted.
- The expected greenfield regression snapshot cannot name an absent base
  object; final admission therefore requires strict classification of every
  code and runtime-data section followed by a clean-commit forced regression
  snapshot/check.
- No assembly, `volatile`, forced inline/noinline, codegen pragma, byte patch,
  flag change, comparator exception, or relocation waiver is allowed.
- No original behavioral defect has yet been proven.
- Hardened comparison reports the function and all four string COMDATs exact.
  Their bytes, relocation identities/addends, external owner names, and
  runtime section inventories agree; the object owns no aliases or writable
  data.

The fixed-size aggregate copies are not byte patches. Each copies the complete
known string object, including its terminator, into a 22-byte destination.
They state the operation's real six- or seven-byte extent in ordinary C and
avoid depending on an unavailable private-header `strcpy` intrinsic setting.

## Final gates

- Forced XDK rebuild under unchanged `/O1 /Gy`: passed.
- Hardened whole-object comparison: `_I10_OUTPUT` and all four target-owned
  strings are strict-exact, including bytes, relocation identities/addends,
  external owners, and runtime section inventory.
- Full `halobetacache_build` and `libcmt_build`: passed.
- Progress admits 364/833 objects overall and 97/212 libcmt objects. The
  libcmt totals are 11,952/55,015 code bytes, 155/476 functions, and
  5,084/8,637 data bytes.
- Semantic audit: 465 units, 3,592 functions evaluated, 3,508 accepted exact,
  and zero unit errors.
- Admission audit: zero candidates and zero revocations; its sole
  contradiction is the pre-existing unrelated `source/shell/shell_xbox`
  completion-label item.
- The complete repository tooling suite passes 179/179 tests.
- Final clean-commit forced regression snapshot/check: passed with
  `still_exact = ["_$I10_OUTPUT"]`, no changed-nonexact or newly-exact
  functions, no failures, and no warnings.
