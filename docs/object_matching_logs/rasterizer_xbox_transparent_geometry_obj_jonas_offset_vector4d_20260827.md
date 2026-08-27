# `rasterizer_xbox_transparent_geometry.obj` offset-vector recovery (2026-08-27)

## Scope and evidence

This wave adds only `_offset_vector4d` to
`source/rasterizer/xbox/rasterizer_xbox_transparent_geometry.c`. The pre-edit
source blob was `aa36c246e46a0976947fc5a1df6f665e5a871c91`; the retained
source blob is `534a56bde16a13c4eceac250b0bb7a08ea4bfdb7`.

The target is 59 meaningful bytes / 64 padded bytes, has zero relocations, and
has normalized SHA-256
`dc90612d420e8d485c954354588a0f4416e2254c3b50ee3bdc6ea8407387412b`.
Its parameter loads prove the order `base`, `vector`, `scale`, `result`; its
four x87 groups are exactly `vector[n] * scale + base[n]`, and `EAX` retains
the typed result pointer.

The compiler-authenticated source donor is the existing exact
`point_from_line3d` helper. The retained source is its direct typed extension
through `real_vector4d::l`, including the result-pointer return.

The target object SHA-256 is
`48BBAC6D2275B9FB1DAEA94A341BE1FBCCDD77B4BB1B5AE6E46F49699C2812EB`.
The frozen baseline is
`build/audit/rasterizer_xbox_transparent_geometry_offset_vector4d_baseline_20260827.obj`,
SHA-256
`3FD6ED0E5D7BB697B5347E91795B1CD0D183B0FA94C3A2FF4B1B2AA4DBE577FD`.

## First-shot result

The first and only code-producing candidate is strict exact. Its frozen object
is
`build/audit/rasterizer_xbox_transparent_geometry_offset_vector4d_first_shot_20260827.obj`,
SHA-256
`7C239E828D22480917DED53C7DB5E625726A9A1DD6901D1C3AC912BFCDA0E9A4`.
The hardened comparator reports identical padded bytes and zero relocations.
The inherited `_subtract_vectors4d` remains exact; the exact-owner delta adds
only `_offset_vector4d`, with `lost_exact` empty. The object advances from
1/22 to 2/22 strict functions.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,508 code bytes, 4,247 functions;
- Halo: 275/468, 498,594 code bytes, 4,080 functions;
- semantic audit: 4,358 evaluated, 4,213 semantic exact, 4,273 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the parameter and terminal-return house rules. No
protected source, assembly, intrinsic, raw address, pointer pun, barrier,
exception, compiler substitution, or object patch was used.
