# `player_control.obj` 2D vector-limit recovery (2026-08-27)

## Scope and source evidence

This wave adds only `_limit2d` and its translation-unit-local inline-wrapper
rename in `source/game/player_control.c`. The object advances from 38/49 to
39/49 strict-exact functions and remains `NonMatching`.

The object-ledger census and baseline symbol table contain no earlier
out-of-line owner or production candidate for this function. The repository
inline and the HCEA reconstruction at
`build/audit/refs/halocea/src/blam/math/limit2d.c` establish the typed
operation: compare squared magnitude with squared length, scale both vector
components by `length / sqrt(magnitude_squared)` only when over the limit, and
return whether scaling occurred. January remains the byte and floating-point
predicate authority.

The pre-edit source blob was `9b7d853f2dbbdd45d7f0b6ed38bf5a79f4035c7e`;
the retained source blob is `464fc6293b2a31976d620a8add483990c215d8b5`.

## Frozen target and predicate discriminator

The January target is 69 meaningful bytes / 80 padded bytes, has zero
relocations, and has normalized SHA-256
`e636786252a58ac9d0fa96fe2eac4dc2a08749e2de1808e0a14250e7fdbdc2ff`.
The target object SHA-256 is
`7B282D76BDC4B571E5FAB060391800565B94B134124742484A0F6473106C1BDC`.

The frozen baseline is
`build/audit/player_control_limit2d_baseline_20260827.obj`, SHA-256
`2150DF37155672DE1E054DBDA91755F7D84FDF59FC93A810C447352A34E84151`.

The first natural candidate copied the header's early-return spelling,
`if (dot <= length * length) return FALSE`. It preserved the target's 80-byte
size and zero relocations but was nonexact, normalized SHA-256
`f98ac900129abc82bb6bf08900fa5702b6013162036892fe742583bed95c92dc`.
Its frozen object is
`build/audit/player_control_limit2d_first_shot_20260827.obj`, SHA-256
`67F496A26985FAFE13C785F97BBFB743D695E64BFDD3D0D6C19BEED3876B75DE`.

That artifact exposed a semantic, target-visible discriminator. The header
spelling emits `test ah, 0x41; jp` and treats an unordered comparison as the
scaling path. January emits `test ah, 0x41; jne` and sends unordered values to
`FALSE`. This proves the original PC source used the direct positive topology,
`if (dot > length * length) { scale; return TRUE; } return FALSE;`.

The one evidence-corrected candidate is strict exact. Its frozen object is
`build/audit/player_control_limit2d_branch_corrected_20260827.obj`, SHA-256
`C54564FDAE0065B66390AC6F34D0C898B1B821C256CB8147CF7A7EF4901A058B`.
The hardened comparator proves all 80 padded bytes exact and confirms zero
relocations. All 38 inherited exact functions remain exact; `lost_exact` is
empty and the sole `newly_exact` entry is `_limit2d`.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,779 code bytes, 4,253 functions, 1,836,778 data
  bytes;
- Halo: 275/468, 498,865 code bytes, 4,086 functions, 1,831,586 data bytes;
- semantic audit: 4,364 evaluated, 4,219 semantic exact, 4,279 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the one-parameter-per-line and explicit return house
rules and uses ordinary typed C89 math. No protected source, assembly,
intrinsic, raw address, pointer pun, barrier, exception, compiler substitution,
or object patch was used.
