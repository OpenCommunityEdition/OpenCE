# `rasterizer_xbox_environment_fog.obj` screen-dispose recovery (2026-08-27)

## Frozen target and baseline

This wave adds only `_rasterizer_environment_fog_screen_dispose` to
`source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`. The pre-edit source
blob was `f95cb8c9cc1b35630d347ab6bef92fe6e9654d4f`; the retained source blob is
`f61163e00bf8cf192bb9d7a7510d43fcfbcc336b`.

The January target is 29 meaningful bytes / 32 padded bytes, with normalized
SHA-256
`047eec55aebecf5cc245ddb934ad186ed5e81c667272e9cc7e7f84ccfc9c289b`.
Its three relocations are:

- `DIR32 +0x01` to `_bss_00465ad0 + 592`, the opaque submission buffer;
- `DIR32 +0x0f` to the exact
  `c:\halo\SOURCE\rasterizer\xbox\rasterizer_xbox_environment_fog.c`
  string COMDAT;
- `REL32 +0x15` to `_debug_free`.

The target object SHA-256 remains
`D12EA6F0BA18BC4E6E25921090F448B27ADAD35F5563477EA0CD64B6305E021C`.
The frozen baseline is
`build/audit/rasterizer_xbox_environment_fog_screen_dispose_baseline_20260827.obj`,
SHA-256
`75E8EFDF6E04491BEE0137B80F2630CB973669F809899BC88BD51370643CA981`.

## Recovery

The target control flow is one null guard around `debug_free`, with January
line `0x117`, followed by an explicit return. It does not clear the pointer.
The first and only code-producing candidate used that direct ordinary-C form.

The retained object is
`build/audit/rasterizer_xbox_environment_fog_screen_dispose_first_shot_20260827.obj`,
SHA-256
`446F33D34055AA12DF9CE54CCBC14CE95E1CBF3828527070DAD91EC4F5F0D85E`.
The hardened comparator proves exact padded bytes and exact relocation
addresses, types, destinations, addends, and ownership.

## Regression and campaign gates

The frozen exact-owner comparison preserves all six inherited exact functions
and adds exactly `_rasterizer_environment_fog_screen_dispose`; `lost_exact` is
empty. The translation unit advances from 6/27 to 7/27 strict functions.

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,332 code bytes, 4,244 functions;
- Halo: 275/468, 498,418 code bytes, 4,077 functions;
- semantic audit: 4,355 evaluated, 4,210 semantic exact, 4,270 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

No assembly, intrinsic, raw address, pointer pun, scheduling barrier,
admission exception, compiler substitution, or object patch was used.
