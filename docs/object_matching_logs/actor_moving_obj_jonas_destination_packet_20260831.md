# `actor_moving.obj` destination packet (2026-08-31)

## Result

Five ordinary-C January owners were manually recovered from the audited Claude
campaign packet and corroborated against HCEA commit `570c83f`.  January's
Xbox ABI and enum spellings remain authoritative where that later build
differs.

| Owner | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_actor_test_destination` | 144 | 3 | `d2ca24e4220403819f30510fd8a79530a13f438cbb9c2cfe5564cb6c15b9b043` |
| `_actor_move_to_point` | 304 | 10 | `6317cf2da4898c7ded91f964bc89fa4a1005152e32b6b5a5741e7e3d04fe4e16` |
| `_actor_move_to_move_position` | 176 | 5 | `a77070df499624c408f82cf0cfcd836cd4335b8980824dd8a0d29d4c589ce54b` |
| `_actor_move_to_firing_position` | 176 | 5 | `c1b74a2aabe4f8c31896d47c9e5bf94cdd79a4fb7d0fec315d6fb366aab45ed9` |
| `_actor_move_to_prop` | 240 | 7 | `7e2daf8e35974a5f0a3ef4f9d27224ed04539eef75bfdcf0cad4b4646c20fe41` |

The object advances from 10 exact / 26 unwritten to 15 exact / 21 unwritten,
with no residual owner.  Every row has identical normalized bytes and
relocation identities.

## Interface cleanup

The public declarations now live in `actors.h`.  Wrong or duplicate
consumer-local declarations were removed from `action_guard.c`,
`action_search.c`, and `action_fight.c`.  In particular,
`actor_move_to_point` returns `boolean`, not the foreign `void` declaration,
and the January `actor_move_to_firing_position` third argument remains the
`boolean temporary` interface rather than HCEA's later path-state pointer.

All 41 direct `actors.h` consumers rebuilt.  `action_guard`, `action_search`,
and `action_fight` retain every exact owner, while protected `units.obj`
remains 189/189.

## Deliberate exclusions

`actor_move_halt` and `actor_move_halt_at_firing_position` are not admitted:
Claude's `void` definitions conflict with the byte/boolean HCEA and existing
interface evidence.  The exact-code avoidance-direction candidate is also
excluded because its required angle data is not exact; two constants remain
one ULP from January.  No raw layout, union overlay, forced inline, assembly,
or speculative ABI is used in this packet.
