# `libs/libcmt/fpinit.obj` exact-match ledger

This isolated atomic lane started from campaign commit `a955302f`. Before any
production edit, `fpinit.obj` was confirmed `MISSING` at config index 652,
absent from `config/parked.json`, and free of topic branches, worktrees, logs,
or active peer ownership.

## Plain-English behavior

This CRT object owns the small amount of state used to bring floating-point
support online. It installs six number-to-text conversion callbacks, remembers
whether fast floating-point behavior was selected, initializes the x87
precision mode, and clears any pending x87 exception flags. It also exports
the startup and per-thread initialization function pointers used by CRT
startup.

## Primary provenance and January inventory

The authentic XDK 3911 archive member is `obj\i386\fpinit.obj`. Its CodeView
records identify Microsoft source
`d:\xbox-finalfre\private\sdktools\crt\fpw32\conv\fpinit.c`, compiler ID
`0x001c2426`, and ordinary C function records. The archive member and January
target are strict-identical before any reconstruction:

- `__fpclear`: 1 byte, 0 relocations, SHA-256
  `ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e`;
- `_cfltcvt_init`: 56 bytes, 11 relocations, SHA-256
  `9ab8c3c9039d812d6fc32e87746069e2f9a711337d6a16a1f3ceecffe44b3950`;
- `__setfflag`: 16 bytes, 2 relocations, SHA-256
  `81993fe55ca098be8dbed54156c0e0a6607bf9d48a9259b90108bcccda9c7807`;
- `_fpmath`: 13 bytes, 2 relocations, SHA-256
  `67b6ae4d342f34995e3c51ed912ed87f9bf46284446bafe4b84327ea6cda0255`.

The object owns 20 bytes of initialized writable `.data`: `_fltused` is
`0x9875`, `_ldused` is `0x9873`, and `_FPinit`, `_FPmtinit`, and `_FPmtterm`
relocate respectively to `_fpmath`, `_fpclear`, and `_fpclear`. It also owns
eight zero bytes of writable `.bss`, split into `__fastflag` and `_adjust_fdiv`.
The target, archive, and readable candidate all agree on every byte, symbol
offset/class, section flag, and data relocation.

## Measured source-shape experiments

| ID | One changed factor | Strict measurement | Result |
|---|---|---|---|
| E01 | Direct typed C for all globals and four functions; `_fpmath` calls `_cfltcvt_init`, `_setdefaultprecision`, and public `_clearfp` | Three functions exact; all 28 runtime data bytes and their three relocations exact. `_fpmath` is 15/13 bytes and 3/2 relocations because `_clearfp` remains an external call rather than January's two-byte `fnclex` | Rejected and restored |
| E02 | Add `#pragma intrinsic(_clearfp)` without otherwise changing E01 | Compiler warning C4163 says `_clearfp` is not available as an intrinsic; object is byte-identical to E01 | Rejected and restored; the pragma is also prohibited by the lane rules |

The target's final two bytes are `DB E2`, decoded as `fnclex`. The exact XDK
member proves this is historical Microsoft output, but the campaign compiler
does not expose `_clearfp` as an intrinsic under the unchanged `/O1 /Gy`
command. A direct `_clearfp()` call necessarily creates an extra REL32
relocation. Emitting `fnclex` directly would require inline assembly, a raw-byte
directive, or a compiler/flag exception. All three are outside the project's
readable-C standard, so no further spelling matrix is justified.

## House, Berth, readability, and bug audit

E01 used explicit callback types, named globals, vertical signatures, natural
assignments, and final returns. The values and callback ordering came from the
January object and authentic XDK member rather than guesses. No original bug
was found: clearing pending floating-point exceptions during initialization is
intentional CRT behavior.

No assembly, `volatile`, forced inline/noinline directive, undefined behavior,
object-byte patch, flag change, comparator exception, symbolic-relocation
waiver, or codegen-only cast is admitted. Although three functions and all
runtime data are independently reconstructible in readable C, the atomic
object rule forbids retaining partial production source or marking the object
`Matching` while `_fpmath` remains outside the allowed source language.

## Disposition and reopen criterion

Production source and configuration were restored. This is a documentation-
only park, not matching credit. Reopen only if one of the following appears:

1. authenticated Microsoft/XDK source demonstrates a readable non-assembly C
   spelling that emits `fnclex` under the existing command; or
2. project policy explicitly admits the authentic two-byte instruction as a
   separately reviewed vendored-runtime implementation class.

Do not retry `_clearfp`, `_clear87` (the same header macro), or
`#pragma intrinsic(_clearfp)` under the current compiler and flags.
