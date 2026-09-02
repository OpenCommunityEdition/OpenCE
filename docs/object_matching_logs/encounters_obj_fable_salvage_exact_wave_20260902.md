# `encounters.obj` Fable salvage and ownership reconciliation (2026-09-02)

## Result

- Unit: `source/ai/encounters` against the January 2002 split object.
- Canonical baseline `778961b89`: 12 exact, 0 residual, 49 unwritten.
- Preserved Fable checkpoint `589a83930`: 25 exact, 10 residual, 26 unwritten
  before anonymous-target symbol reconciliation.
- Accepted result: **39 exact, 6 residual, 16 unwritten**.
- Full-tree strict comparison: **27 newly exact functions, 5,568 padded code
  bytes, zero exact-function losses**.
- No function is parked or claimed exact merely because it has a complete body.
  The six emitted non-exact functions remain visible residuals, and the sixteen
  omitted target functions remain unwritten.

The Fable working evidence remains under
`C:\halo-worktrees\fable-small-families-20260901\scratch\workers`; the immutable
salvage snapshot is `589a83930` on
`root/fable-small-families-salvage-20260902`.

## Newly exact functions

| function | padded bytes |
| --- | ---: |
| `actor_iterator_next` | 160 |
| `encounter_activate` | 192 |
| `encounter_actor_iterator_prev` | 128 |
| `encounter_attach_actor` | 544 |
| `encounter_attach_unit` | 272 |
| `encounter_build_firing_position_owner_actor_indices` | 304 |
| `encounter_clear_pursuit` | 80 |
| `encounter_deactivate` | 160 |
| `encounter_detach_actor` | 432 |
| `encounter_find_pursuit` | 240 |
| `encounter_force_activate` | 48 |
| `encounter_force_deactivate` | 48 |
| `encounter_get_by_name` | 96 |
| `encounter_link_activation` | 160 |
| `encounter_mark_examined_pursuit_position` | 256 |
| `encounter_modify_pursuit_desires` | 160 |
| `encounter_set_respawn` | 80 |
| `encounter_squad_timer_expire` | 176 |
| `encounter_stand_down` | 432 |
| `encounter_update_squads` | 256 |
| `encounter_update_timers` | 112 |
| `encounter_verify_firing_position_owner_actor_indices` | 320 |
| `encounterless_attach_actor` | 176 |
| `encounterless_deactivate` | 80 |
| `encounterless_detach_actor` | 320 |
| `encounters_unit_died` | 208 |
| `squad_get_actor_type` | 128 |

## Anonymous January symbol recovery

HCEA's symbol-rich Xbox reconstruction, January call/relocation topology, and a
target-wide cross-object reference scan identify all twenty anonymous target
entries below. None is present in the January public PDB delta and none has a
cross-object caller, so each target entry is marked local in `symbols.json`.
The names are semantic source names from HCEA rather than retained
`code_XXXXXXXX` placeholders.

| January address | recovered private name |
| --- | --- |
| `00048260` | `encounter_clear_pursuit` |
| `000482B0` | `encounter_find_pursuit` |
| `000486C0` | `squad_reset_starting_locations` |
| `00048790` | `encounter_new` |
| `00048A20` | `squad_get_actor_type` |
| `00048B00` | `encounterless_deactivate` |
| `00048B50` | `encounter_activate` |
| `00048CB0` | `encounter_deactivate` |
| `00048D50` | `encounters_test_activation` |
| `000492D0` | `encounter_post_combat_add_possibility` |
| `00049360` | `encounter_update_timers` |
| `000494E0` | `encounter_update_squads` |
| `000495E0` | `encounter_test_rule` |
| `0004A140` | `encounter_post_combat_select_random_behavior` |
| `0004A250` | `encounter_post_combat` |
| `0004AA10` | `encounter_place_actor` |
| `0004ACF0` | `encounter_update_respawn` |
| `0004AFB0` | `encounter_update_platoons` |
| `0004B0F0` | `encounter_update_follow` |
| `0004C270` | `encounter_control_actors` |

## Prototype and type ownership

- Public encounter APIs now live in `encounters.h`; consumer-local declarations
  were removed from `actors.c`, `actions.c`, `action_search.c`, and `ai.c`.
- `actor_verify_activation` and `actor_set_team` live in `actors.h`.
- `actor_place` is an external `actors.obj` API authenticated by the January PDB
  at RVA 185920 and by HCEA. Adding it to the broad `actors.h` changes C2's
  declaration-position state and reopens the otherwise exact 1,920-byte
  `unit_preprocess_node_orientations`. The narrow actor-owned
  `actor_placement.h`, included only by its owner and encounter consumer,
  preserves both correct ownership and January code generation.
- AI team/upgrade APIs live in `ai.h`, magical-sight scripting lives in
  `ai_script.h`, and the team predicate lives in `game_allegiance.h`.
- Manual low-word masks were replaced with `DATUM_INDEX_TO_ABSOLUTE_INDEX`, and
  the packed AI squad reference uses `DATUM_INDEX_NEW` and named constants.

Four not-yet-written local helpers are called by the retained fuzzy
`encounters_update`: `encounter_update_respawn`, `encounter_update_platoons`,
`encounter_update_follow`, and `encounter_control_actors`. Target evidence proves
they are private. VC7 rejects a called-but-undefined `static` declaration with
C2129, however, so their declarations remain translation-unit-local but
temporarily non-static. Their definitions and `static` linkage must land
atomically. They are not exported through `encounters.h`, and no fake stubs or
retention calls were introduced.

## Source-credibility checks

HCEA independently corroborates the recovered gameplay behavior. One routine
looks suspicious but is authentic: `encounter_post_combat_add_possibility` does
not break after inserting slot zero. January x86 branches always execute the
second loop iteration, and HCEA's Xbox disassembly records the same behavior;
a candidate that beats slot zero can therefore occupy both two-entry slots.
The five-parameter ABI is also confirmed, including direct `prop_index` and
`unit_index` stores. The function is currently omitted by C2 because its only
caller, `encounter_post_combat`, is unwritten; it will emit naturally when that
caller is recovered.

`encounter_spawn_actor` likewise returns false on every path in both January
and HCEA even after a successful spawn. That result is retained as an
authenticated original quirk, not invented matching logic. Marathon has no
post-combat-possibility analogue, so direct Halo target evidence controls.

No volatile/register forcing, barrier, pragma, inline assembly, raw-address
access, fake dependency, inert retention branch, forced inline, or anonymous
code/data placeholder was admitted.

## Honest residual frontier

| function | target/candidate padded bytes | disposition |
| --- | ---: | --- |
| `encounter_create` | 752/752 | normalized-byte residual |
| `encounter_place_actor` | 368/368 | normalized-byte residual |
| `encounter_spawn_actor` | 288/288 | normalized-byte residual |
| `encounters_create_for_new_map` | 192/192 | normalized-byte residual |
| `encounters_test_activation` | 976/944 | five-relocation/codegen gap |
| `encounters_update` | 256/224 | ordinary-C scheduling gap |

These are credible partial reconstructions and remain unparked for later work.

## Verification

- `python tools/campaign/gate.py source/ai/encounters --all`:
  `39 exact / 6 residual / 16 unwritten`.
- `python tools/campaign/gate.py source/units/units --fn
  _unit_preprocess_node_orientations`: strict exact, 1,920 padded bytes.
- Full `ninja all_source progress semantic_progress`: passed with zero unit
  errors; semantic report accepted 5,177 exact functions.
- Per-function comparison across every built unit against canonical
  `778961b89`: 27 gains in `encounters.obj`, zero losses anywhere.
- `tools/fake_match_scan.py --fail-on-findings` over all changed C/header
  files: zero review leads.
- `python -m pytest -q -p no:cacheprovider --basetemp
  build/pytest-encounters-20260902`: 258 passed.
- `tools.parked_functions`: no stale or invalid entries;
  `tools.audit_object_admission`: no candidates or contradictions.
- `git diff --check`: passed.
