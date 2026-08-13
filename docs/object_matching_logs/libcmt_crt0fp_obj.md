# `libcmt/crt0fp.obj` exact-match ledger

## Plain-English behavior

This CRT startup fallback reports runtime error 2 when floating-point support
is required but unavailable. It owns no runtime data.

## January and XDK provenance

January owns one nine-byte external `__fptrap` function with one REL32
relocation to `__amsg_exit`. XDK 3911 `libcmt.lib` contains
`obj\\i386\\crt0fp.obj`; its records identify `..\\crt0fp.c` and Microsoft
VC7, and its runtime function is strictly identical to January. The readable
source is therefore the direct historical error-handler call.

## Strict evidence

- size: 9 bytes
- relocations: one at `+0x3` to `__amsg_exit + 0`
- normalized SHA-256:
  `a4b7f5d5257282f88fb3a4967dddf4fb7e262498eafc1ad3644114be3105051f`
- target-owned runtime data: none

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| E01 | Direct `_amsg_exit(2)` with explicit final `return;` | **STRICT EXACT** on the first fresh XDK 3911 rebuild: 9/9 bytes, 1/1 relocation, identical normalized hash and destination |

## House/Berth audit

- The prototype, explicit `void`, multiline signature, and final return follow
  project style.
- Runtime error 2 is proven by the target instruction and XDK C member, not a
  guessed engine magic number.
- No assembly, `volatile`, undefined behavior, forced inline, byte forcing,
  alignment directive, flag change, or comparator exception is used.
- This deterministic CRT fallback contains no original-game bug to annotate.

The fresh object contains exactly the target's one external function COMDAT
and introduces no runtime `.rdata`, `.data`, or `.bss`. Admission additionally
requires full builds, zero semantic errors, a clean forced-rebuild regression
check, and all 179 tests.
