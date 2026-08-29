# `collision_usage.obj` timing/display wave

Date: 2026-08-29

## Outcome

`source/physics/collision_usage.obj` advances from 7/14 to 11/14 strict-exact
functions. This wave contributes four functions and 352 padded code bytes:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_0013c830` | 48 | 0 | `7f3de96dc0279dda00bf5c478aceabbf97a62951b3c1e800b18d802dcf25ac53` |
| `_collision_log_end_time` | 128 | 5 | `8ddd5180ba30cb5e82c2c3420ead82757ac792dd3550e4cf75441f8113acec7d` |
| `_collision_log_usage` | 80 | 4 | `f0587256772f378c123b31715c2a40d5f72d85ca613ac889eecf92c7da82de3d` |
| `_collision_log_display` | 96 | 11 | `f805f6626124e29a47d961908653975a266ebb039d58cc884e0447033b8d3bb8` |

The object remains `NonMatching`:

```text
UNWRITTEN    96  _code_0013ca00
residual    256  _code_0013d090  [sha]
UNWRITTEN  1584  _collision_log_render
== exact 11  residual 1  unwritten 2  (of 14 listed)
```

No completion label, parked-function entry, exception, or comparator rule was
changed.

## Recovery evidence

The earlier Codex checkpoint recorded that a typed `LARGE_INTEGER` probe made
`collision_log_end_time` strict exact, but removed it because the required
private current-user helper remained nonexact. The current campaign measures
functions independently, so that exact public body is restored together with
its natural `collision_log_usage` sibling. Both callers use the same readable
typed helper and match their January sections exactly.

The target render disassembly establishes the comparator's aggregate layout.
It sorts 22 elements of 72 bytes and reads the call count at offset 8. The
ordinary structure follows naturally from the compiler's eight-byte alignment
of `struct collision_log`:

```c
struct collision_overall_usage
{
	short user_index;
	struct collision_log total_all_periods;
	struct collision_log usage_by_period[NUMBER_OF_COLLISION_TIME_PERIODS];
};
```

The comparator performs the target's descending three-way comparison over
`total_all_periods.calls`. It needs no raw offset, packed layout, byte array,
representation cast, or comparator exception.

The 96-byte display routine is reconstructed from its complete target
relocation and instruction stream. It appends the six named collision-family
counts from period zero using the target format string. Typed enum indices and
`struct collision_period` fields reproduce all eight aggregate addends and the
target's evaluation order exactly.

## Remaining helper boundary

The readable private `code_0013d090` body is retained as a residual because it
provides the real call target for the two exact public routines. Its control
flow, assertions, API calls, size, and instruction count reproduce the target.
The final successful return uses the compiler's ordinary signed-short
promotion (`movsx eax, si`) where January used a 16-bit `mov ax, si`. Bounded
return-type and local-result probes either retained that instruction or changed
the section topology, so no scheduling device or return-width trick is used.

The two remaining unwritten functions are a private formatting helper and the
1,584-byte renderer. The private helper uses the renderer's in-TU calling
convention and is intentionally not emitted without its real caller.

## Source policy

All retained code is readable typed C. It introduces no assembly, `volatile`,
`register`, pragma, barrier, intrinsic, attribute, raw address, raw offset,
representation punning, incompatible pointer cast, undefined behavior, byte
forcing, object patching, compiler-flag change, or comparison exception.
No-argument definitions use own-line `void`, edited declarations use one
parameter per line, and all void bodies end with an explicit `return;`.

The pre-existing `#pragma once` in `collision_usage.h` is untouched.

## Validation

- Forced translation-unit build: pass.
- `python tools/campaign/gate.py source/physics/collision_usage --all`: 11/14
  strict exact, one measured residual, two unwritten.
- Full `halobetacache_build`, `semantic_progress`, and `progress`: pass.
- Semantic audit: 470 units, 4,849 functions evaluated, 4,732 accepted exact,
  and zero unit errors.
- Strict campaign board: 276/619 complete objects, 651,479/1,922,669 padded
  code bytes, and 4,704/8,246 functions. Parked and assembly-implemented
  functions are not counted.
- Tooling suite: 205/205 tests pass.
- Scoped forbidden-construct scan and `git diff --check`: pass.

Reproducibility identities before commit:

- source SHA-256:
  `90955860FDAE513F5C50B065F8EC0312B6814D7302F55E9BDAAC44677515F88D`;
- rebuilt object SHA-256:
  `AB5B7E85783B39379A52329060454F91AAD4CC08916146CB3BC94C5C2FC5D9BC`;
- January target object SHA-256:
  `E829727ABB548E1A649425732E4C12B60A4EE50D60060FB60BB4F68EF95CB6A4`.
