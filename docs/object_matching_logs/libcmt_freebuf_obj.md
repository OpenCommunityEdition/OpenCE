# `libs/libcmt/_freebuf.obj` exact-match ledger

This is an atomic reconstruction lane created from authoritative campaign
commit `91ee86cd`. Before editing, the unit was confirmed `MISSING`, absent
from `config/parked.json`, absent from every active worktree, and unclaimed by
the other active agents.

## Plain-English behavior

`_freebuf` releases a C stream's buffer only when the stream is open and the
runtime itself owns that buffer. It calls `free` on the stream's typed
`FILE._base` pointer, clears both the public `_IOMYBUF` ownership flag and the
CRT-private flag recording a `setvbuf` buffer, then resets `_ptr`, `_base`, and
`_cnt`. Streams that are closed or do not own their buffer are left unchanged.

## Primary provenance

The primary source of truth is member `_freebuf.obj` extracted directly from
the XDK 3911 `libcmt.lib`, not a later CRT or community reconstruction. Its
runtime `.text` is strictly identical to the January target before any source
work:

- function `__freebuf`, 43 bytes;
- one `REL32` relocation at function offset `+0x14`, targeting `_free + 0`;
- normalized SHA-256
  `fe3ca156c86d0bfc8465c0ae848bfcbc00c3a00ab033378839fc7eaf212a583f`;
- no owned runtime `.data`, `.bss`, or `.rdata`.

The archive member identifies its original source as
`d:\xbox-finalfre\private\sdktools\crt\crtw32\stdio\_freebuf.c`. Its ordinary
C function and line records (`.bf`, `.lf`, and `.ef`) prove this is compiled C,
not handwritten or vendored assembly. The XDK's public `stdio.h` proves the
complete `FILE` layout and the `_IOREAD`, `_IOWRT`, `_IORW`, and `_IOMYBUF`
values. The remaining `_IOSETVBUF` flag is private to the CRT; the
January/XDK instruction clearing `0x0408`, together with the historical CRT
internal-header name, proves its value `0x0400` and its set-buffer role.

## Source-shape experiment record

| ID | Readable C shape | Measured result |
|---|---|---|
| E01 | Typed `FILE *`; combined open-state test followed by `_IOMYBUF`; free `_base`; clear the two flags; chained null assignment for `_ptr` and `_base`; zero `_cnt` | Correct 43-byte size and relocation, but the C assignment chain wrote `_base` before `_ptr`; January writes `_ptr` first |
| E02 | Replace only the chained null assignment with explicit `_ptr = 0;` followed by `_base = 0;` | **Strict exact.** Target and candidate are 43/43 bytes with the same one relocation and normalized SHA-256; directly reflects January's measured store order |

No assembly, byte patch, opaque structure offset, undefined behavior,
`volatile`, forced inline, optimization pragma, or compiler-flag change is
permitted in this lane.

## Strict verification

XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the final source under the generated
`/O1 /Gy /I"xbox/include"` flags. The January target, candidate, and extracted
XDK archive member agree on every runtime property:

- 43/43/43 code bytes and one function;
- one `REL32` relocation at `+0x14`, resolving to external `_free + 0`;
- normalized SHA-256
  `fe3ca156c86d0bfc8465c0ae848bfcbc00c3a00ab033378839fc7eaf212a583f`;
- external function symbol `__freebuf` at offset zero, type `0x20`, storage
  class 2;
- one 43-byte executable COMDAT with flags `0x60101020` and no owned runtime
  data section.

`section_infos_equal` is true for target versus candidate. The archive
member's additional compiler, debug, and build-number sections are removable
metadata, not runtime source ownership.

Current-campaign gates passed:

- forced full `halobetacache_build` and `libcmt_build`;
- regenerated `semantic_progress` with zero unit errors;
- `progress`, crediting 43 libcmt code bytes, one function, and one complete
  object;
- semantic audit with zero unit errors and both `objdiff` and `semantic-coff`
  proof for `__freebuf`;
- admission audit with no `_freebuf` contradiction, candidate, or revocation;
- 179/179 tool unit tests;
- `git diff --check`.

The clean-commit whole-object regression snapshot/check passed after a forced
rebuild: `__freebuf` is `still_exact`, with no changed-nonexact function, no
newly-exact ambiguity, no warning, and no ownership change.

## House/Berth, readability, and bug audit

The implementation uses the XDK's real `FILE` type, named fields, public
stream flags, and the historically named private `_IOSETVBUF` flag. The
parameter is on its own line and has an ordinary typed declaration. `(void)`
is inapplicable because the function takes a parameter. There are no Halo
tag, datum, or object accesses requiring project-specific accessors.

No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
alignment directive, optimization pragma, raw offset, comparator exception,
or codegen-only cast is used. No original January bug was found, so no
bug-preservation comment or disabled corrected form is needed.
