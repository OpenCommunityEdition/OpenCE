# `rasterizer_debug.obj` wrapper-pair recovery (2026-08-26)

## Scope and result

This bounded wave recovers `_rasterizer_debug_line` and
`_rasterizer_debug_triangle` from the January target object. Both natural
wrapper bodies are strict exact on the first and only code-producing candidate
compile. The three inherited exact functions remain strict, so
`source/rasterizer/rasterizer_debug.obj` advances from **3/12 to 5/12
strict-exact functions** and remains a partial object.

The gain is **57 meaningful code bytes**: 26 bytes for the line wrapper and 31
bytes for the triangle wrapper. No whole-object completion or data-byte credit
is claimed.

## Frozen evidence

- cumulative baseline commit:
  `950a671f72fa9e6e84f2f1e6ae8ce6ba525d9035`
- January target object SHA-256:
  `46317F071D3E4DDC3669910FCF786D17CD48F76314D73EE81FE68D47621FFC1E`
- cumulative baseline object SHA-256:
  `60CE42A84C1F20465ECDF2F59A13CA1085F08A4FB66FA9ACAD821447FBE0FF14`
- frozen baseline source SHA-256:
  `EA3F057581D16B20B97801942C788A5221A705E1053DA5AB825AF46F220C8146`
- first candidate source SHA-256:
  `DA6677CF9401D0455A94E7042816E2947071D09281B8389FD34D5E2E9D28A3C5`
- first candidate object SHA-256:
  `EC62EAEE187ADDCB62AE1C6879EC9E0A4FB116E1257C171DBDDD05C86F30EBFF`
- clean pre-wave regression manifest:
  `outputs/rasterizer_debug_wrapper_evidence_20260826/regression-baseline.json`

The target, baseline, preserved first candidate, and source copies are retained
under `outputs/rasterizer_debug_wrapper_evidence_20260826/`. No candidate was
recompiled after the accepted first artifact.

Before the compile, the wave used the January COFF as binary authority and
reviewed the current campaign methodology and exact-match acceleration
playbook. Later HaloCEA rasterizer-debug sources were consulted only as a
semantic cross-check: that later build supplies no-op wrapper stubs, so it is
not code-shape provenance for this target.

## Target-authoritative reconstruction

The January line wrapper pushes its input color pointer twice and calls the
shaded line routine with `(p0, p1, color, color)`. The natural C wrapper emits
the exact 32-byte padded COMDAT, including its sole `REL32` relocation at
offset 17 to `_rasterizer_debug_line_shaded`:

- meaningful size: 26 bytes
- padded size: 32 bytes
- relocation count: 1
- normalized SHA-256:
  `60f8ba4857cfb47aa739f77d4df9844fae5d61fb4292d7d92b25c28166a7a0d7`

The January triangle wrapper pushes its input color pointer three times and
calls the shaded triangle routine with `(p0, p1, p2, color, color, color)`.
The natural C wrapper emits the exact 32-byte padded COMDAT, including its sole
`REL32` relocation at offset 22 to `_rasterizer_debug_triangle_shaded`:

- meaningful size: 31 bytes
- padded size: 32 bytes
- relocation count: 1
- normalized SHA-256:
  `1b0df8cc752bec5820c858223f553d6f62fc7e39bac45bbb9b209d0f980a148a`

The recovered source follows the campaign format: one parameter per line and
an explicit final `return;` in each void function. It introduces no assembly,
register forcing, pragmas, volatile barriers, raw addresses, pointer/integer
conversions, or undefined behavior.

## Inherited strict sentinels

The hardened comparator also proves the three pre-wave exact functions remain
identical in the preserved candidate:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_rasterizer_debug_begin` | 32 | 4 | `6714caa619cbcb397fdb1e12b1e8173dd5f848508b210f9386dcc251b620bd8f` |
| `_rasterizer_debug_end` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_debug_test` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

For `_rasterizer_debug_begin`, all four absolute relocations retain the target
addresses, types, addends, and ownership through `_bss_004662f8` at offsets 8,
16, 24, and 28.

## Ownership and regression gates

The target owns 12 code functions. The frozen baseline candidate owns only the
three inherited exact functions. The accepted candidate adds exactly the two
target-owned wrapper COMDATs, for five emitted target code owners in total. It
adds no defined runtime non-code external, helper COMDAT, constant COMDAT,
COMMON symbol, or candidate-only code owner.

A whole-translation-unit check against the clean pre-wave manifest reports:

- `NEWLY_EXACT`: `_rasterizer_debug_line`,
  `_rasterizer_debug_triangle`;
- `still_exact`: `_rasterizer_debug_begin`, `_rasterizer_debug_end`,
  `_rasterizer_debug_test`;
- `changed_nonexact`: none.

The remaining physical differences are the expected symbol inventory and
`.debug$S` changes caused by adding two source-backed public functions. Direct
strict comparisons independently prove all five accepted functions. A clean
post-admission snapshot is required after integration so subsequent work can
fail closed from the new accepted state.

## Remaining frontier

Seven target functions remain nonexact:

- `_rasterizer_debug_new_primitive`
- `_rasterizer_debug_initialize`
- `_rasterizer_debug_dispose`
- `_code_0016d980`
- `_rasterizer_debug_draw`
- `_rasterizer_debug_line_shaded`
- `_rasterizer_debug_triangle_shaded`

The two shaded routines are large private implementation bodies and should be
recovered with their common state/primitive helper surface rather than tuned as
isolated external cdecl leaves. Reopen them only after freezing caller, global,
literal, and relocation ownership together.

## Post-integration verification

The accepted recovery was integrated at cumulative commit
`41766c50b50fcaf9583180a0b9449432fdc86682`. A normal rebuild reproduced all
five strict functions. The clean post-admission regression manifest is
`outputs/rasterizer_debug_wrapper_evidence_20260826/regression-post-admission.json`;
its immediate replay passed with no failures, warnings, newly exact functions,
or changed nonexact functions.

The refreshed campaign gates report:

- semantic audit: 470 units, 4,296 evaluated functions, 4,213 accepted exact,
  zero unit errors;
- object admission: zero candidates and zero revoked objects;
- parked-function validation: three active, zero stale, zero invalid;
- tooling suite: 179/179 tests pass;
- all code: 507,676 / 2,198,102 bytes and 4,187 / 11,060 functions;
- Halo code: 494,762 / 1,770,166 bytes and 4,020 / 7,574 functions;
- completed files: unchanged at 376 / 833 overall and 274 / 468 Halo.

Relative to the frozen pre-wave metrics, this is exactly the intended gain of
57 code bytes and two functions with no data or completed-object movement.
