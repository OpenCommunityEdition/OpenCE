# `rasterizer_xbox_transparent_geometry.obj` subtract-vector recovery (2026-08-27)

## Scope and evidence

This wave moves to a different unprotected Halo object and adds only
`_subtract_vectors4d` plus the established `cseries.h` / `real_math.h`
dependencies to
`source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c`.
The pre-edit source blob was `b1aa103a73ac9f6e5f1904504d17e717a6ec298c`;
the retained source blob is `aa36c246e46a0976947fc5a1df6f665e5a871c91`.

The target body is 47 meaningful bytes / 48 padded bytes, has zero
relocations, and has normalized SHA-256
`c75656fb75fb83b0bd4d027d0576ba5f273723407bec8218c8a8fedd249b7895`.
Its parameter loads and retained `EAX` result prove the typed contract:
two `real_vector4d const *` inputs followed by a `real_vector4d *` result.

The compiler-authenticated donor is the existing exact project implementation
of `subtract_vectors3d`: component-wise subtraction in declaration order and a
typed result-pointer return. Extending the same source form through the `l`
component reproduces the target without casts or raw storage access.

The target object SHA-256 is
`48BBAC6D2275B9FB1DAEA94A341BE1FBCCDD77B4BB1B5AE6E46F49699C2812EB`.
The frozen baseline is
`build/audit/rasterizer_xbox_transparent_geometry_subtract_vectors4d_baseline_20260827.obj`,
SHA-256
`D37CE0E3A1D394AA843DA193CAAE902F84F97C7F6E7051A17D8DD5F96CB84726`.

## First-shot result

The first and only code-producing candidate is strict exact. Its frozen object
is
`build/audit/rasterizer_xbox_transparent_geometry_subtract_vectors4d_first_shot_20260827.obj`,
SHA-256
`3FD6ED0E5D7BB697B5347E91795B1CD0D183B0FA94C3A2FF4B1B2AA4DBE577FD`.
The hardened comparator reports identical padded bytes and zero relocations.
The exact-owner delta is exactly `_subtract_vectors4d`, with `lost_exact`
empty. This establishes the first strict function in the 22-function object.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,449 code bytes, 4,246 functions;
- Halo: 275/468, 498,535 code bytes, 4,079 functions;
- semantic audit: 4,357 evaluated, 4,212 semantic exact, 4,272 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the no-argument/parameter/terminal-return house rules
where applicable. No protected source, assembly, intrinsic, raw address,
pointer pun, barrier, exception, compiler substitution, or object patch was
used.
