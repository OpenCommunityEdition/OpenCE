# `hs_runtime.obj` Opus reconciliation (2026-09-04)

## Provenance and scope

- Canonical base: `f928db6dabe1b1e377ab5ef10a13850ea5c7a237`.
- Frozen donor: `opus/small-families-30k-20260902` at
  `eeedd72e5ba0fc1761519f68cad241fe606320e7`; the relevant donor commit is
  `608c66380dca2d841b1e95150d1f88abe41a06d8`.
- Reconciliation was performed in the isolated
  `agent/hs-runtime-opus-reconcile-20260903` worktree. Neither the donor nor
  canonical checkout was modified.
- Donor notes consulted:
  `scratch/reports/hs_runtime.md` and `scratch/reports/naming_hs-runtime.md`
  in the frozen donor worktree.

Only the `hs_runtime` packet and the declarations/configuration needed to
integrate it were admitted. The result is measured against a stable snapshot
of the stated base, not against a moving campaign checkout.

## Result

`tools/campaign/gate.py source/hs/hs_runtime --all` improved from 30 exact,
1 residual, and 35 unwritten functions to **55 exact, 3 honest residuals, and
8 unwritten functions**. The stable whole-tree sweep reports **25 newly exact
functions and 6,688 padded bytes with zero regressions**.

Newly exact functions:

- `_hs_syntax_nth` (48)
- `_hs_thread_delete` (80)
- `_hs_stack_push` (128)
- `_hs_stack_allocate` (336)
- `_hs_cast` (192)
- `_hs_runtime_recompile_error` (64)
- `_hs_return` (288)
- `_hs_global_evaluate` (64)
- `_hs_evaluate` (368)
- `_hs_script_evaluate` (96)
- `_hs_arguments_evaluate` (448)
- `_hs_macro_function_evaluate` (48)
- `_hs_evaluate_begin` (208)
- `_hs_evaluate_if` (400)
- `_hs_evaluate_logical` (304)
- `_hs_evaluate_arithmetic` (384)
- `_hs_evaluate_equality` (224)
- `_hs_evaluate_sleep` (448)
- `_hs_evaluate_sleep_until` (512)
- `_hs_evaluate_inspect` (256)
- `_hs_evaluate_object_cast_up` (288)
- `_hs_evaluate_debug_string` (384)
- `_hs_thread_main` (752)
- `_hs_runtime_update` (176)
- `_hs_runtime_evaluate` (192)

The three residuals are explicitly parked rather than source-forced:

- `_hs_inspect_enum`: 144 bytes, 11 relocations, identical bytes; only a
  linker symbol-alias identity differs for the enum table.
- `_hs_evaluate_begin_random`: 528 bytes, 30 relocations, 99.89011%; four
  ModRM register-choice bytes differ.
- `_hs_global_reconcile_read`: 1,408 bytes, 86 relocations, 92.30703%; the
  remaining difference is compiler instruction/prologue scheduling.

The eight functions still unwritten are
`_expression_get_function_name`, `_hs_evaluate_inequality`,
`_hs_evaluate_set`, `_hs_global_reconcile_write`,
`_hs_runtime_initialize_for_new_map`, `_hs_thread_new`,
`_render_debug_scripting`, and `_render_debug_trigger_volumes`.

## Semantic and ownership audit

- Address-derived private/global identifiers in the packet were replaced by
  PDB-backed semantic names. PDB-backed private helpers and file globals are
  file-static in source and marked `"static": true` in `symbols.json` where
  the January object requires it.
- `hs_type_inspectors` is a real static 49-entry table. Its 196-byte owner
  prefix and ten relocations are target-exact. `hs_runtime_globals` is the
  target-exact static eight-byte BSS owner.
- The corrected `e_hs_type` order places sound/effect/damage/looping sound in
  their target order, followed by object types and object-name types. The
  related cast and compile logic now uses the corresponding enum constants
  rather than compensating arithmetic or raw ordinals.
- `hs_macro_function_evaluate` uses the source-authentic `long *` return type.
  Heterogeneous callers explicitly cast that value at the call site; long
  pointer callers remain direct.
- `hs_runtime_initialize_for_new_map` is declared in its owning `hs.h`.
  Duplicate local declarations were removed from `hs.c` and `main.c`.
- The AI scripting callbacks used here are declared in the closest narrow
  owner header, `ai_debug_scripting.h`. Adding them to broad `ai_debug.h`
  measurably changed C2 allocation in unrelated `units.obj`; the narrow
  declaration surface preserves both objects.
- Inspector callbacks take `union hs_conversion_result` by value. This is
  ABI-compatible with the January code and avoids the donor's incompatible
  function-pointer calls. A later HCEX PDB describes a `long` formal, but
  forcing that later signature either loses January target identity or
  reintroduces undefined pointer-call behavior.

### Deliberate linkage exception

The conversion helpers and object-list coercions are PDB file-static and are
marked target-static in `symbols.json`, but remain externally linked in the C
source for now. VC7 dead-strips these otherwise-unused helpers when declared
static until the genuine `typecasting_procedures` matrix is present. The PDB
identifies that matrix itself as global. Although the raw January relocation
slots were decoded during review, their indices do not reconcile safely with
the corrected semantic enum ordering. Inventing a table merely to retain the
helpers would be fake, nonsensical matching, so the authentic matrix and final
source linkage are left as a documented follow-up. Existing exact converter
bodies were not expanded or rewritten around that uncertainty.

## Verification

- Per-object gate: 55 exact / 3 parked residual / 8 unwritten (66 listed).
- Stable whole-tree comparison: +25 functions / +6,688 padded bytes / zero
  regressions.
- Full Ninja build, semantic progress, campaign progress, parked-function
  validation, object-admission audit, fake-match audit, no-point COMDAT check,
  and pytest were run after the final source/linkage/prototype changes. Pytest
  passed 261/261 with a workspace-local temporary directory.
- Campaign progress at admission: 830,681 / 2,198,102 matched code bytes,
  5,816 / 11,060 functions, and 391 / 833 linked objects. Semantic validation
  reported 5,868 semantic exact, 5,885 accepted, 162 hidden exact, and zero
  unit errors. The parked ledger validated 190 entries.

## Canonical integration replay

The isolated commit was subsequently cherry-picked onto canonical commit
`44e897053`, after the intervening render, sound, path, game, networking, and
devices packets. A fresh configure, full Ninja build, and rename-stable sweep
reproduced the same 25-function / 6,688-padded-byte gain with zero regressions
across all 8,245 target functions. The current object remains 55 exact, 3
parked residual, and 8 unwritten functions.

The rebased progress report records 843,442 / 2,198,102 matched code bytes,
5,877 / 11,060 matched functions, and 2,001,721 matched data bytes. Relative to
the immediately preceding canonical snapshot, the packet contributes 6,159
meaningful code bytes and eight data bytes. Semantic validation reports 5,929
semantic exact, 5,946 accepted exact, and zero unit errors. The current park
manifest validates at 188 active, 0 stale, and 0 invalid entries; object
admission remains 0 candidates, 0 contradictions, and 0 revocations. The fake
scan remains clean, all rebuilt HS objects emit no `point_from_line3d` symbol,
and the complete tool suite again passes 261/261 tests.

Stable snapshots:

- before: `scratch/after-devices-touch-group-delete-20260904.json` (5,895 exact)
- after: `scratch/after-hs-runtime-opus-integration-20260904.json` (5,920 exact)
