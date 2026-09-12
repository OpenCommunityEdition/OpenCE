# `unit_dialogue.obj` unwritten speech packet (2026-09-12)

## Result

This packet reconstructs the four previously unwritten speech and damage-sound
routines in `source/units/unit_dialogue.c`.  One routine is newly strict exact:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_unit_scream` | 384 | 384 | 24 | `e942ba201054ab307a1ba718ee57bf9e9529e78f803fa3335ca37945249eb66b` |

The strict exact gain is therefore **384 meaningful / 384 padded bytes** and
one function.  The object moves from 8 to 9 strict exact functions.  All eight
inherited exact functions remain exact, and the forbidden
`_point_from_line3d` symbol is absent.

## Retained zero-credit reconstructions

The remaining three bodies are complete, ordinary-C semantic reconstructions.
They are retained because they replace empty owners with useful and credible
research baselines.  None receives exact-match credit.

| Function | Target meaningful / padded | Candidate meaningful / padded | Relocations (target / candidate) | Objdiff | Target normalized SHA-256 | Candidate normalized SHA-256 | Credit |
| --- | ---: | ---: | ---: | ---: | --- | --- | ---: |
| `_unit_describe_speech` | 265 / 272 | 267 / 272 | 15 / 15 | 97.23% | `7d9ee7e3b923460ae26a00c0612904b3106b6ed95c8c0491504d017841c1dc9c` | `2b01a81debf36cda38bd965419e48997b2a2bbeac2dd4041f6752591f481edaf` | 0 |
| `_unit_notify_impulse_sound` | 230 / 240 | 231 / 240 | 10 / 10 | 84.41% | `ff46a3c5213b3d65bc19da918a094f3aba00c56dbc017a97f1e646a06df18c81` | `3582e4a63aa482d79f813af47a217c92370b47c7f9fa3221eef7f9fb1d97c7d7` | 0 |
| `_unit_make_damage_sound` | 776 / 784 | 777 / 784 | 18 / 18 | 81.61% | `87952366bca787dc5174221b78335b6a9e91770433b8d76266bdb21d1bf65975` | `a72cd2b9285abc4fe3ed3a8f4de6c0606fe474c9e01a1049e16b3285f1497f01` | 0 |

All three retain the target's relocation-destination sequence.  Their
remaining differences are instruction scheduling, register allocation, and
minor branch layout, not missing calls or invented behavior.

## Semantic evidence

The January target disassembly is authoritative.  HCEA was used only as a
semantic cross-check:

- `unit_describe_speech` formats the current vocalization and sound tag name,
  with the target's distinct abbreviated and full-path handling;
- `unit_notify_impulse_sound` arbitrates scripted speech, queues one 48-byte
  speech item, verifies the selected sound, records the impulse handle, and
  starts the AI communication notification;
- `unit_make_damage_sound` selects pain or death vocalizations from damage
  category, severity, actor combat status, and cooldown state, then queues one
  speech item and raises the death-scream AI effect when appropriate;
- `unit_scream` validates the six scream modes, uses the correct enum mapping
  (including the grenade-attached random choice), resolves the typed dialogue
  tag, arbitrates speech, and queues one initialized item.

Two HCEA divergences were deliberately rejected.  January suppresses wounded
sounds through `ai_debug.disable_wounded_sounds`, which the HCEA body omitted,
and January clears exactly one 48-byte speech item rather than HCEA's
three-item/144-byte buffer.  The January target also has no HCEA bridge call at
the end of `unit_notify_impulse_sound`.

The source uses `unit_get`, `actor_get`, `dialogue_definition_get`, and
`damage_effect_definition_get` rather than raw typed-access casts.  Constants
are represented by locally named enums instead of unexplained literals.  No
address-named owner, synthetic anchor, forced inline, `volatile` scheduling
device, register declaration, assembly, raw byte emission, or fake-match
construct is introduced.

## Validation

- Focused hardened object gate: 9 exact, 5 residual, 0 unwritten.
- Inherited exact regression count: zero.
- `_point_from_line3d` emitted-symbol guard: pass.
- Function shapes measured directly from the gated scratch COFF object.
- Focused fake-match scan: zero review leads.
- `git diff --check`: pass (line-ending warning only).

The root integration moved the two missing public declarations to `units.h`
and removed the consumer-local declarations from `units.c` and
`object_types.c`. A full consumer rebuild preserves all 189 January functions
in `units.obj`, and the repository-wide stable sweep reports zero regressions.
The three remaining speech bodies are recorded as zero-credit parks; aggregate
publication evidence is in the sixth 10K batch ledger.
