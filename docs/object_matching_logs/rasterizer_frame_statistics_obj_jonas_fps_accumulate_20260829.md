# `rasterizer_frame_statistics.obj` FPS accumulation recovery (2026-08-29)

## Scope and result

This bounded wave starts from canonical commit
`a35ebf2050364dcfd2d832b1222c32e66d00bed1` and recovers
`_rasterizer_fps_accumulate` in ordinary typed C:

| Function | Target/base padded bytes | Target/base relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `_rasterizer_fps_accumulate` | 64/64 | 8/8 | `24eb522b8bfbf5f27a6863e9eb2eb79f2b7581b721049566531b4ca7adff547c` | **strict exact on first compile** |

The object advances from 4/10 to 5/10 strict-exact functions, adding 64
padded code bytes. The retained-only gate reports 5 exact, 0 residual, and 5
unwritten functions. No object-completion, target-owned data, semantic
exception, or parked credit is claimed.

## Evidence and typed layout

The complete January body and relocation oracle prove three source-level
operations:

1. enable `rasterizer_debug_options.fps_accumulation` at offset zero;
2. zero-extend `system_milliseconds()` into the 64-bit BSS value at `+0x748`;
3. copy the 64-bit rasterizer-owned counter at `_rasterizer_globals+0x18`
   into the BSS value at `+0x750`.

The authenticated HCEA debug-options, rasterizer-globals, frame-statistics,
and draw sources independently corroborate the field meanings and 64-bit
types. HCEA's later-build `rasterizer_fps_accumulate` is an empty retail stub,
so it is negative version evidence rather than a body donor; January COFF and
disassembly remain authoritative.

The existing `rasterizer_globals_definition` already reserved the bytes from
`+0x04` through `+0x27`. This wave names the proven 64-bit counter at `+0x18`
and splits the surrounding reserved arrays without changing structure size or
any previously named offset. A compile-time assertion fixes the new field at
`+0x18`. The positional zero initializer in `rasterizer.c` gains corresponding
zero entries, preserving the complete owned data image exactly.

The frame-statistics translation-unit-local BSS prefix now names the existing
64-bit fields at `+0x748` and `+0x750`, each with a compile-time `offsetof`
assertion. It remains an `extern` declaration-only view; the target-owned
0x860-byte BSS is not defined or rehomed. A one-byte declaration-only debug
options prefix names the offset-zero enable flag.

All object-specific Claude documentation and tooling evidence was consumed
through the complete prior lifecycle ledger
`rasterizer_frame_statistics_obj_jonas_lifecycle_first_shot_20260826.md`,
which records the relevant Claude/HCEA repositories, commits, documentation,
tool identities, and negative source census. The current HCEA source/header
cache was then read directly for this function's fields. No Claude branch
contains a completed January accumulator body to transplant.

## Rejected private comparator

The unwritten 32-byte `_code_0016e5c0` was screened before the accumulator.
January proves an unsigned 16-bit `first > second` predicate with two explicit
epilogues. Three ordinary spellings were compiled:

- direct boolean return produced a 32-byte branchless `sbb`/`neg` residual,
  hash `6200b0af097899c88f70f6c720c361bc5814d0d147d7d4f652438b33023df910`;
- an explicit positive guard produced a 16-byte `seta` residual;
- the literal inverse `first <= second` guard produced the same 16-byte
  residual, hash
  `824636ceaea1b9fd752c3bd2d2e627269d300d2dbc33b596caea74e2aeaa9d0c`.

All three were removed. Reopen only with caller/source provenance or a new
lawful control-flow fact; do not retain the residual or add an optimizer
barrier.

## Cross-TU regression proof

Because the new field name lives in a shared header, the complete dependent
object graph was rebuilt. `source/rasterizer/rasterizer.obj` remains 131/131
strict exact, and its owned `.bss`, `.data`, and `.rdata` remain 612/612 bytes
exact. The existing `_code_000f0bb0` main frame-snapshot function remains
48/48 bytes with all six relocations exact. The strict campaign board changes
by exactly the expected one function and 64 bytes, with no lost exact owner.

## Policy and verification

The retained body has `void` on its own line inside the no-argument parameter
list and an explicit terminal return. The wave introduces no assembly,
`volatile`, `register`, pragma, intrinsic, optimizer barrier, attribute,
pointer/union pun, undefined behavior, raw address/offset access, object-byte
access, synthetic anchor, byte forcing, compiler-flag change, comparator
exception, or object patch.

- January frame-statistics object SHA-256:
  `48452628b2525d642bacbb87d89e273e710734f1ca73a5be722e288b4aa4b795`.
- Final rebuilt frame-statistics object SHA-256:
  `62f18175fea209a585c2e29782b7627e6bb384acdf3b45639dda2e47c12bc7c3`.
- Final rebuilt rasterizer owner object SHA-256:
  `8c8b58c12f49f5e6e4c143f8a3351026b35e6b80ddf041ba1ffce33b59ee028a`.
- Retained Git blobs: `rasterizer.c`
  `5f2b8238d9a1e3fb043d617ee6708575f0a09a54`, `rasterizer.h`
  `e1105436be89d44eaab8c80939a3c9f1a44466a5`, and
  `rasterizer_frame_statistics.c`
  `251af141ccb75ec6fb0102722d39b417e6992941`.
- Full Halo/libcmt build and semantic scan: 470 units, 4,789 functions,
  4,665 semantic exact, 4,675 accepted exact, and zero unit errors.
- Tool tests: 205 passed; pytest's optional cache warning is sandbox-only.
- Object-admission audit: 0 candidates, 0 contradicted, 0 revoked.
- Parked ledger: 13 active, 0 stale, 0 invalid.
- Strict campaign board: 275/619 objects, 637,143/1,922,669 padded code
  bytes, and 4,646/8,246 functions.
- `git diff --check` passes.

No configuration, semantic ledger, parked entry, tool, Units, Vehicles,
Matrix Math, protected source, target object, or unrelated translation unit
was modified.
