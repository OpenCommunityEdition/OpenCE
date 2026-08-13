# `libs/libcmt/strnicmp.obj` exact-match ledger

This atomic lane treats prior commit `1881d2c6` only as an untrusted source
proposal. It screens active ownership and the parked ledger, transplants the
readable C into a fresh worktree from the authoritative campaign tip, and
regenerates all acceptance evidence from scratch.

## Plain-English behavior

The function compares at most `count` bytes from two strings without regard to
ASCII letter case. A zero count compares equal immediately. In the C locale it
hands the work to the CRT's specialized ASCII helper. In other locales it
lowercases each byte with `tolower`, stopping when the limit is exhausted, the
left string ends, or the folded bytes differ, and returns their integer
difference.

The casts to `unsigned char` are semantic, not codegen tricks: the C library's
character classification/conversion functions require either EOF or a value
representable as unsigned char. Passing a negative signed byte would be
undefined behavior.

## January target inventory

- Function: `__strnicmp`, with a csplit same-address alias `_strnicmp`
- Padded code size: 85 bytes
- Four relocations: `___lc_handle + 8`, `___ascii_strnicmp`, and two calls to
  `_tolower`
- Owned runtime data: none

## Source-shape experiment record

| ID | Readable C shape | Measured result |
|---|---|---|
| E01 | Conventional early return for zero count | Correct main body, but the four-byte zero epilogue was emitted first and shifted all relocations |
| E02 | Initialize a result and use one final return | Put the zero epilogue at the tail, but shortened the body by six bytes through different register allocation |
| E03 | Enclose work in `if (count)`, keep direct helper/locale returns, then return zero | Candidate source under final validation; preserves January's three physical exits |

The multiple returns are a measured exception to the preferred single-return
house rule. E02 proves that forcing one return changes the target code rather
than merely improving style.

## Strict verification

- XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the source with the generated
  `/O1 /Gy /I"xbox/include"` flags.
- Both target aliases share the same section and address. Target `_strnicmp`
  and target `__strnicmp` each compare strict-equal to candidate `__strnicmp`.
- Size: `85/85`; relocations: `4/4`.
- Normalized SHA-256, target and candidate:
  `2d81a1231fa8dcb0bd4bcfb82aba82e07491b40e725ffd6c3f66b7e3b01df377`.
- Relocations match in address, type, destination, and addend:
  `___lc_handle + 8` at `+0x0B`, `___ascii_strnicmp` at `+0x14`, and
  `_tolower` at `+0x26` and `+0x32`.
- Each side owns one 85-byte runtime `.text` section. Neither side owns runtime
  `.data`, `.bss`, or `.rdata`.

## Current-campaign admission gates

- Full `halobetacache_build` and `libcmt_build`: passed.
- `semantic_progress`: passed with zero unit errors.
- `progress`: credits one 85-byte function and one complete libcmt object.
  Global totals are omitted because concurrent atomic admissions make them
  stale without changing this object's proof.
- The semantic audit records `__strnicmp` with both `objdiff` and
  `semantic-coff` proof. The admission audit reports no `strnicmp`
  contradiction, revocation, or unreviewed candidate.
- The clean-commit whole-object regression snapshot/check is run before
  handoff and must retain only `__strnicmp` as `still_exact`, with no ownership
  change or warning.
- `python -m unittest discover -s tools -p 'test_*.py'`: 179/179 passed.

No historical build artifact was accepted as proof.

## House/Berth and readability audit

All parameters are on separate lines, declarations are typed, every return is
explicit, and the loop expresses the actual string comparison rather than
compiler-oriented machine steps. `(void)` is inapplicable because the function
takes parameters. No tag, datum, or object access needs a project-specific
typed helper.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, raw structure offset, comparator
exception, or codegen-only cast is used. No original January bug was found, so
no bug-preservation comment is needed.
