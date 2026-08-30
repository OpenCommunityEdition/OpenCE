# Xbox transparent-geometry group-begin boundary (2026-08-30)

## Result

`_rasterizer_transparent_geometry_groups_begin` reached a clean first-shot
compiler boundary and was removed. The target and sole candidate have the
same 48-byte padded extent, the same control flow and operations, and the same
five relocation destinations in the same order, but they are not byte exact:

| Build | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January | 43 | 48 | 5 | `cbe5cb045cd25c38b415e0dfb368058ca9fcd06d5c641392a1e3eec2f76e77eb` |
| Sole candidate | 44 | 48 | 5 | `92696fd458a221b8b3adc4680b560c7da04ee6837548340b761c989a79ed73fe` |

The complete exact-set comparison preserved `_subtract_vectors4d`,
`_offset_vector4d`, and
`_rasterizer_transparent_geometry_dispose_aux_buffer`, with no lost exact
function and no newly exact function.

## Evidence and frozen source form

January's object proves a public no-argument leaf. It clears the long at
`_bss_004662d8+0`, clears the boolean at `+8`, then calls
`D3DDevice_BeginVisibilityTest` only when the two adjacent rasterizer debug
booleans at `+0x88/+0x89` are true and the current window index at
`_global_window_parameters+2` is not `NONE`. Its ordered relocations are:

- DIR32 `+0x01` to `_rasterizer_debug_options+0x88`;
- DIR32 `+0x0B` to `_bss_004662d8+0`;
- DIR32 `+0x11` to `_bss_004662d8+8`;
- DIR32 `+0x1E` to `_global_window_parameters+2`;
- REL32 `+0x26` to `_D3DDevice_BeginVisibilityTest@0`.

The prior Xbox transparent-geometry ledgers authenticate the complete 16-byte
target-owned BSS and all three inherited exact functions. Claude's
`transparent_geometry_recovery.md` independently attests the function name
and surrounding transparent-geometry state semantics, but no audited Claude,
Jonas, Stian, Pastudan, or other registered source tree contains this Xbox
body. Canonical history contains no earlier definition or candidate. January
therefore remained the byte authority.

The one natural typed C89 form used named fields for the two debug booleans,
the BSS reset values, and window index; it declared the stock no-argument
stdcall XDK function and used a single `if` with a terminal `return`. It
contained no assembly, volatile/register/pragma/intrinsic, attribute,
optimizer barrier, raw address or raw offset access, pointer/integer
reconstruction, representation pun, undefined behavior, byte forcing, object
patch, exception, or compiler-option change.

## Exact boundary

January begins with a five-byte 32-bit load:

```text
A1 ........       mov eax, dword ptr [_rasterizer_debug_options+0x88]
```

The candidate begins with a six-byte 16-bit load:

```text
66 A1 ........    mov ax, word ptr [_rasterizer_debug_options+0x88]
```

Every subsequent instruction is otherwise structurally identical, so the
candidate's five relocation addresses are exactly one byte later:
`+0x02/+0x0C/+0x12/+0x1F/+0x27`. The candidate remains 48 bytes only because
it has one fewer tail NOP. This is an aggregate access-width/code-layout
boundary, not a semantic or link-closure defect.

The pre-shot base object and restored base object have SHA-256
`993fd89f24daa6536b59a756c1cd9a6c3092b2642a0f469bb1ba69a665a9c21e`.
The sole candidate object has SHA-256
`57efa86d8bd0a37da2afba9e9a953139713ac9225fea1f4bb8290687048f45ea`.
The production source was restored exactly to Git blob
`5acb480b63469034ab7d83a895e92e133a392a1f`; no candidate code, type,
prototype, or renamed field remains.

Do not tune the aggregate extent, add artificial adjacent storage, change
qualifiers, or retry source spellings from this measurement. Reopen only when
new authenticated source provenance independently establishes the complete
debug-options declaration that made January select the 32-bit load.

## Validation

The restored object retains all three inherited strict functions. The
integrating branch runs the full Halo/libcmt, semantic/progress, admission,
parked, tooling-test, and protected Units replay after concurrent exact lanes
are merged. This lane itself retains only this additive evidence record.
