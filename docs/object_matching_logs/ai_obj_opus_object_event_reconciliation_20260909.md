# `ai.obj` Opus object/event reconciliation (2026-09-09)

## Scope and result

This packet was independently reconstructed from the completed Opus donor
`C:\halo-worktrees\opus-small-families-30k-20260902` at
`eeedd72e5ba0fc1761519f68cad241fe606320e7`, on canonical base
`105d9acaacda589e21a4389d7e6bd69db0b42aba`.

The strict per-function gate advances `source/ai/ai.obj` from **14 to 36 exact
target functions** with **zero inherited exact-function regressions**. The 22
newly admitted functions total **4,973 meaningful code bytes** and **5,120
padded code bytes**.

| Function | Meaningful | Padded |
| --- | ---: | ---: |
| `ai_adjust_damage` | 105 | 112 |
| `ai_consider_major_upgrade` | 364 | 368 |
| `ai_create_mounted_weapons_for_unit` | 99 | 112 |
| `ai_enemies_endanger_player` | 515 | 528 |
| `ai_find_inactive_encounters` | 330 | 336 |
| `ai_flush_spatial_effects` | 54 | 64 |
| `ai_get_major_upgrade_chance` | 176 | 176 |
| `ai_get_responsible_unit` | 119 | 128 |
| `ai_handle_allegiance_broken_notification` | 294 | 304 |
| `ai_handle_allegiance_status_changed` | 307 | 320 |
| `ai_handle_bump` | 245 | 256 |
| `ai_handle_damage` | 270 | 272 |
| `ai_handle_death` | 139 | 144 |
| `ai_handle_deleted_object` | 349 | 352 |
| `ai_handle_killing_spree` | 194 | 208 |
| `ai_handle_unit_effect` | 320 | 320 |
| `ai_initialize` | 127 | 128 |
| `ai_initialize_for_new_map` | 234 | 240 |
| `ai_reconnect_to_structure_bsp` | 186 | 192 |
| `ai_release_inactive_encounters` | 301 | 304 |
| `ai_release_inactive_swarms` | 186 | 192 |
| `compare_potentially_releasable_entities` | 59 | 64 |

The canonical symbol map now gives the three previously anonymous target
owners semantic names:

- `code_0002e110` -> `compare_potentially_releasable_entities`;
- `code_0002f050` -> `ai_flush_spatial_effects`;
- `code_000309a0` -> `ai_enemies_endanger_player`.

The January COFF marks all three as external definitions. The reconciled
candidate preserves that storage class rather than forcing a guessed `static`
classification. All 22 admitted definitions match January in function type and
COFF storage class in addition to padded bytes, normalized code, and relocation
identity.

## Source and ownership audit

- The reconstructed AI-global view has named fields and layout assertions; it
  contains no `bss_<address>` placeholder.
- All imported functions use semantic names; no `code_<address>` source
  function was admitted.
- Public declarations remain with their subsystem owners. AI-internal lifecycle,
  event, and actor-helper declarations used across implementation files live in
  the existing narrow `ai_runtime.h`; `game.c` and `units.c` now include that
  owner instead of repeating those declarations locally. Declarations that are
  genuinely part of the broader subsystem interface remain in `ai.h`,
  `ai_communication.h`, `ai_debug.h`, `encounters.h`, `props.h`, and
  `game_allegiance.h`.
- The major-upgrade enumeration lives beside
  `squad_definition::major_upgrade` in `ai_scenario_definitions.h`.
- Unit, object, tag-block, and tag-definition access uses the established
  `unit_get`, `object_get`, `TAG_BLOCK_GET_ELEMENT`, and
  `unit_definition_get` abstractions. Redundant casts after
  `data_iterator_next` were removed.
- The donor's misleading local name `engaging` was changed to
  `does_not_endanger_player`; the exact logic is a skip predicate for distant
  inconsequential or nonattacking actors.
- `ai_adjust_damage`'s owner declaration was corrected from `void`/const to its
  target-authenticated `boolean`/mutable contract.
- The reconstructed code contains no volatile/register forcing, manual byte
  emission, inline assembly, code-generation pragma, fake dependency, fixed
  boolean branch, or nonsensical exactness carrier. The standard
  `fake_match_scan.py` reports zero review leads.

The source-authenticated `match_assert(file, line, expression)` calls are
intentionally retained. `docs/assertions.md` defines that macro as the proper
abstraction when January's original source location is known. Replacing it
with ordinary `assert` would discard the authenticated path/line and change
the target code; using a modern variadic macro is also unsupported by the
pinned VC7 compiler.

## Inline and ownership boundary

The gate's explicit `_point_from_line3d` emitted-symbol guard passes. The
candidate does emit four ordinary header-inline `.text` COMDAT copies:
`vector_from_points3d`, `magnitude_squared3d`, `distance_squared3d`, and
`encounter_get_squad`. Their sections carry the canonical COMDAT flags and are
not credited as target functions in `ai.obj`. January's exact
`ai_enemies_endanger_player` and `ai_consider_major_upgrade` owners contain the
same symbolic calls, so these are necessary foldable copies of shared inline
owners, not invented AI functions.

## Deliberate exclusions

The donor's remaining `ai.c` harvest was not admitted merely because it was
present. The following January owners remain unwritten:
`ai_disconnect_from_structure_bsp`, `ai_find_line_of_fire_friend_pills`,
`ai_handle_editing`, `ai_handle_spatial_effect`, `ai_test_line_of_sight`,
`code_0002eb80`, and `code_0002f9d0`. `ai_test_ballistic_line_of_fire`,
`ai_test_line_of_fire`, and `ai_update` were also excluded because the donor
bodies were not strict exact. No fuzzy body, guessed anonymous-name mapping,
or uncredited donor function was carried into this packet.

## Verification

The isolated production-source gate was run with the canonical AI compile
flags:

```text
python tools/campaign/gate.py source/ai/ai --all \
  --forbid-emitted-symbol _point_from_line3d \
  --out scratch/ai-complete.obj
```

Result: **36 exact, 0 residual, 10 unwritten** of 46 January target functions;
the emitted-symbol guard passed. A separate COFF audit passed target/candidate
function type and storage-class equality for all 22 new functions.
`git diff --check` passed. Generated `build/` and `scratch/` artifacts are not
part of the commit.

After integration onto published canonical `6a9bc341e`, a full semantic sweep
initially exposed two unrelated losses caused solely by the donor's broader
header declaration placement: `game_engine::_populate_statistic_buffer` and
`units::_unit_preprocess_node_orientations`. Narrowing the implementation-only
declarations to `ai_runtime.h` restores both without changing any admitted AI
body. The final rebuilt ledger is **22 gains / zero losses / +4,973 meaningful
exact bytes**, with 6,708 accepted functions, 1,063,802 meaningful accepted
bytes, and zero unit errors. The universal per-TU gate also reports
`units.obj` at **189 exact / 0 residual / 0 unwritten** and `ai.obj` at
**36 exact / 0 residual / 10 unwritten**.
