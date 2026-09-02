# `game_engine_ctf.obj` small-families wave (2026-09-01)

## Scope and provenance

- Translation unit: `source/game/game_engine_ctf.c` (only file edited besides
  this ledger and `scratch/workers/game_engine_ctf*`).
- Target object: `build/split/source/game/game_engine_ctf.obj` (January 2002
  split, byte authority). Candidates compiled only through
  `tools/campaign/gate.py`; `build/base` untouched.
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1 with the build.ninja flags
  (`/O2 /Oy- /DDEBUG /Dxbox`). No flag, header, config or symbols.json change.
- Strict oracle: `tools/coff_compare.py` `section_infos_equal` through
  `gate.py` (padded bytes, normalized bytes, relocation identity).
- Evidence: target disassembly (`gate.py --disas`, saved in
  `scratch/workers/game_engine_ctf_disas.txt`), `scratch/xrefs.py` relocation
  lists, January assert strings (file `c:\halo\SOURCE\game\game_engine_ctf.c`
  with line anchors 0xBD, 0x107, 0x1A7, 0x1B4-0x1B6, 0x214, 0x261, 0x3CE,
  0x41A), the exact sibling functions in this TU, the engine callback table
  mapping in `game_engine_ctf_obj_jonas_public_leaves_20260828.md`, the HCEA
  cache (`halocea/src/blam/game/ctf_*.c`, names/semantics only) and
  `halocea/src/headers/ctf_globals.h` (field names), plus the prior ledgers
  (`game_engine_obj*.md`, player-added family, oddball predicates).
- This session resumed a worker stopped by a usage limit. Its first full write
  of all 22 unwritten functions (S1/S2 in `scratch/workers/game_engine_ctf.log`)
  and its measured negatives were inherited; every inherited body was
  re-verified with `gate.py` in this session.

## Validated baseline

Original campaign baseline (`gate.py source/game/game_engine_ctf`):
`== exact 17  residual 0  unwritten 22  (of 39 listed)`; `board.py`
`480/5328 B, 17/39 fn`. Every unwritten target symbol is a `_code_<addr>`
placeholder; the unit's globals block is the placeholder `_bss_0043e914`
(48 bytes, matching `struct ctf_globals`).

Orchestrator measurement of the resumed on-disk state without aliases was
`exact 16 residual 1 unwritten 22`: the one "lost" function, `_code_0009f470`,
shows `[reloc-identity]` only because the previous worker renamed the globals
placeholder to `ctf_globals` in the .c. With `--alias ctf_globals=bss_0043e914`
it is byte-exact again (verified RESUME-0). No code change was needed.

## Result (running; updated after every retained batch)

RESUME-0 (inherited on-disk state, all aliases): `== exact 33 residual 6`.
RESUME-1 (after V410i, Vdist15, Vw1, V4e): `== exact 35 residual 4`.
RESUME-2 (after V950i, V4g): `== exact 38 residual 1` (only
`_code_0009faf0` / `ctf_engine_weapon_update` remains, size and relocation
count exact).

All 17 baseline-exact functions remain exact throughout.

## Name provenance (target symbol -> source name)

| Target symbol | Source name | static | Provenance |
|---|---|---|---|
| `_code_0009eb60` | `ctf_create_flag_object` | yes | inferred: creates the flag object (`get_flag_definition_index`, `object_new`, "created a flag" string); private ABI |
| `_code_0009ebd0` | `create_the_flag` | yes | authenticated: HCEA `create_the_flag` (called from `ctf_engine_weapon_update.c`, `ctf_engine_initialize_for_new_map.c`); assert line 0xBD |
| `_code_0009ec30` | `ctf_single_flag_what_is_up_message` | yes | authenticated: HCEA `ctf_single_flag_what_is_up_message.c` |
| `_code_0009ed10` | `ctf_award_capture` | yes | inferred: scores a capture (asserts `game_engine_can_score()`, `NONE != player_index`, `NONE != team_index`); HCEA inlines this into `ctf_engine_player_update` |
| `_code_0009edd0` | `ctf_flag_failure_sound` | yes | authenticated: HCEA `ctf_flag_failure_sound.c` |
| `_code_0009ee10` | `ctf_find_flag_carrier` | yes | inferred: player iterator + `unit_has_weapon` (HCEA inlines the loop in `ctf_engine_weapon_update`) |
| `_code_0009ef20` | `ctf_engine_display_score` | no | authenticated: engine table slot 0x64 + HCEA `ctf_engine_display_score.c` |
| `_code_0009f2e0` | `ctf_sound_update_warning` | yes | authenticated: HCEA `ctf_sound_update_warning.c` |
| `_code_0009f320` | `ctf_set_flag_warning` | yes | authenticated: HCEA `ctf_set_flag_warning.c` |
| `_code_0009f340` | `ctf_weapon_drop` | no | authenticated: table slot 0x40 + HCEA `ctf_weapon_drop.c`; assert line 0x41A |
| `_code_0009f380` | `ctf_get_score` | no | authenticated: table slot 0x48 + HCEA `ctf_get_score.c` |
| `_code_0009f3c0` | `ctf_test_flag` | no | authenticated: table slot 0x7C + HCEA `ctf_test_flag.c` |
| `_code_0009f410` | `ctf_get_score_header_string` | no | authenticated: table slot 0x50 + HCEA `ctf_get_score_header_string.c` |
| `_code_0009f4a0` | `ctf_engine_initialize_for_new_map` | no | authenticated: table slot 0x0C + HCEA |
| `_code_0009f870` | `ctf_reset_flag` | yes | inferred: clears warning, `game_engine_flag_reset`, clears handled bit (HCEA calls `weapon_reset_flag`, a later-engine public) |
| `_code_0009f8c0` | `ctf_player_drop_flag` | yes | inferred: `unit_drop_current_weapon` + reset (assert `NONE != unit_index` line 0x1A7) |
| `_code_0009f950` | `ctf_position_near_flag` | yes | inferred: distance-to-flag predicate (HCEA inlines it in `ctf_engine_player_update`) |
| `_code_0009f9a0` | `ctf_engine_player_update` | no | authenticated: table slot 0x34 + HCEA |
| `_code_0009faf0` | `ctf_engine_weapon_update` | no | authenticated: table slot 0x38 + HCEA |
| `_code_0009fd30` | `ctf_engine_update` | no | authenticated: table slot 0x44 + HCEA `ctf_engine_update.c` |
| `_code_0009fdb0` | `ctf_weapon_pickup` | no | authenticated: table slot 0x3C + HCEA `ctf_weapon_pickup.c`; assert line 0x3CE |
| `_code_0009ff10` | `ctf_engine_starting_location_rating` | no | authenticated: table slot 0x68 + HCEA |
| `_bss_0043e914` | `ctf_globals` | (see notes) | authenticated: HCEA `ctf_globals.h` (same 0x30-byte layout for the January fields) |

(The remainder of this ledger is filled in below as batches are retained.)
