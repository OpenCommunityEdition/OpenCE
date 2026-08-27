# `rasterizer_xbox_environment_fog.obj` local random boolean recovery (2026-08-27)

## Scope and frozen evidence

The retained change adds only `_local_random_boolean` to
`source/rasterizer/xbox/rasterizer_xbox_environment_fog.c`. The pre-edit source
blob was `16ef3946d543dba5a7d090a5fac02f50bdbdf999`; the retained source blob is
`f95cb8c9cc1b35630d347ab6bef92fe6e9654d4f`.

The January target function is 26 meaningful bytes / 32 padded bytes, with
normalized SHA-256
`3bd7359afa67fecf48892651f71ca0202b6f62c8e2c5bc6a85641440bba3df2a`.
It has two `IMAGE_REL_I386_REL32` relocations:

- `+0x01` to `_get_global_local_random_seed_address`;
- `+0x07` to `_seed_random`.

The target object SHA-256 is
`D12EA6F0BA18BC4E6E25921090F448B27ADAD35F5563477EA0CD64B6305E021C`.
The frozen baseline is
`build/audit/rasterizer_xbox_environment_fog_local_random_boolean_baseline_20260827.obj`,
SHA-256
`04600D05D57E775817B786BD656E4656524C8A6E30BDEE0B678C66F973E6DFA5`.

## Source recovery and bounded retry

The target disassembly is the canonical boolean lowering around one 16-bit
random draw. The existing `real_math.h` helpers authenticate the required
source domains:

```c
boolean local_random_boolean(
	void)
{
	return local_random() > 0x8000;
}
```

The initial evidence-backed spelling used `random_boolean()`. Its object is
`build/audit/rasterizer_xbox_environment_fog_local_random_boolean_first_shot_20260827.obj`,
SHA-256
`DF86E9062CA34FE6E7D855B8DE577B7FD3931D8BA9EA8C6AD1E4A92A4EA197B7`.
It reproduced every padded instruction byte and both relocation addresses and
types, but the first relocation named `_get_global_random_seed_address`.
That exact failure supplied new evidence that the January body consumes the
local seed. The one bounded retry therefore used the established
`local_random()` helper; it was not a source-shape sweep.

The retained object is
`build/audit/rasterizer_xbox_environment_fog_local_random_boolean_local_seed_20260827.obj`,
SHA-256
`75E8EFDF6E04491BEE0137B80F2630CB973669F809899BC88BD51370643CA981`.
The hardened comparator reports exact padded bytes and exact relocation
addresses, types, destinations, addends, and ownership.

## Regression and campaign gates

The frozen exact-owner comparison preserves all five inherited exact
functions and adds exactly `_local_random_boolean`; `lost_exact` is empty.
The retained translation unit is now 6/27 strict-exact functions.

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,303 code bytes, 4,243 functions;
- Halo: 275/468, 498,389 code bytes, 4,076 functions;
- semantic audit: 4,354 evaluated, 4,209 semantic exact, 4,269 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. The full test suite passes 179 tests; the only
warning is the existing sandbox-denied pytest cache write.

No assembly, intrinsic, raw address, pointer pun, scheduling barrier,
admission exception, compiler substitution, or object patch was used.
