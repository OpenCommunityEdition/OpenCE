# `ai_script.obj` Opus coherent reconciliation (2026-09-09)

Canonical base: `105d9acaacda589e21a4389d7e6bd69db0b42aba`.
Read-only Opus donor: `C:\halo-worktrees\opus-small-families-30k-20260902`
at `eeedd72e5ba0fc1761519f68cad241fe606320e7`.

This is a selective reconstruction, not a wholesale donor import. The admitted
packet adds **33 strict-exact functions, 5,624 meaningful code bytes and 5,872
padded code bytes**. Five useful natural bodies remain fuzzy and receive zero
exact credit. Two donor-exact but semantically invalid bodies and an incomplete
command-status graph are deliberately left unwritten.

## Exact result

The base object has 62/116 strict-exact owners. The ordinary focused gate uses
the pre-rename split object and reports 93 exact, six residual and 17 unwritten.
The single authenticated private-symbol correction
`_code_000461b0` -> `_ai_scripting_assess_status` changes no image byte. With
that name applied to the target, the final result is **95 exact, five residual
and 16 unwritten**. The delta from the base is 33 exact owners.

Trailing `0x90`/`0xCC` alignment bytes are excluded from meaningful sizes.

| newly exact owner | meaningful | padded |
|---|---:|---:|
| `_ai_index_actor_iterator_next` | 97 | 112 |
| `_ai_scripting_allegiance` | 236 | 240 |
| `_ai_scripting_assess_status` | 114 | 128 |
| `_ai_scripting_attach_free` | 221 | 224 |
| `_ai_scripting_attach_unit` | 542 | 544 |
| `_ai_scripting_braindead_by_unit` | 246 | 256 |
| `_ai_scripting_command_list` | 179 | 192 |
| `_ai_scripting_command_list_advance` | 141 | 144 |
| `_ai_scripting_command_list_advance_by_unit` | 100 | 112 |
| `_ai_scripting_command_list_by_unit` | 154 | 160 |
| `_ai_scripting_detach_units` | 135 | 144 |
| `_ai_scripting_erase` | 140 | 144 |
| `_ai_scripting_erase_all` | 47 | 48 |
| `_ai_scripting_exit_vehicle` | 157 | 160 |
| `_ai_scripting_force_active_by_unit` | 196 | 208 |
| `_ai_scripting_free` | 205 | 208 |
| `_ai_scripting_free_units` | 195 | 208 |
| `_ai_scripting_ignore` | 152 | 160 |
| `_ai_scripting_link_activation` | 168 | 176 |
| `_ai_scripting_look_at_object` | 106 | 112 |
| `_ai_scripting_magically_see_encounter` | 202 | 208 |
| `_ai_scripting_magically_see_players` | 145 | 160 |
| `_ai_scripting_magically_see_unit` | 204 | 208 |
| `_ai_scripting_place` | 138 | 144 |
| `_ai_scripting_prefer_target` | 152 | 160 |
| `_ai_scripting_select` | 52 | 64 |
| `_ai_scripting_set_return_state` | 206 | 208 |
| `_ai_scripting_set_team` | 101 | 112 |
| `_ai_scripting_spawn_actor` | 264 | 272 |
| `_ai_scripting_status` | 152 | 160 |
| `_ai_scripting_try_to_fight` | 195 | 208 |
| `_ai_scripting_try_to_fight_nothing` | 141 | 144 |
| `_ai_scripting_try_to_fight_player` | 141 | 144 |
| **total** | **5,624** | **5,872** |

## Private-name evidence

The donor names the 128-byte owner immediately preceding
`_ai_scripting_status` as `_ai_scripting_assess_status`. Its source reads one
actor and maps active/mode/combat/target/attack state to the status value that
the public wrapper maximizes across its iterator. Independently of that
semantic fit:

- January `_code_000461b0` and candidate `_ai_scripting_assess_status` have
  identical 128-byte sections, identical normalized hashes and the same two
  relocations (`_actor_data` and `_datum_get`);
- January `_ai_scripting_status` has a direct relocation to
  `_code_000461b0` at function offset 116;
- changing only that target symbol name makes the 160-byte status wrapper
  strict-exact as well;
- `config/symbols.json` contains one target entry for the semantic name and no
  collision.

The source definition remains `static`. No other address-derived symbol is
renamed by this packet.

## Honest fuzzy frontier

These are the best credible ordinary-C bodies found in this pass. None is
counted as exact.

| function | target meaningful / padded / relocs | candidate meaningful / padded / relocs | result |
|---|---:|---:|---|
| `_ai_index_actor_iterator_new` | 204 / 208 / 9 | 204 / 208 / 9 | residual `[sha]` |
| `_ai_index_from_string` | 294 / 304 / 13 | 305 / 320 / 13 | residual `[size, sha]` |
| `_ai_scripting_renew` | 361 / 368 / 22 | 361 / 368 / 22 | residual `[sha]` |
| `_ai_scripting_set_current_state` | 160 / 160 / 10 | 160 / 160 / 10 | residual `[sha]` |
| `_ai_scripting_vehicle_encounter` | 491 / 496 / 19 | 496 / 496 / 19 | residual `[sha]` |

The donor made `_ai_scripting_set_current_state` exact by redundantly declaring
`actor_action_set_default_state` inside `ai_script.c`, despite `actions.h`
already owning its correct prototype. Removing that declaration changes one
independent parameter-slot load and makes the body fuzzy. The owner-correct
version is retained and no exact credit is claimed.

The actor-reference iterator intentionally presents its final three contiguous
`long` fields to the established encounter-iterator API. The local view is
three `long` fields, while the public iterator is asserted to be 0x18 bytes
with `actor_index` at 0x10; this is a layout-checked iterator-tail view, not a
numeric or unrelated representation cast.

## Rejected donor paths

- `_ai_scripting_allegiance_broken` (65 meaningful / 80 padded) was donor
  exact only with `game_team_is_ally(...) && game_team_is_enemy(...)`. Those
  mutually exclusive predicates make the logic nonsensical, so the function
  remains unwritten.
- `_ai_scripting_follow_target_ai` (171 meaningful / 176 padded) formatted its
  follow-target diagnostic from `ai_reference` rather than
  `follow_ai_reference`. The misleading implementation remains unwritten.
- `_ai_scripting_command_list_status` (590 meaningful / 592 padded) depended
  on a tiny guessed helper in place of January's 1,067 meaningful / 1,072
  padded `_code_00044010`. That helper ignored the actor/unit indices and
  complex-control state required by the surrounding graph. Both public shell
  and guessed private helper were removed rather than preserving incomplete
  source as apparent progress.

## Ownership and house-rule reconciliation

Public declarations were removed from the `.c` file and placed only in their
true owners:

- `ai_erase` in `source/ai/ai.h`;
- `ai_conversation` in `source/ai/ai_communication.h`;
- `encounters_update_dirty_status` in `source/ai/encounters.h`;
- `game_allegiance_create` in `source/game/game_allegiance.h`;
- the public script entry points in `source/ai/ai_script.h`.

The implementation uses `actor_get`, `unit_get`/`unit_try_and_get`, actor/tag
definition accessors and `TAG_BLOCK_GET_ELEMENT`. The one generic `object_get`
walk reads the generic child link first and performs the unit overlay only
after testing `_object_mask_unit`. No raw typed `tag_get` cast is introduced.
New signatures put one parameter on each line; the no-argument wrapper uses
`void` on its own line; modified void functions end in explicit `return;`.
Project `real`, named enum values, `SET_FLAG`/`TEST_FLAG` and `match_assert`
are used. The packet introduces no inline assembly, `volatile`, `register`,
code-generation directive, raw byte emission, fake dependency, or spill
steering.

## Focused verification

```text
python -B tools/campaign/gate.py source/ai/ai_script --out scratch/ai_script_final.obj \
  --forbid-emitted-symbol _point_from_line3d \
  --forbid-emitted-symbol _ai_scripting_command_list_status_internal
== emitted-symbol guard passed (2 forbidden names checked)
== exact 93 residual 6 unwritten 17 (pre-rename split target)

semantic-name comparison:
== exact 95 residual 5 unwritten 16
base 62 -> final 95; +33 functions; +5,624 meaningful / +5,872 padded bytes

python -B tools/fake_match_scan.py source/ai/ai_script.c --fail-on-findings
Scanned 1 file(s); 0 review lead(s).

python -B -m pytest -q tools/test_fake_match_scan.py tools/test_cseries_assertions.py
193 passed

git diff --check -- <AI packet paths>
pass (only Git's existing LF-to-CRLF checkout warning)
```

All 14 direct non-`ai_script.c` includers of the five edited headers were
compiled with the focused gate both before and after the declarations changed.
Their exact counts remained, respectively,
`16, 29, 54, 24, 5, 21, 14, 23, 68, 3, 47, 13, 9, 54`; every exact-owner set
had zero losses and zero gains. Every compile also passed the
`_point_from_line3d` emitted-symbol guard.

No configure, Ninja build, or push was performed in this bounded lane.
