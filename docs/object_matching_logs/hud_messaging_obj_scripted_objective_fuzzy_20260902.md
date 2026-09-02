# `hud_messaging.obj` scripted-objective fuzzy park (2026-09-02)

## Result

Starting from exact checkpoint `fa6c10f2f`, this packet reconstructs the
complete ordinary-C `_scripted_hud_set_objective` body.  The focused gate moves
from **28 exact / 0 residual / 8 unwritten** to **28 exact / 1 residual / 7
unwritten**.  The function is **96.80%** similar in objdiff, with the same
160-byte padded envelope and all eight relocation identities.  It is parked as
an honest fuzzy match; no exact-byte credit is claimed.

| Measurement | January | Candidate |
|---|---:|---:|
| Meaningful / padded bytes | `147 / 160` | `145 / 160` |
| Relocations | `8` | `8` |
| Normalized SHA-256 | `a83e487b78b95cb689279b7de9dfe2a3078ce9e67c5809282fdd7a6b3ebc6a54` | `c3e1fe857ff8074266e9c69b797a29e79f3bc99d6a09fdee025d4d8bd20d9d6e` |

All 28 inherited exact functions remain exact.

## Source and evidence

January proves the short signed parameter, scenario HUD-message lookup,
message/element block calls, single-text-element predicate, objective runtime
stores, timing-field sum, silent error, and eight-relocation order.  The later
HCEA Xbox reconstruction independently preserves those semantics, Stian's PC
lift preserves the error path and target topology, and Aerocatia/demon
authenticates `_hud_message_type_text`, `_hud_message_type_icon`, the two-byte
element record, and the 64-byte message record.

The retained code therefore uses `HUD_MESSAGE_TEXT_DEFINITION_GET` and
`TAG_BLOCK_GET_ELEMENT`, named structures and enum constants, project integer
types, and the HCEA-authenticated `up_ticks`/`fade_ticks` scalar locals.  The
public declaration was moved from `hs.c` into the owning
`interface/hud_messaging.h` with the target-proven signed `short` parameter.
The tag definitions remain translation-unit-private to avoid the documented
VC7 allocator sensitivity of broadly included structure declarations.

## Fixed code-generation boundary

The streams are identical through the successful predicate and agree again at
the error/return structure.  January loads `fade_ticks` through
`[hud_globals+0x11e]`, advances the same base by `0x100`, then adds `up_ticks`
through the shorter `+0x1c` displacement.  VC7 lowers the natural typed scalar
source as reads through `+0x11e` and `+0x11c`, without materializing the
subobject base.  That three-byte instruction-shape difference shifts the later
branches/relocation addresses while preserving every relocation identity.

A coherent local pointer to the complete `objective_color` subobject instead
moved the base adjustment before the objective store and shrank the function
to 144 padded bytes.  It was rejected.  A mixed one-use alias was not admitted
merely to steer the second read.  The fail-closed classifier returns `UNKNOWN`,
so the manifest uses the honest `unclassified` label rather than asserting an
unsupported compiler recipe.  The retained source has no raw offset, address
name, volatile/register forcing, barrier, pragma, assembly, fake dependency,
undefined behavior, forced inlining, or nonsensical branch.

## Validation

- Production `ninja all_source`: pass.
- Fresh direct csplit plus objdiff report: `_scripted_hud_set_objective`
  `96.80%`; object gate `28 / 1 / 7`.
- Park validator: `105 active / 0 stale / 0 invalid`, including this entry.
- Header-consumer gates remain stable:
  - `hs.obj`: `445 exact / 3 residual / 0 unwritten`;
  - `hud.obj`: `19 / 3 / 0`;
  - `players.obj`: `54 / 15 / 1`;
  - `game_state.obj`: `24 / 0 / 0`.
- `compare_messages` remains dead-stripped and unclaimed; no artificial caller
  or anchor was introduced.

Reopen only for authoritative January source/local-variable records or a
natural same-compiler donor that explains the asymmetric subobject-base
lifetime.
