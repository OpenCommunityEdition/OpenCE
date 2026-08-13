# `libs/libcmt/wcstombs.obj` exact-match ledger

This isolated atomic lane starts from authoritative campaign commit
`378ee498`. Configuration index 816 was `MISSING`; the object was absent from
`config/parked.json`, production history, prior matching logs, active
worktrees, and the other agents' claims.

## Plain-English behavior

`wcstombs` converts a wide-character string to the current locale's multibyte
encoding. In the built-in C locale it copies characters directly after
rejecting values above 255. For other single-byte locales it converts one
bounded block. For multibyte locales it first tries the whole string and, if
the caller's buffer is too small, resumes one wide character at a time without
splitting a multibyte character. Passing a null destination asks only for the
required output length. Conversion failure sets `errno` to `EILSEQ` and
returns `(size_t)-1`.

The private 47-byte helper counts at most a caller-provided number of wide
characters, including the terminator when it is found. January's split symbol
is `_code_001d3099`; CodeView identifies the authentic source function as
Microsoft's file-local `wcsncnt`.

## Primary provenance and January inventory

The exact `obj\i386\wcstombs.obj` member was extracted from the local XDK
3911 `libcmt.lib`. CodeView names
`d:\xbox-finalfre\private\sdktools\crt\crtw32\convert\wcstombs.c` and carries
ordinary `.bf`, `.lf`, and `.ef` records for both functions, proving compiled
C rather than assembly. The corresponding Microsoft 1990-2001 source was
recovered from the pinned Easy-Build-2k NT source mirror at commit
`36fce23389e43918faf4cd88688c64aa2668ff8f`.

Before writing production source, both authenticated archive sections compared
strict-equal to the January split object:

| January symbol | Authentic source name | Bytes | Relocations | Normalized SHA-256 |
|---|---|---:|---:|---|
| `_code_001d3099` | `wcsncnt` | 47 | 0 | `014a8edc67b89e083ade261c140c72e4d1d6bd7f52560656638c9fbf92e7b75b` |
| `_wcstombs` | `wcstombs` | 414 | 15 | `0ed7742a264f1ea143a0ff7e6a926fe2228fdf0eea92dd07c65826b23397423b` |

January owns exactly those two `.text` sections and no data, BSS, or read-only
data. The archive's separate select-any CRT build-number metadata is not owned
by January and is not reproduced.

## Measured source-shape experiments

All production measurements used the inherited XDK 3911 command and unchanged
`/O1 /Gy` flags.

| ID | Readable-C shape | Result |
|---|---|---|
| E01 | Faithful Microsoft control flow and locals, but include the broad XDK umbrella header and assign through public `errno` | The helper was immediately exact. The public routine was 413/414 bytes with 15/15 relocations: public `errno` became a six-byte data relocation to `_errno`, whereas January calls the internal `_errno()` accessor. The umbrella header also emitted three unrelated D3D constant tables, violating object ownership. Rejected. |
| E02 | Preserve the same authentic algorithm, declare only the typed Win32/CRT interfaces actually used, and spell the historical internal operation as `*_errno() = EILSEQ` | Both functions strict exact: 47/47 and 414/414 bytes; 0/0 and 15/15 relocations; both normalized hashes equal; no owned data or extra runtime table remains. Accepted. |

The 15 public relocations match in address, type, destination, and addend:
two references to `__lc_handle + 8`, two to `__mb_cur_max`, four to
`__lc_codepage`, four calls to `WideCharToMultiByte`, and one call each to
`GetLastError`, `_errno`, and `wcslen`.

## Original-behavior and bug audit

No confirmed implementation bug was found. The null-source case is an API
precondition inherited from the C runtime rather than an engine defect; the
original routine does not add a defensive null check, and neither does this
reconstruction. The apparently unusual whole-string-then-character fallback
is intentional: it ensures the function does not copy a partial multibyte
character when the destination is too small. All original return values,
error assignments, terminator-count rules, and buffer-limit behavior are
preserved.

## House, Berth, and readability audit

The final source is ordinary typed C derived from authenticated Microsoft
source. Parameters are one per line, the zero-argument `_errno` and
`GetLastError` declarations explicitly use `void`, and all returns are
explicit. Constants and external interfaces are named; there are no opaque
offsets, raw object/tag access, or invented structure layouts.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
codegen cast, alignment coercion, compiler-flag change, pragma, comparator
exception, or prebuilt-object substitution is used. The narrow interface
declarations avoid the unrelated ownership pollution measured in E01.

## Final validation

- The object was deleted and force-rebuilt with XDK 3911 `CL 13.00.9254.1`
  under the unchanged `/O1 /Gy` command. A post-rebuild hardened comparison
  kept both functions strict exact.
- Whole-object ownership is exact: the rebuilt runtime content is only the
  two expected code COMDATs, with no data, BSS, read-only data, or unexpected
  external definition. The compiler directive/debug sections are not linked
  runtime ownership.
- Full `halobetacache_build` and `libcmt_build`: successful.
- `progress`: successful; `wcstombs.obj` is one of 64/212 complete libcmt
  objects and contributes exactly 461 code bytes and two functions.
- `semantic_progress` and an explicit semantic audit: zero unit errors; both
  functions accepted by strict COFF evidence.
- Admission audit: no unmarked completion candidate or newly contradicted
  label. The sole contradicted `shell_xbox` entry is the pre-existing global
  audit finding and is unrelated to this unit.
- Tooling unit tests: 179/179 passed.
- `git diff --check`: clean.
- Clean per-unit regression snapshot/check: force-rebuilt the committed unit,
  retained `_code_001d3099` and `_wcstombs` as `still_exact`, and reported no
  changed non-exact function, failure, or warning.
