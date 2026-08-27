# `game.obj` random-seed setter recovery (2026-08-27)

## Scope and new evidence

This wave adds only the natural out-of-line `_set_random_seed` owner and a
translation-unit-local inline-wrapper rename in `source/game/game.c`.
`game.obj` advances from 7/27 to 8/27 strict-exact functions and remains
`NonMatching`.

The earlier typed-continuation ledger excluded this function before compilation
because `game.h` includes the canonical `__inline` definition from
`math/real_math.h`. Since that audit, this campaign has established the ordinary
source wrapper pattern: rename an inline only while including its header, then
emit the required out-of-line function under its target name. This avoids a
shared-header semantic change and is genuine new evidence, not a retry of an
old candidate.

The canonical inline body is exactly:

```c
*get_global_random_seed_address() = seed;
```

The HCEA reconstruction at
`C:\halo-worktrees\claude-ai-conversation-2\src\halo\game\game.c`
independently confirms that operation. The previously reviewed HCEA PDB records
the public parameter as an unsigned long. January remains the byte, relocation,
padding, and linkage authority.

The pre-edit source blob was `22040601a6eec509ed822e9420089e36f4e29e75`;
the retained source blob is `bc81043cbf95809c4c55411b00125be0f256a25f`.

## Frozen target and first shot

The January target is 15 meaningful bytes / 16 padded bytes, with one `REL32`
relocation at `+0x04` to `_get_global_random_seed_address` and normalized
SHA-256
`f7ba85f0d67393fb3bd1d5d14579b95240091a0c66ab2190900fdb24f199239a`.
The target object SHA-256 is
`FD703BA666F13A5F1311DFBE1383EBA452CF49A0A9A7FAB47B1CFBFD06C1CFD0`.

The frozen baseline is
`build/audit/game_set_random_seed_baseline_20260827.obj`, SHA-256
`0310A3F3A7CF70CC3D268BA9577EAB4ACE7274B79F10DDB81C10E6F62CA203D2`.

The first and only code-producing candidate is strict exact. Its frozen object
is `build/audit/game_set_random_seed_first_shot_20260827.obj`, SHA-256
`BBC24C85EAD2E33A42F27DD09E18C24E5233F0BA84A9EC73B8783D5E33948CBA`.
The hardened comparator proves all 16 padded bytes and the relocation exact in
address, type, identity, addend, and ownership. All seven inherited exact
functions remain exact; `lost_exact` is empty and the sole `newly_exact` entry
is `_set_random_seed`.

## Repository-wide gates

The normal full Ninja build completed and reported:

- all objects: 377/833, 511,661 code bytes, 4,251 functions, 1,836,778 data
  bytes;
- Halo: 275/468, 498,747 code bytes, 4,084 functions, 1,831,586 data bytes;
- semantic audit: 4,362 evaluated, 4,217 semantic exact, 4,277 accepted,
  zero errors.

Admission is 0 candidates / 0 contradicted / 0 revoked. Parked evidence is
3 active / 0 stale / 0 invalid. All 179 tests pass; the only warning is the
existing sandbox-denied pytest cache write.

The function follows the one-parameter-per-line and explicit terminal-return
house rules. It uses the typed seed accessor and ordinary C89 source. No
protected source, assembly, intrinsic, raw address, pointer pun, barrier,
exception, compiler substitution, or object patch was used.
