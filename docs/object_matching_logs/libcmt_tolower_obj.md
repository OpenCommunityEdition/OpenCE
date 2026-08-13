# `libcmt/tolower.obj` exact-match ledger

## Plain-English behavior

The unchecked helper converts an already-known uppercase ASCII value by adding
the distance between `A` and `a`. The public function first asks the active CRT
character classifier whether its input is uppercase, then performs the same
conversion only when appropriate.

## January and XDK provenance

January owns two external function COMDATs and no runtime data. XDK 3911
`libcmt.lib` contains `obj\\i386\\tolower.obj`; its COFF records identify
`..\\tolower.c`, the original Microsoft path, and VC7. Both XDK functions are
strict-equal to January, proving the historic C provenance, branch topology,
globals, and classifier call before source reconstruction.

## Strict target evidence

- `__tolower`: 8 bytes, zero relocations, normalized SHA-256
  `896bc6d5fe67391885caffc40607e53c66493d01b693e3c3eb07b286ea451710`
- `_tolower`: 49 bytes, relocations `+0x02 -> ___mb_cur_max`,
  `+0x12 -> __isctype`, and `+0x1B -> __pctype`; normalized SHA-256
  `c75ae3b38d77c3cfdf44fd21405d8150d36d75e975e2bd6e7b1b4b1a6c078db9`
- target-owned `.rdata`, `.data`, and `.bss`: none

## Experiment record

| ID | Source shape | Result |
| --- | --- | --- |
| T01 | Named predicate followed by conditionally mutating the parameter | Helper exact; public function 54/49 with correct 3 relocs, but VC7 updated the stack argument instead of retaining it in ESI; rejected |
| T02 | Same predicate branches returned through a final conditional expression | **STRICT EXACT**: both functions, 57/57 bytes, 3/3 relocations, hashes and destinations identical |

## House/Berth audit

- The classification bit and ASCII case distance are named constants rather
  than unexplained numeric literals.
- Both routines use readable C and descriptive locals; the public function has
  one final return.
- No assembly, `volatile`, undefined behavior, forced inline, byte forcing,
  alignment directive, flag change, or comparator exception is used.
- No original bug was found in these deterministic character conversions.

The rebuilt object owns exactly the target's two external function COMDATs and
introduces no runtime `.rdata`, `.data`, or `.bss`. Admission requires strict
equality for both functions and ownership, full Halo
and CRT builds, zero semantic errors, a clean forced-rebuild regression check,
all 179 tests, and a clean worktree.
