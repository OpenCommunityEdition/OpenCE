# `player_control.obj` signed-angle recovery (2026-08-27)

## Scope and provenance

This wave adds only `_signed_angular_difference` and a translation-unit-local
inline-wrapper rename in `source/game/player_control.c`. The object advances
from 37/49 to 38/49 strict-exact functions and remains `NonMatching`.

The current object-ledger census, all-ref source history, and baseline symbol
table contain no earlier production candidate or out-of-line definition for
this owner. The canonical PC implementation is the existing `__inline`
function in `source/math/real_math.h`: subtract the two angles, subtract `2*pi`
when the result is at least `pi`, and add `2*pi` when it is at most `-pi`.
The independent HCEA reconstruction at
`build/audit/refs/halocea/src/blam/math/signed_angular_difference.c` confirms
the same two-branch topology and 32-bit float constants. January remains the
byte, relocation, padding, and ownership authority.

The pre-edit source blob was `1fa63b1eee8d9d92961cbb7c25347e1b1be20365`;
the retained source blob is `9b7d853f2dbbdd45d7f0b6ed38bf5a79f4035c7e`.

## Frozen target and first shot

The January target is 49 meaningful bytes / 64 padded bytes, with normalized
SHA-256
`c45ba91e0bdd268247bfef87604a2b00a3561c66a78a2e77623331db34bfd8f8`.
Its four `DIR32` relocations are at `+0x0b`, `+0x18`, `+0x1e`, and `+0x2b` to
the `pi`, `2*pi`, `-pi`, and reused `2*pi` float constants respectively.

The target object SHA-256 is
`7B282D76BDC4B571E5FAB060391800565B94B134124742484A0F6473106C1BDC`.
The frozen baseline is
`build/audit/player_control_signed_angular_difference_baseline_20260827.obj`,
SHA-256
`BA9F08D158F675850CF81B52E990DB1A3AC026B64DA750BF720D9FED3F1F7FDA`.

The first and only code-producing candidate is strict exact. Its frozen object
is
`build/audit/player_control_signed_angular_difference_first_shot_20260827.obj`,
SHA-256
`2150DF37155672DE1E054DBDA91755F7D84FDF59FC93A810C447352A34E84151`.
The hardened comparator proves all 64 padded bytes and all four relocations
exact in address, type, identity, addend, and ownership. All 37 inherited
exact functions remain exact; `lost_exact` is empty and the sole
`newly_exact` entry is `_signed_angular_difference`.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,710 code bytes, 4,252 functions, 1,836,778 data
  bytes;
- Halo: 275/468, 498,796 code bytes, 4,085 functions, 1,831,586 data bytes;
- semantic audit: 4,363 evaluated, 4,218 semantic exact, 4,278 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the one-parameter-per-line and explicit return house
rules, uses ordinary C89 source, and preserves typed math access. No protected
source, assembly, intrinsic, raw address, pointer pun, barrier, exception,
compiler substitution, or object patch was used.
