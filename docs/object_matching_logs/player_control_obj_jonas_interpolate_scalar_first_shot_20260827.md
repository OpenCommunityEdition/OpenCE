# `player_control.obj` scalar interpolation recovery (2026-08-27)

## Scope and provenance

This wave adds only `_interpolate_scalar` and its translation-unit-local
inline-wrapper rename in `source/game/player_control.c`. The object advances
from 39/49 to 40/49 strict-exact functions and remains `NonMatching`.

The object-ledger census and baseline symbol table contain no earlier
out-of-line owner or production candidate for this function. The canonical PC
inline computes `desired - *current`, clamps that delta to plus or minus the
maximum speed with the established `PIN` macro, and adds it to the current
value. The independent HCEA reconstruction at
`build/audit/refs/halocea/src/blam/math/interpolate_scalar.c` confirms the same
typed two-bound clamp and update. January remains the byte, x87-predicate,
padding, and ownership authority.

The pre-edit source blob was `464fc6293b2a31976d620a8add483990c215d8b5`;
the retained source blob is `324d483d0d5154e15b61e9aa95c81cfca32c1e18`.

## Frozen target and first shot

The January target is exactly 64 meaningful and padded bytes, has zero
relocations, and has normalized SHA-256
`9b8ea19a97a241749dd4c3b6096d56f33061cc15a23118d913022eddacc487bd`.
The target object SHA-256 is
`7B282D76BDC4B571E5FAB060391800565B94B134124742484A0F6473106C1BDC`.

The frozen baseline is
`build/audit/player_control_interpolate_scalar_baseline_20260827.obj`,
SHA-256
`C54564FDAE0065B66390AC6F34D0C898B1B821C256CB8147CF7A7EF4901A058B`.

The first and only code-producing candidate is strict exact. Its frozen object
is
`build/audit/player_control_interpolate_scalar_first_shot_20260827.obj`,
SHA-256
`5D25C01725F4E396E4F3F5E9396B3BD1DCE6509EA0C036DD2206F72FB2A7A9E1`.
The hardened comparator proves all 64 bytes exact and confirms zero
relocations. All 39 inherited exact functions remain exact; `lost_exact` is
empty and the sole `newly_exact` entry is `_interpolate_scalar`.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,843 code bytes, 4,254 functions, 1,836,778 data
  bytes;
- Halo: 275/468, 498,929 code bytes, 4,087 functions, 1,831,586 data bytes;
- semantic audit: 4,365 evaluated, 4,220 semantic exact, 4,280 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the one-parameter-per-line and explicit terminal-return
house rules and uses ordinary typed C89 math. No protected source, assembly,
intrinsic, raw address, pointer pun, barrier, exception, compiler substitution,
or object patch was used.
