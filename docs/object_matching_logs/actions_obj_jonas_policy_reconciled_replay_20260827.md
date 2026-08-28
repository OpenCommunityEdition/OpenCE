# `actions.obj` policy-reconciled replay (2026-08-27)

## Result

The preserved Actions reconstruction was replayed into the integration tree and
reconciled with the newer shared headers and house rules.

- strict functions: **0/63 -> 55/63**
- strict padded function bytes: **0 -> 11,392**
- unwritten owners: **62 -> 0**
- residual owners: **1 -> 8**
- `_global_action_functions`: **888 bytes / 96 relocations, exact**
  - normalized SHA-256:
    `b4bd37bc3750a724b625c0254a2b8bc3272c1aff70c71ae46952bac6acd4a9ab`
- `_global_actor_mode_names`: **16 bytes / 3 relocations, exact**
  - normalized SHA-256:
    `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb`

The strict project board after a forced 569-object rebuild is:

```text
objects 270/619
functions 4445/8246
bytes 592087/1922669
```

The forced rebuild exposed eight additional exact `units.obj` functions from
already-committed Units source.  Those 5,808 bytes are not credited to this
Actions replay.  Actions itself contributes 55 functions and 11,392 bytes.

## Provenance read before integration

- donor tree:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\actions-full-20260813`
- preserved Claude ledger:
  `docs/object_matching_logs/actions_obj.md`
- preserved Codex closure ledger:
  `docs/object_matching_logs/actions_obj_codex_closure.md`
- historical integration precedent:
  `66112c6181b2e882e3382e739aaeb528606d4285`
- house rules source:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\units-complete-20260824\docs\house_rules.md`
- house rules SHA-256:
  `9294B81DC2E1C92D75FFD2F9F289287131BDBA3564BA95BB054B08189781BFE1`

The two preserved ledgers remain verbatim.  This file records only the
integration-specific reconciliation and verification.

## Policy and header reconciliation

The donor's raw byte-offset accessors and private cast overlays were not copied
into production.  They were replaced with typed source constructs whose layouts
are compile-time pinned:

- `prop_datum` reads use the canonical `enemy` and `dead` members at `+0x60`
  and `+0x127`.
- `actor_datum` movement reads use `control.moving` at `+0x504`.
- the donor's `unit_definition + 0x238` access was identified as
  `unit.dive_distance`, not `unit.evade_distance`; the nested offset sum is
  asserted as `0x238`.
- the two vehicle AI values use a TU-local completion of the already-declared
  `struct vehicle_definition`, returned directly by the typed
  `vehicle_specific_definition_get` macro.  Their offsets are pinned at
  `0x390` and `0x394`.
- debug writes use the canonical `struct actor_debug_info`.  The mapped cover,
  grenade, danger, and dive clusters are pinned at `0xB8`, `0x150`, `0x168`,
  and `0x184`; the complete record is pinned at `0x657C`.
- the image-attested `ai_debug.flee_always` byte at `+0x8` replaces the donor's
  incompatible private view without changing `struct ai_debug_state` size.
- the Actions view of `ai_globals` pins `grenades_enabled` at `0x3B4`, the
  enterable-vehicle array at `0x3B8`, and the complete local prefix at `0x8B8`.

The newer Units lane intentionally declares the animation-impulse argument as
`long`.  Keeping the reconstructed evade animation local as `long` preserves
January's full-register call shape and the exact 464-byte
`actor_action_try_to_evade` owner without changing Claude's active Units header.

All reconstructed declarations use one parameter per line, every no-argument
list places `void` on its own line, and every reconstructed `void` definition
has an explicit terminal `return;`.

## Strict residual frontier

```text
1776  actor_action_handle_combat_selection
1520  actor_action_handle_danger_avoidance
 800  actor_action_handle_evasion
1872  actor_action_handle_lost_contact
 960  actor_action_handle_vehicle_entry
 720  actor_action_try_to_dive       (candidate 704)
 320  actors_searching_same_position
 368  code_0000b3b0
```

No residual is credited.  The preserved donor ledger contains the exhaustive
negative matrices and the best measured shapes for these eight owners.

## Verification

```text
build\local-tools\ninja.exe -t clean all_source
build\local-tools\ninja.exe all_source
python -B tools/campaign/verdicts.py scratch/actions_replay_after.json
python -B tools/campaign/verdicts.py --diff \
  scratch/actions_replay_before.json scratch/actions_replay_after.json
python -B tools/campaign/gate.py source/ai/actions
python -B tools/campaign/gate.py source/ai/ai_debug
python -B tools/campaign/gate.py source/ai/ai
python -B tools/campaign/gate.py source/units/units
python -B tools/campaign/board.py
```

The before/after verdict diff reports **63 gains and 0 regressions**.  Actions
gates at **55 exact / 8 residual / 0 unwritten**.  `ai.obj` remains 13 exact
with no residual owners, `ai_debug.obj` retains all 11 prior exact owners, and
`units.obj` gates at its committed **183 exact / 6 residual / 0 unwritten**.
