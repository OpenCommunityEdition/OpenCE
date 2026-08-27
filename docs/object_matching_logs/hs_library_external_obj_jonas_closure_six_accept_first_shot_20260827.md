# `hs_library_external.obj` Jonas closure six-accept first shot (2026-08-27)

This ledger records one frozen whole-remainder experiment for the 12 functions
that were still absent after the object-orientation wave. Six functions were
strict on the first candidate object and are retained; six misses were removed
without source-shape tuning or a second candidate attempt.

`hs_library_external.obj` advances from 24/36 to 30/36 accepted functions and
from 2,208 to 3,008 padded exact bytes. Only six functions / 816 padded bytes
remain, so the unit is substantially closer to whole-object closure but remains
`NonMatching`.

## Authority and bounded scope

- Cumulative published base:
  `f95fb1001655ce26a8dd36584e38624c0b652e11`.
- Baseline source blob: `88453837cb8f4566d14b83468c399c2a3fa442f1`.
- Retained source blob before commit: `095202d4dbcf387576fdfc4583d9791db1e5e82a`.
- Retained physical source SHA-256:
  `a68846ac7f5ba2eebaea22ab26f41d65468e02603d6107ca7993e55de9d6ef33`.
- January split object SHA-256:
  `c57df669788e0cbfaa6b6830d2d107ce0edb83d927437099654f4d2747c1e5e5`.
- Compiler: pinned Microsoft VC7/XDK through the unchanged ordinary Ninja
  production edge.

January COFF remains the authority for bytes, padding, relocations, linkage,
constants, strings, and ownership. HCEA supplied readable behavior and source
topology only. The only tracked implementation change is
`source/hs/hs_library_external.c`; this ledger is the only other tracked path.
No shared header, configuration, compiler flag, comparator, admission rule,
completion label, semantic allowlist, parked entry, or tool changes.

Applicable Claude guidance, the current matching methodology, all prior
`hs_library_external.obj` ledgers, the January instructions for all 12 targets,
and the corresponding HCEA donors were screened before mutation. No prior
candidate or residual recipe existed for this remainder.

## Retained first-shot functions

The retained functions and their complete strict measurements are:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_hs_players` | 96 | 8 | `36bb0c435fbd575ca5d39d6c5372f41a4b5a3eaaf5b71201a6d5023f71a4fafa` |
| `_hs_trigger_volume_test_objects` | 176 | 6 | `454055c386f4ccc95c901dc24eaf60b62951685b2a3f3811730382f248ee01b8` |
| `_hs_unit_can_see_object` | 112 | 5 | `f8f2a387ddf748092d8bcdacf69a5e41c43da8541512552fe7b5be60e189d3d1` |
| `_code_000b8c80` | 176 | 6 | `a3020a81f23d843715ea6b3510c40220b24bedd62c9eaea6a6390023e62d857d` |
| `_hs_object_create` | 80 | 6 | `f656776b5cc7ba905104731929e02baffc72c1b4484a37adcb94259ea667feaf` |
| `_hs_trigger_volume_test_objects_all` | 160 | 6 | `2a528c7c74f99032ccc061cc0c07a9e2644b859bd1387af963839300999741bc` |
| **Wave gain** | **800** | **37** | |

For every retained function, the hardened comparator reports identical padded
bytes and equal relocation count, order, address, type, symbolic destination,
addend, and target-owned constant/string identity.

The retained readable behavior is:

- build a script object list containing every player's live unit;
- evaluate any/all trigger-volume membership while updating the original
  `hs_debug_data` bit vector through `BIT_VECTOR_SET_FLAG`;
- test unit visibility against a target unit head or typed object bounding
  sphere using the canonical degree-to-radian macro;
- recursively detect an object that is, contains, is mounted beneath, or is an
  item belonging to a player; and
- create a scenario-named object only when absent, otherwise emit January's
  exact warning string.

The `all` wrapper is ordinary readable delegation to
`hs_trigger_volume_test_objects`; VC7 inlines and constant-folds it into the
160-byte January target without a helper relocation.

## Fail-closed rejections

The first object also emitted six semantically grounded but nonexact bodies.
They were removed wholesale after measurement:

| Rejected function | Target / candidate padded bytes | Measured first-shot difference |
| --- | ---: | --- |
| `_hs_objects_can_see_object` | 128 / 112 | Same four destinations, but all relocations are two bytes early and the candidate lacks the target's initial EBX result lifetime. |
| `_hs_unit_can_see_flag` | 80 / 80 | Same four destinations, but target schedules the radian multiply before scenario/tag-block lookup while the candidate schedules it afterward. |
| `_hs_objects_can_see_flag` | 160 / 160 | Same seven destinations and extent, but the same multiply/lookup scheduling difference propagates through the inlined flag predicate. |
| `_hs_object_destroy` | 64 / 64 | Same four destinations and extent; candidate emits the protected-object error arm before delete, while target lays out delete first and branches to the error arm. |
| `_hs_object_destroy_all` | 192 / 176 | Same 11 destinations; iterator/lifetime topology produces a shorter prologue and systematically earlier calls. |
| `_hs_object_set_permutation` | 192 / 192 | Same eight destinations and most relocation addresses; the final `object_permute_region` call is three bytes late due the loop-exit/local schedule. |

No alternate declaration order, branch spelling, local lifetime, helper shape,
or scheduling device was compiled in this wave. These measurements are the
starting evidence for a future fresh wave.

After the rejected callers were removed, `code_000b8c80` was given external
translation-unit linkage so the exact target function remains naturally emitted
without a synthetic caller or anchor. January's split symbol has external
storage. This changes no function expression or byte recipe; the retained-state
rebuild independently confirms the complete 176-byte body and all six
relocations remain strict exact.

The rejected public definitions are absent from the retained object. The
existing undefined `_hs_object_destroy` reference remains because already-exact
wrappers correctly call the function owned by the eventual source definition.

## House-rule conformance

The retained source uses:

- `void` on its own line for no-argument functions;
- one parameter per line;
- an explicit terminal `return;` for every retained void function;
- `object_get`, `unit_try_and_get`, and `player_get` typed access macros;
- `TAG_BLOCK_GET_ELEMENT` and existing typed definition macros for tag access;
- top-of-scope C89 declarations and typed iterator/data structures.

The single item-prefix view is centralized through the TU-local
`hs_item_datum_from_object` typed macro; there are no repeated casts after raw
object lookup. The source contains no assembly, `register`, `volatile`, pragma,
intrinsic, attribute, optimizer barrier, raw address, pointer/integer
reconstruction, type pun, undefined behavior, synthetic anchor, byte forcing,
object patch, comparator exception, or compiler substitution.

## Frozen experiment and retained replay

Before mutation, the baseline manifest was written to
`build/audit/hs_library_external_closure_baseline_20260827.json`, SHA-256
`94b8af71fa733e813289a31b0e237b8e73d0d57ce5231a6f4a8cefd20a657356`.
It freezes published commit `f95fb100` and the sole unit
`source/hs/hs_library_external`.

Exactly one all-12 candidate production edge ran:

```text
[1/1] CL build\base\source\hs\hs_library_external.obj
hs_library_external.c
```

The first object was immediately preserved at
`build/audit/hs_library_external_closure_first_shot_20260827.obj`, SHA-256
`0abbac748596b7b48fcd76a5ee28accaec3edfe0dc21e85037004a3f3e20f9ae`.
The compiler repeated only the inherited C4047 diagnostic at the already-exact
effect-marker call.

The fail-closed regression replay rebuilt the pruned retained source. Its object
was preserved at
`build/audit/hs_library_external_closure_retained_20260827.obj`, SHA-256
`16ffe4f6c80d5fa2f8dea321f63d427a2dec48f8a14bccb4aa0a25b28d94c85b`.
It reports exactly the six retained functions as `NEWLY_EXACT`, reports
`changed_nonexact: []`, and emits none of the six rejected definitions.

The gate flags inherited COMDAT/non-code evidence indices and symbol ownership
because the six new functions add sections, a warning string, and the
`__real@3c8efa35` constant. No adjudication file or waiver was used. Direct
hardened comparison of the retained object proves all 24 inherited functions
and all six new functions strict exact: 30/30 accepts, with no inherited code
regression.

## Full validation and progress

- Complete Halo and libcmt build targets: pass, with no work remaining after
  the retained replay.
- Split, ordinary report, semantic report, and progress generation: pass.
- Strict board: 260/619 exact objects, 4,302/8,246 functions, and
  557,527/1,922,669 padded bytes.
- `hs_library_external.obj`: 30/36 functions and 3,008/3,824 padded bytes.
- Semantic audit: 470 units, 4,462 functions evaluated, 4,311 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,371 accepted, and zero unit errors.
- `build/report.json` SHA-256:
  `a85cf7b193ced0ee2272074087feeeac91f0fdd0feb6937c65762d0e58543d52`.
- `build/semantic_report.json` SHA-256:
  `bab73ca5c79da1cdeb88bf1b0dbc44f7afe3c4c16dc5a9e144b3c19c5cfda354`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 194 passed, with only the known unwritable
  pytest-cache warning.
- Tools publication audit: 121 tracked paths and zero changed or untracked
  `tools/**` paths.
- `git diff --check`: clean apart from Git's line-ending notice.

The unit remains `NonMatching` in `config/config.json`; no object-completion or
data-completion label is changed.
