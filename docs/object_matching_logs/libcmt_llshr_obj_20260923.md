# `libs/libcmt/llshr.obj` — original-assembly CRT helper

This is the Microsoft x86 signed 64-bit arithmetic right-shift helper. Its
calling convention is internal to the compiler: `EDX:EAX` carries the input
and result, while `CL` carries the shift count. A C function prototype would
misrepresent that ABI. Counts below 32 shift both halves, counts 32–63 shift
the old high half into `EAX` and sign-extend `EDX`, and counts at least 64
produce all sign bits.

The authentic XDK 3911 `libcmt.lib` member has SHA-256
`39e1b5afbfcda56c1ffd9496a7be178dce7691d97079bb8b542904ef726b508a`.
Its `.file` record says `..\i386\llshr.asm`, and CodeView identifies
Microsoft Macro Assembler 6.15.8803. January and this member own exactly one
runtime `.text` section: 33 bytes, no relocations, ordinary external
`__allshr`, 16-byte alignment, flags `0x60500020`, normalized SHA-256
`f028a2bb21a2463601e9046f5105991b09f90af85bae56b363c014dafc609ea6`.
Neither owns runtime data.

The initial test wrote the same instructions inside a naked C helper. That
gave the correct 33 bytes but the wrong `.text` alignment/flags (`0x60101020`),
even with a function `__declspec(align(16))`. VC7 also forbids a C `return;`
inside a naked function. Consequently that C approximation was not landed.

A genuine MASM source now uses an ordinary `.text SEGMENT ALIGN(16) PUBLIC
'CODE'`. Modern installed `ml.exe` assembles the readable instructions to a
section for which the hardened `section_infos_equal` comparison with January
is **true**: same 33 code bytes, zero relocations, target flags, symbol
storage, and owner offset. Empty `.text$mn` and `.data` plus `.debug$S` are
assembler bookkeeping, not runtime ownership. No copied `.obj`, hex byte
directive, linker flag override, or post-processing is used.

The build generator now routes `.asm` files through MASM only; C units retain
the existing VC7 rule. Plain `python configure.py` finds `ml.exe` on PATH,
through `VCToolsInstallDir`, or in an installed Visual Studio tree; on this
machine it finds the VS18 Hostx64/x86 assembler. On other installations,
`python configure.py configure --ml "C:\\path with spaces\\ml.exe"` explicitly
selects the x86 assembler. The generator quotes this path when Ninja replays
`configure.py`. A targeted Ninja build of `build/base/libs/libcmt/llshr.obj`
and focused build-rule tests passed.

Accounting must remain explicit: this was transcribed from an authenticated
original-assembly object, so its exact result is evidence of reproducibility,
not a recovered C implementation. Treat it like the campaign's existing
`asm-implemented` provenance parks when computing meaningful reconstructed
code credit. Mark whole-object `Matching` only after the full build, tests,
and ownership sweep pass; do not launder 33 bytes into the C reconstruction
ledger merely because the object comparator returns exact.
