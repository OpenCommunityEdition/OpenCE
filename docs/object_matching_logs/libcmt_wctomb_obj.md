# `libs/libcmt/wctomb.obj` exact-match ledger

This atomic lane starts from authoritative campaign commit `752fcc7d`. Before
editing, the unit was confirmed `MISSING`, absent from `config/parked.json`,
absent from prior source and object-ledger history, absent from active
worktrees, and unclaimed by the other active agents.

## Plain-English behavior

This Xbox CRT uses a single-byte character encoding. `wctomb` returns zero
when passed a null destination because there is no shift state to reset. If a
wide character does not fit in one unsigned byte, it reports `EILSEQ` through
the thread-local CRT `errno` and returns `-1`. Otherwise it writes the low byte
to the caller's destination and returns one byte converted.

The cast to `char` is semantic: the preceding `UCHAR_MAX` range check proves
that the value fits in one byte before it is stored.

## Primary provenance and January inventory

Member `obj\i386\wctomb.obj` was extracted directly from the XDK 3911
`libcmt.lib`. It identifies its original source as
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\wctomb.c`; its ordinary
C function and line records (`.bf`, `.lf`, and `.ef`) prove it is compiled C,
not handwritten or vendored assembly.

Before any source was written, the extracted archive member compared
strict-equal to the January split target:

- one externally linked function, `_wctomb`, 43 bytes;
- one `REL32` relocation at function offset `+0x17`, resolving to `_errno + 0`;
- normalized SHA-256
  `c3ed067014adce1c905164b62e578bb68c00f48fdfcee67db9a3f227d534426a`;
- no owned runtime `.data`, `.bss`, or `.rdata`.

XDK `stdlib.h` proves the public prototype `int wctomb(char *, wchar_t)`;
`errno.h` proves `EILSEQ == 42` and the multithreaded `_errno()` accessor; and
`limits.h` supplies the named single-byte ceiling `UCHAR_MAX`.

## Source-shape experiment record

| ID | Readable C shape | Measured result |
|---|---|---|
| E01 | Null destination returns zero; reject `character > UCHAR_MAX` with public `errno = EILSEQ` and `-1`; otherwise store `(char)character` and return one | Correct control flow, but 42 bytes with a `DIR32` store to process-global `_errno`; the standalone reconstruction TU lacks the original CRT's internal `_MT` define |
| E02 | Express the proven thread-local contract directly as `*_errno() = EILSEQ`, leaving all other source unchanged | **Strict exact.** Restored the January call and `REL32` relocation without changing compiler flags; target, candidate, and archive member are identical |

No assembly, byte patch, opaque offset, undefined behavior, `volatile`, forced
inline, optimization pragma, codegen-only cast, or compiler-flag change is
permitted in this lane.

## Strict verification

XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the final source under the generated
`/O1 /Gy /I"xbox/include"` flags. The January target, candidate, and extracted
XDK archive member agree on every runtime property:

- 43/43/43 code bytes and one function;
- one `REL32` relocation at `+0x17`, resolving to external `_errno + 0`;
- normalized SHA-256
  `c3ed067014adce1c905164b62e578bb68c00f48fdfcee67db9a3f227d534426a`;
- external function symbol `_wctomb` at offset zero, type `0x20`, storage
  class 2;
- one 43-byte executable COMDAT with flags `0x60101020` and no owned runtime
  data section.

`section_infos_equal` is true for target versus candidate and target versus
the archive member. The archive member's additional compiler, debug, and
build-number sections are removable metadata, not runtime source ownership.

Current-campaign gates passed:

- forced full `halobetacache_build` and `libcmt_build`;
- regenerated `semantic_progress` with zero unit errors;
- `progress`, crediting 43 libcmt code bytes, one function, and one complete
  object;
- semantic audit with zero unit errors and both `objdiff` and `semantic-coff`
  proof for `_wctomb`;
- admission audit with no `wctomb` contradiction, candidate, or revocation;
- 179/179 tool unit tests;
- `git diff --check`.

The clean-commit whole-object regression snapshot/check passed after a forced
rebuild: `_wctomb` is `still_exact`, with no changed-nonexact function, no
newly-exact ambiguity, no warning, and no ownership change.

## House/Berth, readability, and bug audit

The public prototype and wide-character type come from the XDK header. Both
parameters occupy their own lines, all returns are explicit, and the range
test uses the named `UCHAR_MAX` limit rather than a magic byte value. Calling
`_errno()` directly records the January-proven thread-local CRT contract that
the original internal `_MT` build obtained through the `errno` macro; it is
not a codegen-only spelling. `(void)` is correctly present on this no-argument
accessor and inapplicable to `wctomb`, which takes two arguments.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, raw offset, comparator exception,
or codegen-only cast is used. No original January bug was found, so no
bug-preservation comment or disabled corrected form is needed.
