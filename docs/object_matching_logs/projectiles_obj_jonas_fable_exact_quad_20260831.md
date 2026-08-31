# `projectiles.obj` Fable exact-only aiming quartet — 2026-08-31

## Outcome

This bounded wave starts from canonical commit
`af9f192610807e071b4b9f47059a3adb9d2e071e`. It admits four previously
unwritten, semantically named functions and retains all thirteen inherited
strict-exact functions:

- `_projectile_estimate_time_to_target`;
- `_random_vector_in_cone3d`;
- `_projectile_aim_linear`;
- `_projectile_aim`.

The focused hardened gate advances `source/items/projectiles.obj` from
**13 exact / 0 residual / 17 unwritten** to
**17 exact / 0 residual / 13 unwritten**. The gain is 445 meaningful and 480
padded code bytes. No inherited exact function is lost or changed.

`_projectile_aim_ballistic` is deliberately absent. The Fable reconstruction
of that 880-byte function remains nonexact (`36` candidate relocations versus
`35` target relocations, with a normalized-byte mismatch), so neither its body
nor its supporting assertions/constants is admitted. The two proposed trailing
fields in `struct _projectile_datum` are also absent; the Fable session proved
that adding them regresses `units.obj` from 189/189 to 188/189.

No configuration, symbol map, completion label, semantic exception, parked
record, generated report, or unrelated source is changed.

## Provenance and authenticated contracts

The exact PC source forms were recovered in Fable campaign commit
`e02971dfdba111d5481c5b7d7b6fc38a9e90d155`; its complete
`source/items/projectiles.c` blob is
`3608d6b24f30d8ece41c3ee5f819eaf59d51f556`. This wave selects only the four
strict bodies and their required translation-unit support from that donor.

The independent semantic oracle is HCEA commit
`c168af2e747d3095d9a29418ae401f3a39544863`. The relevant donor blobs are:

| Contract | HCEA path | Git blob |
| --- | --- | --- |
| time estimate | `src/projectile_estimate_time_to_target.c` | `542cc66ed018e457d0b6f1d794092ce07084b0b1` |
| global-seed cone wrapper | `src/random_vector_in_cone3d.c` | `49acdb9cfadb08271d8fa3970c80a17e6a5f43ce` |
| linear aim | `src/projectile_aim_linear.c` | `b05e1a278f0bbb7fc72cde26b31f1c2164ff1485` |
| solver selection | `src/projectile_aim.c` | `a0b9880a68efd6081b2d8863b29a2b4016ee54a5` |
| definition flags | `src/headers/projectile_definition_flags.h` | `e2795bbbf833d6ec9944ef4f1cf6f08b7891ef6e` |

HCEA is a semantic oracle rather than a PC byte donor. Its PowerPC bodies
independently confirm the signatures, typed projectile-definition fields,
optional outputs, global random-seed wrapper, and ballistic-versus-linear
selection. January's own PC object remains authoritative for instruction and
relocation topology.

The flag-header oracle states that its enum names are database-verified. The
translation-unit enum therefore preserves its spellings
`_projectile_detonation_max_time_if_attached_bit` and
`_projectile_minimum_unattached_detonation_time`. It intentionally does not
copy Fable's unauthenticated `maximum` expansion or added `_bit` suffix. Only
the authenticated `_projectile_aim_ballistic_bit == 1` member participates in
emitted code, through `TEST_FLAG`; no manual bitwise test or numeric flag is
used.

All four function names already exist in the January PDB-derived
`config/symbols.json`; this wave requires no address alias or symbol rename.

## January-plausible inline schedule

Two current shared-header inline definitions conflict with January's proven
ownership and call graph if included under their public names:

1. January `projectiles.obj` itself owns the out-of-line
   `_random_vector_in_cone3d` COMDAT at RVA `0xE7860`.
2. January `_projectile_aim_linear` contains a `REL32 _normalize3d` call at
   function offset `+0x2A`; it does not inline normalization there.

The translation unit therefore remaps the two shared-header definitions while
including `cseries.h` and `projectiles.h`, then restores the public spellings:

```c
#define normalize3d normalize3d_inline
#define random_vector_in_cone3d random_vector_in_cone3d_inline
/* shared headers */
#undef normalize3d
#undef random_vector_in_cone3d
```

After the restore, the source supplies the typed external declaration required
by January's `_normalize3d` call and an ordinary out-of-line definition for
`random_vector_in_cone3d`. This is a per-TU reconciliation of the reconstructed
shared header with January's observable symbol topology, not an artificial
caller, dead branch, forced-inline annotation, or compiler directive.

The final COFF census is decisive: the candidate owns exactly seventeen
target-named code sections, has **zero candidate-only code COMDATs**, and emits
neither `normalize3d_inline`, `random_vector_in_cone3d_inline`, nor
`point_from_line3d`. Thus the remapped, unreferenced header bodies do not
materialize.

This supplies the new evidence required to reopen the 2026-08-21 rejection in
`projectiles_obj_jonas_ballistic_leaf_20260821.md`, which had excluded the cone
wrapper because no admissible resolution to the then-observed inline/out-of-
line ownership conflict was available. The later Fable source form, exact
January relocation identities, and zero-extra-owner census resolve that
conflict without fake retention or code-generation tuning.

## Strict function evidence

The hardened comparator requires padded bytes, normalized bytes, relocation
address, type, symbolic destination, and addend to agree.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_projectile_estimate_time_to_target` | 44 / 48 | 2 | `9c587ac970f3b77073e44a755654f4e7f086a3cde1a9d1bc7ef3fb40c82dd079` |
| `_random_vector_in_cone3d` | 35 / 48 | 2 | `14aed32fbbc92eb83753c9183582737959a5f03aeae1a37ac9bef0635bdcb084` |
| `_projectile_aim_linear` | 186 / 192 | 6 | `6506539733f739bbfcff67d1e8415c91740b5c80942191cb01aefa9d06c6d548` |
| `_projectile_aim` | 180 / 192 | 3 | `46be56e656c9e4dddd931548b2304fa50f0979653de1d7ae4031b74609a639e6` |
| **Wave gain** | **445 / 480** | **13** | |

The exact relocation maps are:

- time estimate: `DIR32 __real@00000000` at `+0x08` and `+0x14`;
- cone wrapper: `REL32 _get_global_random_seed_address` at `+0x14` and
  `REL32 _seed_random_vector_in_cone3d` at `+0x1A`;
- linear aim: `REL32 _normalize3d` at `+0x2A`, zero constant at `+0x39`, source
  path at `+0x66`, assertion text at `+0x6B`, `REL32 _display_assert` at
  `+0x70`, and `REL32 _system_exit` at `+0x77`;
- solver selection: zero constant at `+0x2E`,
  `REL32 _projectile_aim_ballistic` at `+0x71`, and
  `REL32 _projectile_aim_linear` at `+0xA1`.

The omitted ballistic body remains an undefined typed external for the exact
selector relocation. No synthetic definition or inactive retention reference
is present.

## Owner prototype and header-consumer A/B

`projectile_estimate_time_to_target` was previously declared locally in
`weapons.c`, away from its owner. This wave forward-declares
`struct projectile_definition`, moves the const-correct public prototype into
`projectiles.h`, includes that owner header from `weapons.c`, and removes the
misplaced local declaration.

Every direct `projectiles.h` consumer was enumerated before the edit:

- `source/items/projectiles.c`;
- `source/cutscene/cinematics.c`;
- `source/game/players.c`;
- `source/units/units.c`.

`weapons.c` was gated separately because it becomes a new consumer. PID-local
before/after objects were compared per target function, not only by aggregate
counts:

| Unit | Before | After | Lost exact | Changed pre-existing emitted target sections |
| --- | --- | --- | ---: | ---: |
| `source/items/projectiles` | 13 exact / 0 residual / 17 unwritten | 17 / 0 / 13 | 0 | 0 |
| `source/cutscene/cinematics` | 15 / 0 / 2 | 15 / 0 / 2 | 0 | 0 |
| `source/game/players` | 52 / 17 / 1 | 52 / 17 / 1 | 0 | 0 |
| `source/units/units` | 189 / 0 / 0 | 189 / 0 / 0 | 0 | 0 |
| `source/items/weapons` | 37 / 2 / 40 | 37 / 2 / 40 | 0 | 0 |

No consumer gains or loses a code owner. `units.obj` remains the required
**189/189 strict-exact** sentinel. Its pre-existing `_point_from_line3d`
helper owner is present identically before and after; the header edit does not
create a new owner. The new `projectiles.obj` candidate itself contains no
`point_from_line3d` owner.

## Data and whole-object ownership

The candidate defines exactly the thirteen inherited exact functions plus the
four functions in this wave. The thirteen still-unwritten target functions are
the eight address-placeholder functions and the five semantic owners
`projectile_accelerate`, `projectile_aim_ballistic`,
`projectile_export_function_values`, `projectile_new`, and
`projectile_update`.

The candidate owns no `.data`, `.bss`, or COMMON storage. Its four runtime
read-only COMDATs are the two inherited assertion strings, the target-owned
`result_aim_vector` assertion string, and the natural select-any
`__real@00000000` constant used by the exact functions. The zero constant is
coalescible support and receives no data credit. The incomplete target's 2,092
data bytes remain unclaimed.

No projectile runtime layout is enlarged. In particular, the offsets and
three compile-time assertions in `projectiles.h` remain unchanged.

## Source-policy and validation record

The retained bodies are ordinary typed C using `real`, `boolean`, canonical
projectile-definition fields, `TEST_FLAG`, `match_assert`, and existing math
and random APIs. They introduce no assembly, raw address or byte-offset access,
address-named function/global, unsafe representation cast, union pun,
`volatile`, `register`, pragma/barrier, forced inline, raw byte emission,
fixed/dead condition, synthetic anchor, or nonsensical write used to influence
code generation.

Validation completed without a global configure or build:

- `tools/campaign/gate.py source/items/projectiles --all`:
  `17 exact / 0 residual / 13 unwritten`;
- hardened comparison of all four new functions: `all_equal: true`;
- whole-object code census: 17 target owners, zero candidate-only code owners,
  no unwanted inline helper, and no `point_from_line3d`;
- exact-set and emitted-section A/B for all header consumers and `weapons.c`:
  no losses and no changed pre-existing target function;
- `tools/fake_match_scan.py` over the three changed source/header files:
  zero findings;
- `python -m pytest tools/test_coff_compare.py tools/test_fake_match_scan.py -q`:
  62 passed;
- `git diff --check`: clean apart from Git's informational LF/CRLF warnings.

The focused candidate object has raw SHA-256
`3D749088D1FBE02F2784AA76AE1704A19CBD7D1805F5BF6BDEB05911B7476889`;
raw COFF hashes are audit artifacts because timestamps vary. Stable source
blob fingerprints before commit are:

- `source/items/projectiles.c`:
  `f91cb92e2d0b1be1dde5a7b4f36c8f866eec8bec`;
- `source/items/projectiles.h`:
  `cd9e53816c7ea279cd077289573f7e474b0c6746`;
- `source/items/weapons.c`:
  `8d62323b273378252b5b179f1916c745eef43a2b`.

The January split `projectiles.obj` SHA-256 is
`0F558B623EFE404E1C803B8FAB03338C4F2B5B12AF8CE14C4634C3AD59D6C8DA`.
The compiler is Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1,
SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`,
using the repository's unchanged XDK 3911 flags.

A full cumulative ninja/board/regression replay and commit are intentionally
left to the integration owner.
