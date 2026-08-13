# `purevirt.obj` exact-match log

## Plain-English behavior

This CRT handler reports runtime error 25 when a pure virtual method is
called. It owns no data.

## Provenance and verification

- January target: one nine-byte cdecl function, with one REL32 relocation to
  `__amsg_exit`.
- Compiler: XDK 3911 CL 13.00.9254.1 with the libcmt project's unchanged
  `/O1 /Gy` flags.
- The readable call plus explicit final `return;` must reproduce
  `push 25; call _amsg_exit; pop ecx; ret`, including relocation identity.
- No assembly, `volatile`, forced inlining, undefined behavior, byte patch,
  or compiler-flag change is used.

## Strict measurements

The prior source was used only as a lead and rebuilt independently from
campaign tip `9ad8f126` with XDK 3911. The fresh result is strict exact:

- code: 9/9 bytes
- relocations: 1/1
- normalized SHA-256, both sides:
  `f10173af9b0783d23271b12acf8a48b16dcffe6e64befbb3de29911941eb5d23`
- relocation: `+0x3`, `IMAGE_REL_I386_REL32`, `__amsg_exit + 0`
- `section_infos_equal`: `true`

The target and candidate each expose one external `__purecall` function at
offset zero with COFF function type `0x20` and storage class `2`. The target
owns no `.rdata`, `.data`, or `.bss`; the candidate adds none. Compiler debug
and directive sections are non-runtime metadata.

## House/Berth audit

- The implementation names the runtime operation directly instead of hiding
  it behind a wrapper or magic address.
- The explicit `void` prototype and final `return;` follow project style and
  reproduce the target's one epilogue.
- Error number 25 is the documented CRT pure-virtual-call runtime error, not
  an invented layout or codegen constant.
- No assembly, `volatile`, undefined behavior, forced inlining, byte forcing,
  alignment directive, flag change, or comparator/semantic exception exists.
- There is no original-game bug to annotate in this one-call CRT handler.

Before admission, the full Halo and libcmt builds, semantic/progress audits,
clean per-unit regression snapshot/check, and all 179 tool tests must pass.
