# `game_engine_slayer.obj` target/update closeout (2026-09-01)

## Result

The natural-C Slayer packet advances `source/game/game_engine_slayer` from
18 exact / 0 residual / 9 unwritten functions to 25 exact / 2 credible fuzzy
parks / 0 unwritten functions. The seven newly strict functions contribute
1,376 padded code bytes, and the reconstructed 640-byte display formatter
closes the final unwritten slot at a measured 98.98104% fuzzy match.

| Function | Padded bytes | Result |
| --- | ---: | --- |
| `_target_is_eligible` | 80 | strict exact |
| `_update_speed_for_score` | 272 | strict exact |
| `_slayer_engine_adjust_score` | 64 | strict exact |
| `_find_next_target` | 400 | strict exact |
| `_slayer_engine_player_killed_player` | 144 | strict exact |
| `_slayer_test_flag` | 16 | strict exact |
| `_slayer_player_update` | 400 | strict exact |
| `_slayer_engine_display_score` | 640 | credible fuzzy park |

`_slayer_get_score_header_string` is retained as a natural 80-byte candidate
against January's 96-byte function. It preserves all six relocation identities
and both semantic branches. January shrink-wraps ESI separately in each branch;
the available VC7 compile hoists one save and merges the common return. The
ordinary early-return and branch-local lifetime controls were exhausted without
manufacturing an optimizer carrier, so the function is recorded in
`config/parked.json` as a `tu-context-optimization` fixed point.

The former 640-byte `_code_000a4300` is authenticated as the public
`_slayer_engine_display_score` callback. The retained ordinary C reproduces
January's two-stage message topology, unconditional player-datum validation,
legacy dead `score[128]` formatting block, B3-B6 UI strings, packed-place
return, and the surprising team-score read in the nonteam branch. Target and
candidate have equal 640-byte padded envelopes and the same 52 relocation
identities in semantic order. They are byte-identical through offset `+0x161`.

Manual aligned review confines the residual to two branch-local pointer
allocation choices. January spills `place_name` to the dead `message` parameter
slot and keeps the team format in EBX, then assigns EBX/EDI to
`place_name`/`format` in the nonteam branch; the available VC7 compile chooses
the opposite legal registers. Argument values, call order, and branch behavior
remain identical. The fail-closed blocker classifier reports
`UNKNOWN/unclassified` because its atlas has no pointer register/spill-swap
rule, so the park records this as a manual register-allocation classification
rather than claiming tool proof. Declaration-order changes were inert, while a
shared fallback-format lifetime made both size and relocation topology worse.

## Source and naming evidence

The function and field names in this packet are authenticated by the later
Halo CE Anniversary source/PDB topology. January's COFF symbol is external and
is referenced from the Slayer engine callback table, so the callback retains
public January linkage even though the later HCEA PDB describes a static
variant. The `player_datum` fields formerly
named `unknown88` and `unknown_d1` are now `multiplayer_special` and
`quit_out_of_game`; the same semantic rename was applied to their existing
users without changing those translation units' exact-function counts.

Public Slayer declarations live in the dedicated owner header
`source/game/game_engine_slayer.h`. Adding those declarations to the broad
`game_engine.h` changed VC7 definition position and reopened `units.obj`; the
dedicated header preserves the January schedule and keeps `units.obj` at
189/189. The packed `game_engine_place` type and its two owner prototypes now
live in the narrow `source/game/game_engine_place.h` header instead of the
implementation file. Private helpers remain `static` in the Slayer translation
unit.

## Verification

- `tools/campaign/gate.py source/game/game_engine_slayer --all`: 25 exact,
  2 residual, 0 unwritten; both residuals are measurement-stable parks.
- `tools/campaign/gate.py source/units/units --all`: 189 exact, 0 residual,
  0 unwritten.
- The narrow owner header leaves Game Engine unchanged at 166 exact / 8
  residual / 6 unwritten.
- Full Ninja build, semantic report, and progress report pass.
- Strict campaign board moves from 710,595 to 711,971 padded bytes and from
  4,999 to 5,006 exact functions, with no unrelated exact-function loss; the
  fuzzy display-score closure intentionally adds no strict-match credit.
- No inline assembly, volatile optimizer control, fake dependency, address-name
  placeholder, or `point_from_line3d` COMDAT is introduced.
