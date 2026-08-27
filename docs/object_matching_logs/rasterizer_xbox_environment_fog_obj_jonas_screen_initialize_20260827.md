# `rasterizer_xbox_environment_fog.obj` screen-initialize recovery (2026-08-27)

## Frozen evidence

This wave adds `_rasterizer_environment_fog_screen_initialize` and its owning
`errors.h` include to
`source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`. The pre-edit source
blob was `f61163e00bf8cf192bb9d7a7510d43fcfbcc336b`; the retained source blob is
`41a3fd599506ceeb542eb3d1d03ca0c25630b7e4`.

The January target is 70 meaningful bytes / 80 padded bytes, with normalized
SHA-256
`c0ab439f289d787f9c4f7062c9117e58480a6cc6ea3c0639c5a605e1f733fa08`.
Its six relocations identify the complete source behavior:

- `DIR32 +0x07` to the exact source-file string COMDAT;
- `REL32 +0x15` to `_debug_malloc`;
- `DIR32 +0x1f` to `_bss_00465ad0 + 592`, the allocation pointer;
- `DIR32 +0x25` to `_bss_00465ad0 + 596`, the submission count;
- `DIR32 +0x30` to the exact 84-byte allocation-error string COMDAT;
- `REL32 +0x37` to `_error`.

The target also proves allocation size `0x5000`, `clear = FALSE`, January line
`0xF8`, `_error_silent`, unconditional count clearing, and a boolean result
initialized to true and cleared only on failure.

The target object SHA-256 is
`D12EA6F0BA18BC4E6E25921090F448B27ADAD35F5563477EA0CD64B6305E021C`.
The frozen baseline is
`build/audit/rasterizer_xbox_environment_fog_screen_initialize_baseline_20260827.obj`,
SHA-256
`446F33D34055AA12DF9CE54CCBC14CE95E1CBF3828527070DAD91EC4F5F0D85E`.

## Recovery result

The first build attempt stopped before producing an object because this sparse
translation unit did not include `errors.h`, the public owner of
`_error_silent` and the variadic `error` prototype. Adding that established
dependency did not change the frozen source reconstruction. The first
code-producing candidate is strict exact.

The retained object is
`build/audit/rasterizer_xbox_environment_fog_screen_initialize_first_shot_20260827.obj`,
SHA-256
`0573A944A4C71D07A17F14B67E0553C576D9088CC5C5F26719DF5B638A560905`.
The hardened comparator proves exact padded bytes and exact relocation
addresses, types, destinations, addends, and ownership.

## Regression and campaign gates

The frozen exact-owner comparison preserves all seven inherited exact
functions and adds exactly `_rasterizer_environment_fog_screen_initialize`;
`lost_exact` is empty. The translation unit advances from 7/27 to 8/27 strict
functions.

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,402 code bytes, 4,245 functions;
- Halo: 275/468, 498,488 code bytes, 4,078 functions;
- semantic audit: 4,356 evaluated, 4,211 semantic exact, 4,271 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

No assembly, intrinsic, raw address, pointer pun, scheduling barrier,
admission exception, compiler substitution, or object patch was used.
