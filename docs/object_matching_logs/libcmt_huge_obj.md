# `libs/libcmt/huge.obj`

## Plain-English purpose

This object owns the C runtime's positive-infinity sentinel. Math routines use
the public `_HUGE` value (also exposed as `HUGE_VAL`) when a result overflows.
There is no executable code in this translation unit: the complete January
object is one writable eight-byte global.

## January evidence

- Object status before this lane: `MISSING`.
- Functions: none.
- Owned data: `__HUGE`, external, at offset zero in `.data`.
- Section size/alignment: 8 bytes, 4-byte aligned, readable and writable.
- Relocations and aliases: none.
- Bytes, little-endian: `00 00 00 00 00 00 F0 7F`.
- Meaning: IEEE-754 binary64 positive infinity (`0x7FF0000000000000`).

The XDK's `Math.h` declares `_HUGE` as the runtime's error-return value. The
XDK's `YMath.h` independently demonstrates Microsoft's word-union convention
for non-finite floating constants. A locally installed Microsoft CRT source
(`vcruntime/huge.c`) supplies the stronger source-provenance clue: Microsoft
defines `_HUGE` as two 32-bit IEEE words, rather than asking C to construct an
infinity-valued static floating initializer. The modern source makes those
words `const`; January's object is writable `.data`, so the recovered Xbox form
is the corresponding non-`const` two-word definition.

## Source shape accepted

```c
unsigned int _HUGE[2] = { 0x00000000, 0x7FF00000 };
```

On this 32-bit target, each `unsigned int` is four bytes. The declaration is a
readable description of the IEEE representation used by the CRT, not an
anonymous byte array, type-punning expression, linker trick, or post-build
patch. It emits exactly one external `__HUGE` symbol in writable `.data`.

## Measured experiment log

All experiments used XDK 3911 CL 13.00.9254.1 with this unit's configured
`/O1 /Gy` flags. Rejected candidates were replaced before admission.

| Candidate | Result | Reason rejected |
| --- | --- | --- |
| `double _HUGE = 1e+300 * 1e+300;` | `8C F2 1A 00 00 E0 AC 03` | VC7 did not materialize binary64 infinity. |
| `double _HUGE = (float)(1e+300 * 1e+300);` | `8C F2 1A 00 14 97 B3 8E` | The float cast did not reproduce the target representation. |
| `double _HUGE = DBL_MAX + DBL_MAX;` | `8C F2 1A 00 00 F8 FF FF` | Static folding produced the wrong non-finite representation. |
| `double _HUGE = 1.7976931348623159e+308;` | compile error C2177 | Literal is outside VC7's accepted range. |
| `double _HUGE = 1.7976931348623158e+308;` | `FF FF FF FF FF FF EF 7F` | This is finite `DBL_MAX`, not infinity. |
| `double _HUGE = 1.0 / 0.0;` | compile error C2099 | VC7 does not accept it as a static constant initializer. |
| two 32-bit IEEE words | exact | Microsoft CRT provenance and all strict object evidence agree. |

## Strict result

- Target size: 8 bytes; candidate size: 8 bytes.
- Target relocations: 0; candidate relocations: 0.
- Target normalized SHA-256:
  `9402bb655bc3b7331a00f1219ef647565bbe517c74aa0e41026885785867d1cd`.
- Candidate normalized SHA-256: the same.
- `tools.coff_compare` for `__HUGE`: `equal: true`, `all_equal: true`.
- COFF inspection: same external symbol name, offset, `.data` ownership,
  section size, alignment, read/write characteristics, and bytes; no hidden
  code, other data, relocations, COMDAT aliases, or ownership spill.
- Objdiff/progress: 100% data, `complete_units: 1`.

## House-rule, Berth-rule, and bug audit

- Natural readable C only: no assembly, `volatile`, undefined behavior,
  pointer punning, optimizer pragma, special compiler flag, forced byte array,
  post-build mutation, or fuzzy credit.
- The object was reconstructed atomically and was not admitted from an older
  branch or a fuzzy match.
- The configured compiler and `/O1 /Gy` flags were unchanged.
- There is no original Halo/CRT bug to annotate here. Positive infinity is the
  intended sentinel, and the recovered word representation is deliberate.
- No neighboring production translation unit or canonical worktree was edited.

## Campaign and regression gates

- The complete `halobetacache_build` and `libcmt_build` passed under the
  unchanged generated build.
- Regenerated progress admitted the entire unit. At this lane's snapshot,
  libcmt reports 31/212 complete objects, 30/476 functions, 1,937/55,015 code
  bytes, and 212/8,637 data bytes. This data-only object contributes exactly
  one completed object and eight exact data bytes; it contributes no function
  or code-byte credit.
- The semantic audit scanned 407 units and 3,467 available functions, found
  zero unit errors, and retained 3,383 accepted exact functions. The admission
  audit found no candidate or revocation caused by this change; its sole
  contradiction is the unrelated pre-existing `source/shell/shell_xbox` item.
- The tooling suite passed 179/179 tests.
- After the atomic commit, the fail-closed regression snapshot/check for
  `libs/libcmt/huge` passed with no failures, warnings, changed non-exact
  siblings, unexpected new matches, or ownership changes. Its `still_exact`
  function list is correctly empty because this unit has no code; the gate
  freezes the full non-code section and symbol-ownership manifest instead.
- `git diff --check`, JSON parsing, the prohibited-construct scan, and final
  scope review were clean.
