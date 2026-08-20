# HS large-object reconstruction checkpoint

## Scope and disposition

This is an honest partial checkpoint for `source/hs/hs.obj` and the two
independently recovered functions in `source/hs/hs_runtime.obj`. Both units
remain `NonMatching`; this change grants no object or data credit and makes no
configuration, parked-function, symbol, or frozen-object change.

The source basis is local Claude branch commit
`fae4cc9a82b3ea906bddb30673a8d1b19f072f2d`, itself built from the existing
campaign history plus the later ordinary-C cleanup through `8a9509f1`. Only
`source/hs/hs.c` and `source/hs/hs_runtime.c` were transplanted into a fresh
worktree based on canonical `a0bad121`; branch history and unrelated files were
not imported. The source contains no assembly, `volatile`, `__forceinline`,
optimization pragma, compiler barrier, byte forcing, or omitted return.

## Strict result

Fresh XDK 3911 `/O1 /Gy` reconstruction gives:

- `hs.obj`: 442/448 functions strict-exact, 28,656 padded exact code bytes and
  1,417 exact relocations. The canonical base was 435/448, 27,504 bytes and
  1,359 relocations, so this checkpoint adds seven strict functions, 1,152
  padded bytes, and 58 relocations without losing an exact function.
- `hs_runtime.obj`: 24/66 functions strict-exact, 1,776 padded exact code bytes
  and 117 exact relocations. The canonical base was 22/66, 1,504 bytes and 106
  relocations, so this checkpoint adds two strict functions, 272 padded bytes,
  and 11 relocations without losing an exact function.
- Combined gain: nine strict functions, 1,424 padded code bytes, and 69 exact
  relocations.

The seven newly exact `hs.obj` functions are:

- `_code_000b2f00`
- `_code_000b3450`
- `_code_000b3e80`
- `_hs_find_script_by_name`
- `_hs_find_tag_reference_by_index`
- `_hs_scenario_merge`
- `_hs_scenario_postprocess`

The two newly exact `hs_runtime.obj` functions are `_code_000ba310` and
`_hs_can_cast`.

Non-code ownership is not complete and is not claimed. The hardened snapshot
finds only 30/911 target non-code sections strict in `hs.obj` and 25/61 strict
in `hs_runtime.obj`; both units therefore remain closed to object admission
regardless of their code counts.

## Remaining `hs.obj` code

All six target functions are emitted. Five have exact padded size and
relocation count and differ only in normalized instruction scheduling:

- `_code_000b2110`: 80 bytes
- `_code_000b2160`: 80 bytes
- `_code_000b2a40`: 96 bytes
- `_code_000b33d0`: 128 bytes
- `_code_000b3f50`: 96 bytes

`_hs_compile_and_evaluate` is the only structural residual: target 592 bytes,
candidate 608 bytes, with all 40 relocations present. January reserves EBX as
the shared zero carrier and homes the selected source pointer through the dead
incoming argument slot; the current compiler keeps that pointer in EBX.

The final bounded review rejected the following do-not-repeat shapes:

- HCEA's `is_blank`/`check_blank` scan grows the full function to 624 bytes and
  41 relocations by emitting a second static `isspace` call site.
- Direct parameter reuse, a function-scope initialized source alias, and a
  function-scope uninitialized source assigned at the existing point all stay
  608 bytes and do not change the EBX source lifetime.
- Direct grouped-BSS postfix increment for `_code_000b33d0` emits a memory
  increment and regresses the tail. A local-short postfix and a typed short
  struct view normalize byte-for-byte to the existing `mov eax`/`inc eax`
  residual. The flat-static family can produce `mov ax`/`inc ax`, but 720
  layout permutations failed to preserve January's BSS offsets.
- The three FPU wrappers already exhausted argument binding, declaration,
  ordering, scope, cast, pointer, struct, and union variants. The retained
  ordinary `double` locals reproduce January's x87 push path without the
  forbidden `volatile` stand-in.
- `_code_000b3f50` requires its explicit readable `(real)` conversion; the
  implicit form grows to 112 bytes and adds two relocations.

## `hs_runtime.obj` remainder

Seven emitted functions remain nonexact (`_code_000b9a10`, `_code_000ba090`,
`_code_000ba290`, `_code_000ba2c0`, `_code_000ba390`, `_hs_evaluate_wake`, and
`_hs_wake_by_name`) and 35 target functions remain unwritten. This checkpoint
does not imply that `hs_runtime.obj` is near admission.

## Validation

- Forced clean XDK rebuild of both objects: passed; the strict counts above
  reproduced from the clean artifacts.
- Full `halobetacache_build` and `libcmt_build`: passed across 567 build edges.
- `semantic_progress`/`progress`: 470 units, 3,611 functions evaluated, 3,529
  accepted exact functions, and zero unit errors. The partial checkpoint adds
  code-function evidence but intentionally leaves both objects incomplete.
- Admission audit: zero candidates and zero revocations; only the pre-existing
  unrelated `shell_xbox` contradiction remains.
- Parked-function validation: zero stale and zero invalid entries.
- Full tooling suite: 179/179 tests passed.
- Clean-commit regression snapshot, forced deletion/rebuild of both objects,
  and check: `ok: true`, no changed nonexact functions, no newly exact warnings,
  no ownership warnings, and no failures. The manifest freezes all accepted
  functions plus complete non-code and symbol ownership for both units.
- House/Berth scan: passed; raw `tag_get` occurs only inside the two typed local
  retrieval macros.
- Diff and scope audit: exactly the two HS source files plus this new
  Codex-owned ledger; no configuration, parked manifest, Claude-authored
  Markdown, or frozen-object path changed.
