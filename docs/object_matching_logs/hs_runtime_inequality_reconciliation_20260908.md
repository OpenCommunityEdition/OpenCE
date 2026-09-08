# `hs_runtime.obj` inequality reconciliation (2026-09-08)

## Adjudicated result

This packet accepts the exact, idiomatic `expression_get_function_name`
control-flow correction and reconstructs `hs_evaluate_inequality` as useful
ordinary C. It also atomically restores the real function-local
`parameter_types[2]` BSS owner and the four-byte `hs_runtime_globals` layout.

The expression-name helper's `while (TRUE)` plus explicit flag break is
semantically equivalent and independently exact: **one function / 176 padded
bytes**. The inequality evaluator is retained as the best coherent fuzzy
implementation and receives **zero exact credit**.

## House-rule decision

An Opus donor made the 528-byte inequality owner exact by widening both `real`
arguments into named `double` locals and narrowing them immediately back to
`real`. Although this reproduces January's x87 lifetime, no source evidence
supports that redundant round trip and its demonstrated purpose is compiler
steering. Under the no-fake-matching rule it is rejected. Canonical instead
uses direct `real` loads and honestly parks the body as fuzzy.

The evaluator otherwise follows the recovered syntax traversal, type cases,
comparison operators, invariant assertions, result convention, and explicit
return. Its prototype lives in the narrow genuine owner
`hs_library_internal_runtime.h`.

## Atomic BSS correction

```text
4535724  ?parameter_types@?1??hs_evaluate_inequality@@9@9  static, 4 bytes
4535728  _hs_runtime_globals                              static, 4 bytes
```

The old four-byte `reserved` member is removed from `hs_runtime_globals` in the
same change. This preserves the seven established consumers of the runtime
global; their restoration is not counted as a gain.

Final credit is assigned only after regenerated-target, full-build, and stable
no-regression verification.
