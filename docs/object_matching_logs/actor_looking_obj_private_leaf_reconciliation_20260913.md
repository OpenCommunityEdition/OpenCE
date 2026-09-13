# `actor_looking.obj` private-leaf reconciliation (2026-09-13)

## Result

This reconciliation keeps exactly **three** strict owners in the ordinary
translation-unit build.  The inherited pair remains unchanged, and the new
`valid_real_normal2d` body contributes **77 meaningful / 80 padded exact code
bytes**.  No other body in this packet receives exact credit.

| January owner | meaningful | padded | relocations | disposition |
| --- | ---: | ---: | ---: | --- |
| `_actor_look_secondary_stop` | 111 | 112 | 7 | inherited exact |
| `_actor_look_compute_prop_interest` | 295 | 304 | 17 | inherited exact |
| `_valid_real_normal2d` | 77 | 80 | 2 | new exact |
| **ordinary total** | **483** | **496** | **26** | **3 strict owners** |

The focused gate reports `exact 3, residual 2, unwritten 11`.  The two
residuals are the already documented fuzzy `actor_look_decode_direction` and
`actor_look_affect_movement` bodies.  The latter two remain zero-credit parks;
this packet does not change their source or classification.

## Authentic target-name map

The remaining anonymous January owners are renamed in `config/symbols.json`
from address-derived placeholders to their semantic private names:

| January offset | old target name | recovered name | meaningful / padded |
| ---: | --- | --- | ---: |
| `0x16020` | `_code_00016020` | `actor_look_get_looking_definition` | 71 / 80 |
| `0x163E0` | `_code_000163e0` | `actor_look_valid_aim_vector` | 124 / 128 |
| `0x16460` | `_code_00016460` | `actor_look_valid_look_vector` | 233 / 240 |
| `0x16600` | `_code_00016600` | `actor_look_idle_find_prop` | 602 / 608 |
| `0x16860` | `_code_00016860` | `actor_look_idle_timer` | 351 / 352 |
| `0x169C0` | `_code_000169c0` | `actor_look_find_random_vector` | 595 / 608 |
| `0x172D0` | `_code_000172d0` | `actor_look_idle_new_major_direction` | 523 / 528 |
| `0x174E0` | `_code_000174e0` | `actor_look_idle_new_minor_direction` | 362 / 368 |

All eight mappings are exact-name entries in the local Halo CE symbol atlas,
whose SHA-256 is
`851802ac508bbcf7cd2896c6cfddd1e0f81510b7b9ef9ea02f39f913c123578e`.
The atlas records the three plain-C identities and the later-build C++ names
for the two validity helpers; those mangled spellings corroborate identity,
not January source-language linkage.  The current TU and January relocation
graph establish that these are same-TU implementation details, so the target
entries and the reconstructed definitions use private linkage.

The source-file symbol inventory is updated with the same semantic names.
No source identifier in this packet is an address-derived `code + address`
name.

## Five retained source-only reconstructions

The dirty Claude intake also contained coherent typed reconstructions for:

- `actor_look_get_looking_definition`;
- `actor_look_valid_aim_vector`;
- `actor_look_valid_look_vector`;
- `actor_look_idle_timer`;
- `actor_look_find_random_vector`.

Their corresponding January scope is **1,374 meaningful / 1,408 padded
bytes**, but the ordinary candidate emits none of the five.  Their genuine
callers (`actor_looking_test_validity`, the idle-direction helpers, and
`actor_look_update`) remain unwritten, and VC7 eliminates the presently
unreferenced private definitions.  They are therefore retained as useful
source-only fuzzy progress with **zero exact credit**.

A diagnostic compile forced the five functions to emit first by making them
external, then by taking their addresses.  Both experiments changed VC7's
private calling-convention decision: for example, January's 80-byte
`actor_look_get_looking_definition` receives its actor index in `EAX`, while
the forced candidate gained a cdecl frame and stack argument.  Those objects
are not admissible comparisons and are not used for matching credit.  Adding
an anchor, externalizing the functions, forcing inlining, or inventing a
caller would be fake source.  Strict classification must wait for the real
caller graph.

The reconstruction remains semantically grounded by the HCEA function files.
Pinned SHA-256 values are:

| HCEA evidence file | SHA-256 |
| --- | --- |
| `actor_look_get_looking_definition.c` | `9c0abe0007d82a773199110f457e1e83ed9513fef1d9a4e15a63f4aeb5a4db8b` |
| `actor_look_valid_aim_vector.c` | `4fce8f5a792e2e4d455db12f0ad0239826d84512ee989b6862bfc4b6590c6899` |
| `actor_look_valid_look_vector.c` | `5c726ba3f70e1aa479d5d0a88678dbe2e776d25c1fddc8e21ef1b6c527c12c94` |
| `actor_look_idle_timer.c` | `d24d39b586ba89c6dcedfb1e066b150fc41a219be63d8e8ec0ab27af66dded14` |
| `actor_look_find_random_vector.c` | `bfa6bcca5cd760c0f8c31e00ca7a4858086a92d1879cf65688699f22cf72d1d3` |

January remains authoritative where the later PPC build differs.  In
particular, January proves the idle-timer diagnostic branch and the AI
collision profiling/user-stack operations in the random-vector helper.

## Source-quality and compiler boundary

The timer selector now uses the authenticated `_idle_timer_facing`,
`_idle_timer_aiming`, and `_idle_timer_looking` constants instead of numeric
switch labels.  The retained bodies use project `real`/vector types, typed
actor/tag accessors, named collision flags/users, and normal C control flow.
There is no inline assembly, UB-based representation trick, `volatile`,
compiler barrier, forced inline/noinline annotation, or manual byte emission.

`actor_looking_test_validity` is deliberately not reconstructed here.  It
requires the genuine shared `_actor_mode_combat` owner.  The earlier bounded
owner trial documented in
`actor_looking_obj_validity_owner_boundary_20260906.md` perturbed a protected
Units function, and duplicating the constant locally would violate enum
ownership and the house rules.

## Reproducible checks

The ordinary focused gate command is:

```powershell
python tools/campaign/gate.py source/ai/actor_looking --fn _actor_look_secondary_stop --fn _actor_look_compute_prop_interest --fn _valid_real_normal2d --fn _actor_look_affect_movement --fn _actor_look_decode_direction --out scratch/actor-looking-final-gate.obj --forbid-emitted-symbol _point_from_line3d
```

It reports all three listed exact sections, the two known residuals, and a
passed forbidden-emitted-symbol guard.  The three normalized target hashes
are respectively:

- `32ede7c321e2b80f80155ee4ea993670fb1c9ec148e2458760ae9ecc0f7d7c7f`;
- `33b0547c0151f3f5dc6117e59e8cb81060c5f431171fd4685a533436006cb714`;
- `4d7f1b722651caf913f8e1429c3dfdfe107804abd2e92c8ebcef0b21f8f4a0f`.

`tools/fake_match_scan.py source/ai/actor_looking.c` reports zero review
leads, and `git diff --check` passes for the source, symbol map, and this
ledger.  No global configure, Ninja build, commit, or push was performed in
this bounded reconciliation.
