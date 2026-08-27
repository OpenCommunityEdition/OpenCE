# `structure_detail_objects.obj` 4D dot-product recovery (2026-08-27)

## Scope and prior-ledger status

This wave adds only `_dot_product4d` and the established inline-wrapper rename
around `real_math.h` in `source/structures/structure_detail_objects.c`.
The prior bounded ledger explicitly says this function was excluded and never
authored or tested; this is not a replay of a rejected candidate.

The pre-edit source blob was `3a8c1add9f78385235e855d1f030ef19abc86897`;
the retained source blob is `71fa727d5860c397157cb3a9995322c64a27c3a1`.

## Frozen target and donor

The January target is 39 meaningful bytes / 48 padded bytes, has zero
relocations, and has normalized SHA-256
`c44dc8dad9251d4d3f4dca50da952a26b8d087ab4ab09b20c1eb92cae21fe516`.

The source authority is the existing canonical inline `dot_product4d` in
`real_math.h`:

```c
return a->i*b->i + a->j*b->j + a->k*b->k + a->l*b->l;
```

VC7 lowers that expression in the target's reverse x87 load order (`l`, `k`,
`j`, `i`) without any source scheduling device.

The target object SHA-256 is
`1BEA58EFF13264FB6E322E07E27E7A204D3390A025A1B67525ABB3E0C15344DA`.
The frozen baseline is
`build/audit/structure_detail_objects_dot_product4d_baseline_20260827.obj`,
SHA-256
`B8729C8C9A8A64899D80D9A3249973194C18AC20876998B9E42779CB0027FACC`.

## Candidate result

The preliminary compile stopped before producing an object because the inline
header already owned the same source spelling. The repository-established
wrapper pattern, also used by `action_charge.c`, renames the inline during
header inclusion and then emits the required out-of-line function. This was a
dependency correction; the function body did not change.

The first code-producing candidate is strict exact. Its frozen object is
`build/audit/structure_detail_objects_dot_product4d_first_shot_20260827.obj`,
SHA-256
`159C3FA51CD9EF03EE714A349EF1A882BCF2C45E52CCC21CFF0026405D34E7F2`.
The hardened comparator reports identical padded bytes and zero relocations.
All three inherited exact functions and the accepted BSS owner remain exact;
`lost_exact` is empty. The object advances from 3/15 to 4/15 strict functions.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,573 code bytes, 4,249 functions;
- Halo: 275/468, 498,659 code bytes, 4,082 functions;
- semantic audit: 4,360 evaluated, 4,215 semantic exact, 4,275 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the parameter and terminal-return house rules. No
protected source, assembly, intrinsic, raw address, pointer pun, barrier,
exception, compiler substitution, or object patch was used.
