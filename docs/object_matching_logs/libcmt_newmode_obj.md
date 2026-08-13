# `libcmt/_newmode.obj` exact-match ledger

## Plain-English behavior

This object owns the CRT's default allocation-failure mode. A zero value tells
the allocator not to invoke a new-handler unless a caller changes the mode.
There is no executable function in this translation unit.

## January and XDK provenance

The January target owns one four-byte external BSS symbol, `__newmode`, at
offset zero. It has no relocations and no code, initialized data, or read-only
data. The XDK 3911 `libcmt.lib` archive contains
`obj\\i386\\_newmode.obj`; its embedded compiler records identify the source
as `..\\_newmode.c` and Microsoft VC7, not assembler. The natural C
declaration is therefore the original semantic form, not a reconstruction by
instruction forcing.

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| E01 | External `int _newmode = 0;` | Strict exact on first build |

The fresh XDK 3911 build proves:

- logical BSS size: 4/4 bytes
- relocations: 0/0
- normalized SHA-256, both sides:
  `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`
- section flags, both sides: `0xC0300080`
- owner: external `__newmode`, offset zero, COFF type zero, storage class `2`
- `section_infos_equal`: `true`

The target owns no other runtime section and the candidate adds none. The
XDK archive member has the same logical BSS owner; its compiler-only debug and
build-number sections are not part of January's linked runtime object.

Admission additionally requires a full Halo/libcmt build, zero semantic unit
errors, a clean forced-rebuild regression check, and all 179 project tests.

## House/Berth audit

- The named external integer expresses the CRT state directly.
- The explicit zero initializer is intentional: it must produce owned BSS,
  rather than an undefined/common symbol that silently removes target data.
- No assembly, `volatile`, undefined behavior, forced inline, byte forcing,
  alignment override, magic offset, compiler flag change, or comparator
  exception is used.
- There is no original-game bug in a default zero-valued CRT mode variable.
