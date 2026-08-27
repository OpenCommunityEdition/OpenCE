# `rasterizer_xbox_transparent_geometry.obj` auxiliary-buffer disposal recovery (2026-08-27)

## Scope and frozen evidence

This wave adds `_rasterizer_transparent_geometry_dispose_aux_buffer`, the
complete 16-byte target-owned BSS layout, and the established stdcall
`D3DResource_Release` prototype to
`source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c`.
The pre-edit source blob was `534a56bde16a13c4eceac250b0bb7a08ea4bfdb7`;
the retained source blob is `5acb480b63469034ab7d83a895e92e133a392a1f`.

The target function is 26 meaningful bytes / 32 padded bytes, with normalized
SHA-256
`11c10cbd7e9b7939d6ea69fdb0be8d1b344e1b90439fcafc6d23b0086fbf7513`.
Its three relocations are:

- `DIR32 +0x01` to `_bss_004662d8 + 4`;
- `REL32 +0x0b` to `_D3DResource_Release@4`;
- `DIR32 +0x11` to `_bss_004662d8 + 4`.

The target BSS section is exactly 16 zero bytes, normalized SHA-256
`374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb`.
Other target functions bound its visible layout: a long at `+0`, the resource
pointer at `+4`, a boolean at `+8`, and a seven-byte unknown tail. A compile-
time size assertion preserves that complete layout without inventing tail
semantics.

The `D3DResource_Release(void *)` declaration and guarded release/clear source
shape are already established in
`rasterizer_xbox_hardware_bitmaps.c`. No raw COM vtable access is used.

The target object SHA-256 is
`48BBAC6D2275B9FB1DAEA94A341BE1FBCCDD77B4BB1B5AE6E46F49699C2812EB`.
The frozen baseline is
`build/audit/rasterizer_xbox_transparent_geometry_dispose_aux_buffer_baseline_20260827.obj`,
SHA-256
`7C239E828D22480917DED53C7DB5E625726A9A1DD6901D1C3AC912BFCDA0E9A4`.

## First-shot result

The first and only code-producing candidate is strict exact. Its frozen object
is
`build/audit/rasterizer_xbox_transparent_geometry_dispose_aux_buffer_first_shot_20260827.obj`,
SHA-256
`4CCD812BDC204211692F8E82102B576252CCF7E2C77EE16BDC12275E98B6DA9B`.

The hardened comparator proves exact padded function bytes and all three exact
relocations. It separately proves exact BSS size, contents, ownership, and
zero relocation count. Both inherited vector helpers remain exact;
`lost_exact` is empty. The object advances from 2/22 to 3/22 functions and
from no accepted data to 16/16 accepted BSS bytes.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,534 code bytes, 4,248 functions, 1,836,778 data
  bytes;
- Halo: 275/468, 498,620 code bytes, 4,081 functions, 1,831,586 data bytes;
- semantic audit: 4,359 evaluated, 4,214 semantic exact, 4,274 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the no-argument and explicit terminal-return house
rules. No protected source, assembly, intrinsic, raw address, pointer pun,
barrier, exception, compiler substitution, or object patch was used.
