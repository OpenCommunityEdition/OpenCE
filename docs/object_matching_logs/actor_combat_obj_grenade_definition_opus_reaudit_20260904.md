# Actor Combat grenade-definition helper Opus re-audit (2026-09-04)

## Result

The original interrupted Opus donor contains one genuine missed January
function in `actor_combat.c`. The scratch replay advanced the focused gate
from **18 exact / 6 residual / 10 unwritten** to
**19 exact / 6 residual / 9 unwritten**, preserving all 18 inherited exact
owners. The orchestrator subsequently replayed the same source-authentic
packet into production and regenerated the full report. The helper remains
strict exact there. One inherited residual also changed and improved; its
measurement is recorded below instead of being hidden behind the aggregate
gate count.

| January owner | Semantic name | Padded / relocs | Normalized SHA-256 |
| --- | --- | ---: | --- |
| `_code_0000f920` | `actor_get_grenade_definition` | 64 / 3 | `a2798c426038b5d880d0da621da0366d94815948d8d3bcc3a81683d759658c85` |

The three ordered relocations are REL32 calls to
`scenario_get_game_globals`, `tag_block_get_element_with_size`, and `tag_get`.
The candidate and January section descriptions are strictly equal.

## Why the earlier rejection is superseded

`actor_combat_obj_opus_dirty_cleanup_20260904.md` says the donor abstraction
has no January function section. That statement is incorrect: January has the
64-byte section at file offset 63776 / address `0x0000F920`, and the frozen
Opus object has a byte- and relocation-identical storage-class-3 owner.

The helper is not retained by false public linkage or an anchor. Its two real
same-TU consumers are `actor_combat_reaim_grenade` and the already exact
`actor_combat_build_grenade_trajectory`. VC7 naturally inlines the calls while
also emitting the ordinary static out-of-line owner. The scratch replay keeps
`actor_combat_build_grenade_trajectory` exact. It also naturally improves the
measurement of `actor_combat_reaim_grenade`; that collateral change makes the
old park stale and is detailed in the next section.

## Collateral residual improvement and required park refresh

`actor_combat_reaim_grenade` remains a residual, not a new exact function, but
the replay changes its normalized bytes and raises its generated-report score.

| State | Padded / relocs / instructions | Normalized SHA-256 | Objdiff |
| --- | ---: | --- | ---: |
| January | 448 / 16 / 162 | `8d2e09aa4f73bc982c4cf17d6e4f9a963ffd7db0b486ff7f037d19966ef52772` | 100% |
| Pre-replay canonical | 448 / 16 / 162 | `b2a1aa5cad19d1cfaa1f0cf99729c8edd10f30854f973029e38af9b104e2c54c` | 98.509674% |
| Production replay | 448 / 16 / 162 | `180b57e66e87ba40a0ed66e202278c097623976af9e7a41da86825a5d889a6c6` | 99.86452% |

All 16 relocation destinations, types, and addresses remain in January order.
A relocation-normalized byte comparison reduces the raw difference from 28
bytes before the replay to 21 bytes afterward. The aligned instruction diff
shows exactly what the helper reconstruction fixes: after the inlined
`tag_get`, the pre-replay build stored `eax` in `edi` before reclaiming the
two argument slots, whereas January and the replay reclaim the slots first
and then store `eax`. This also restores January's two null-path branch
targets from `+0x6e` to `+0x6a`.

The change is source-credible rather than a statement-order probe. The caller
now invokes the genuine same-TU `actor_get_grenade_definition` abstraction,
whose ordinary static owner is independently present in January and whose
body is HCEA-corroborated. It also scopes `aim_gravity` to the only branch that
uses it. No source was altered to chase the residual score. The remaining
aligned differences are the existing four-byte frame-size/stack-slot coloring
(`sub esp,0x30` versus January's `sub esp,0x2c`) and the corresponding x87
temporary placement/order in the velocity block. Calls, branches, arithmetic,
stores, and relocation topology otherwise reconverge.

The production park for `_actor_combat_reaim_grenade` therefore must replace
the old base hash and 98.509674% measurement with the replay hash and
99.86452% and retain the authoritative-source reopening boundary. The reviewer
considers `register-allocation` defensible; the orchestrator conservatively
records `unclassified` because the aligned residual also includes temporary
instruction placement/order. Neither label authorizes source tuning.
Refreshing this evidence is
accounting for a real natural improvement; it grants no exact credit and does
not claim whole-object completion.

## Source credibility and name

The body obtains the typed grenade entry from
`scenario_get_game_globals()->grenades`, returns null for a missing projectile
tag, and otherwise uses `projectile_definition_get`. It uses existing tag
block and typed tag-access macros, project integer types, ordinary static
linkage, and an explicit terminal return. The adjacent caller cleanup narrows
`aim_gravity` to the branch where it is used. There is no raw offset, address
identifier, representation pun, forced inline, volatile/register device,
assembly, fake dependency, or nonsensical coincidental logic.

The Opus spelling
`actor_combat_get_grenade_projectile_definition` is descriptive but not the
best recovered semantic name. Pinned HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains
`src/blam/ai/actor_get_grenade_definition.c` with the same short parameter,
return type, tag-block lookup, and projectile-tag result. The proposed private
name is therefore `actor_get_grenade_definition`. HCEA is corroboration for
the name and semantics; January COFF remains the byte authority, and no claim
is made that a January PDB exposes this private spelling.

## Complete emitted-owner boundary

The candidate adds exactly the one 64-byte target owner over the baseline.
It emits no `_point_from_line3d` symbol. The four candidate-only math owners
`square_root`, `actor_combat_magnitude_squared3d_inline`, `dot_product3d`, and
`cross_product3d` are inherited byte-for-byte from the canonical baseline;
this packet neither introduces nor waives that pre-existing whole-object
boundary. No whole-`actor_combat.obj` Matching claim is made.

## Reproduction evidence

- Pre-replay canonical source SHA-256:
  `70548752F51F2D822E0F087018A11C4DD6A3952CEAFFEA97503CD07DB6F4A53A`.
- Final production source SHA-256:
  `6CA4D6545ABEAD50BFF82334F3B52A52736072BA34B624E6C2B97F66682C0B3A`.
- Regenerated production object SHA-256:
  `46A8A8D969E2D9D9F476108B8F560C164D48D8A257553656420B152605A88612`.
- Read-only Opus source SHA-256:
  `4FBBCB80ECA2B83974D88E4B10F0AF9FCE7C4EA31DB7D5BF101FE559A31AF7C7`.
- Baseline scratch object:
  `scratch/actor-combat-grenade-helper-baseline-20260904.obj`, SHA-256
  `82C06C508543EDDF1E9A5A2292C026122395F3EF32E79FCE3CFD466F87936494`.
- Aliased replay object:
  `scratch/actor-combat-grenade-helper-opus-replay-20260904.obj`, SHA-256
  `318B8A7B10155C4FD76F61762A79C013A7A76937876CA919D4535A9728C87BD1`.

The scratch replay used the frozen donor source only as an alternate gate input and
the diagnostic alias
`actor_combat_get_grenade_projectile_definition=code_0000f920`. A production
admission uses the semantic `actor_get_grenade_definition` spelling and maps
the January owner as static. The orchestrator's regenerated production report
records `_actor_combat_reaim_grenade` at 441 meaningful bytes and 99.86452%.
The full emitted-symbol census and stable whole-tree regression sweep remain
the admission authority; this ledger does not replace them.
