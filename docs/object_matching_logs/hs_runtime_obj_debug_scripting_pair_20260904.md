# `hs_runtime.obj` debug-scripting pair (2026-09-04)

## Result

This isolated lane starts at canonical commit
`fc057d673a3f0e3a8a782bb4ec8ae8ea8824f5ea` and reconstructs the two
previously unwritten debug-scripting routines in ordinary typed C:

| Function | Result | Meaningful | Padded | Relocations | Objdiff |
| --- | --- | ---: | ---: | ---: | ---: |
| `_render_debug_scripting` | strict exact | 376 | 384 | 27 | 100% |
| `_expression_get_function_name` | fuzzy parked | 169 | 176 | 12 | 99.52381% |

The focused object gate advances `source/hs/hs_runtime.obj` from **55 exact /
3 residual / 8 unwritten** to **56 exact / 4 residual / 6 unwritten**.  The
rename-stable whole-tree comparison reports exactly one newly exact function,
384 padded bytes gained, and zero regressions across all 8,245 target
functions.  Semantic progress rises by one function and 376 meaningful code
bytes, from 882,677 to **883,053 / 2,198,102** code bytes and from 6,069 to
**6,070 / 11,060** credited functions.  Matched data remains 2,018,310 bytes
and linked objects remain 391/833.

## Evidence and reconstruction

January target disassembly, COFF layout, ABI, strings, and relocation topology
are authoritative.  HaloCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently corroborates the
named helper and overlay behavior in:

- `src/blam/render/expression_get_function_name.c`
- `src/blam/render/render_debug_scripting.c`
- `src/data/debug_scripting.c`

The retained public overlay uses the real `debug_scripting` global, walks the
live HS thread data array, formats each sleeping thread's name and remaining
sleep time, appends the active function/script name when a non-root frame is
present, bounds the visible string, installs the two target tab stops, draws
the result, and restores the tab-stop count.  Its 384-byte section, all 27
relocations and their offsets, and normalized SHA-256 are identical to
January:
`ded2bc40baa703bfd91c98ba199fbb35e37e721fbd25fa1b769e57ce839730d2`.

The private helper is naturally file-static and uses its authenticated
original-style name.  It walks outward through the current stack frame until
it can return a function name, the exact `(end of script)` sentinel, or a
script name through `TAG_BLOCK_GET_ELEMENT`.  Target and candidate have the
same 176-byte padded envelope, the same 70 decoded instructions, and the same
12 relocation identities at the same offsets.  The only differences are two
independent ModRM choices: January uses ECX for the final sign-extended script
index where VC7 selects EDX, and EDX for the zero-extended function index where
VC7 selects ECX.

Bounded ordinary-C probes covered direct versus staged next-expression
assignment, declaration placement/order, short/word scalar variants, and the
later-source duplicated-return/break shape.  Staging the next expression and
assigning it before the next syntax lookup recovers January's complete core;
the other forms either converge on the retained two-register fixed point or
disturb more of the function.  The fail-closed blocker classifier returns
`UNKNOWN`, so the helper is honestly recorded as `unclassified` rather than
claiming a proven compiler recipe.  Its target and candidate normalized hashes
are respectively
`50c82a2814e4ab854e325a08464b0ae9985f2c032445bb2c9e16bb26754ea821`
and
`bf099693458aa0c86c0297f4536c0d37f8d73bda1332a1448d4ee921f4ba51cf`.

## Ownership and house-rule audit

`render_debug_scripting` is declared in `source/hs/hs.h`, the owning HS
subsystem header.  The private helper remains in `hs_runtime.c`; no caller-local
prototype or invented public linkage is used.  The implementation uses project
`boolean`, `long`, and HS structure types, the established typed thread/script
access paths, named constants/macros, one parameter per line, and an explicit
terminal `return;` for the public void function.

No address-derived private/global name, `code_<address>` or `bss_<address>`
identifier, raw field offset, raw datum cast, direct untyped `tag_get`, manual
flag arithmetic, forced inline/noinline request, register/volatile forcing,
barrier, pragma, assembly, fake dependency, representation pun, undefined
behavior, or nonsensical exact-only path is retained.  The rebuilt object
contains no `point_from_line3d` symbol or COMDAT, preserving the January inline
schedule.

## Verification

- Full `ninja`: pass after the final source, owner-header, and park-manifest
  changes; semantic unit errors are zero.
- Focused HS runtime gate: 56 exact / 4 residual / 6 unwritten.
- Fixed Units sentinel: 189 exact / 0 residual / 0 unwritten.
- Rename-stable sweep: +1 function / +384 padded bytes / zero regressions.
- Semantic report: 473 units, 6,498 functions evaluated, 6,143 accepted exact
  proofs, and zero unit errors.
- Parked-function audit: 225 active / 0 stale / 0 invalid.
- Object-admission audit: zero candidates, contradictions, or revocations;
  the two inherited reviewed rejections remain unchanged.
- Focused fake-match scan: zero review leads.
- Tooling suite: 261/261 tests passed.
- `git diff --check`: clean apart from Git's LF-to-CRLF informational warning.
- January `hs_runtime.obj` SHA-256:
  `6D35FD03F7BC250564A02FB86F30D65F3AC936A70BC42947591DAFBC2859375D`.
- Rebuilt `hs_runtime.obj` SHA-256:
  `509CA18C2623275C025304FB688650B449F2537EC4E3E215B761DA70D34028CF`.
- Rebuilt source SHA-256:
  `E57AE74E3DB0356399EFE81F741E85018609FDBC82DFE0A247953F402AA46073`.

Stable snapshot:

- `scratch/after-hs-debug-pair-20260904.json`

The comparison baseline is canonical's
`scratch/post-network-server-replay-20260904.json`, captured at the same
starting commit.
