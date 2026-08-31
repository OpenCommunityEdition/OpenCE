# `players.obj` credibility recovery — 2026-08-30

## Result

This change keeps coherent typed Players logic and removes reconstruction-only
code-generation steering.  It does **not** treat an exact byte sequence as
evidence that implausible source was original.

The current focused measurement, rebuilt from this lane, is:

| Measure | Result |
|---|---:|
| shared exact functions | 52 / 69 |
| shared exact code bytes | 7,744 / 15,888 |
| shared residual functions | 17 / 69 |
| residual code bytes | 8,144 |
| target-only functions | 1 |
| comparator errors | 0 |

This supersedes the earlier `59/9/2` claim.  That claim included code emitted
by a dead `if (0)` path and was not a valid source-credit census.  The dead
emitter is not restored.

## Credibility decisions

Removed constructs include volatile reinterpretation reads, `noinline`,
`_ReadWriteBarrier`, two artificial inline distance helpers, and dead emission
paths.  The retained implementation uses direct typed data, `distance3d`,
`TEST_FLAG`/`SET_FLAG`, action-result enums, and ordinary calls.  A focused
fake-match scan of `players.c` and `players.h` reports zero review leads.

`players_update_before_game` and `players_update_after_game` are retained as
normal game-loop callers.  They supply meaningful player spawning, control,
telefrag, powerup, interaction, and BSP-switch behavior; they were not added
as a byte-emission device and currently remain fuzzy where the comparator says
they do.

The former address labels in the bounded Players block are replaced with
January-atlas names.  Target storage class decides linkage: file-local helpers
are `static`; `player_teleport_internal` and `player_handle_powerup_equipment`
remain external and are declared in
`players.h`.  Declarations for other subsystems live in their owner headers.

`player_examine_nearby_unit` is deliberately **target-only**. The January
COFF section is exactly `C3` followed by 15 alignment NOPs, as reproduced by
`python tools/audit/disasm_coff_function_20260826.py
_player_examine_nearby_unit build/split/source/game/players.obj
build/base/source/game/players.obj` before the candidate body was removed.
The name/ABI evidence is concrete: the local Stian KB declaration at
`../../research-cache/stian-halo-cseries-20260820/kb.json:18220` says
`bool player_examine_nearby_unit(int player_unit_handle, int
nearby_unit_handle)`, and the related metadata name is at
`../../research-cache/stian-halo-cseries-20260820/kb_meta.json:6249`. Both donor
sources call that two-handle boolean form
(`../../research-cache/stian-halo-cseries-20260820/src/halo/game/players.c:3225`;
`../../research-cache/pastudan-halo-20260828/src/halo/game/players.c:2029`);
Pastudan explicitly records the XBE as a single-`ret` stub at
`../../research-cache/pastudan-halo-20260828/src/halo/game/players.c:456-462`.
Stian instead supplies an interaction body at
`../../research-cache/stian-halo-cseries-20260820/src/halo/game/players.c:814`,
proving that donor logic cannot be used as an original-source oracle. A no-argument C no-op that
reproduces `ret` contradicts the ABI, while a boolean `return FALSE` does not
reproduce the target. We retain only the header ABI and grant no credit until
first-party provenance explains the stub.

The local vehicle projection intentionally remains private to `players.c`.
`VEHICLES.C` still has a larger independent runtime definition, so publishing
an incomplete global vehicle type would be false ownership rather than a
useful header cleanup.

## Data boundary

The `0x00453408` BSS symbol is the external
`machine_to_player_table[4][4]` (0x40 bytes).  The following screen-flash and
debug state is represented by separate meaningful statics; no invented 0x5C
aggregate or padding global is claimed.  `players_static_data` at `0x002DEE08`
is an external-linkage object whose full profile-storage layout remains only
partially inferred, and `adjustment_weights` is a static read-only table. Data
ownership beyond those observed bounds is deliberately
not credited by this lane.

The current project report records 3,096 matched data bytes out of 5,349
Players data bytes. That aggregate is a progress measure, not an ownership
admission: this lane did not rerun a per-symbol data comparator and therefore
does not claim an exact section census.

The telefrag fields are named from their observed game-engine transition:
collision marks a blocking player and increments the timeout; the after-game
update decays or resolves it.  The Players global packs the local triggering
player in bits 0–3 and the recursive-switch tick count in bits 4–7, which is
the VC7/x86 declaration order used by the target.  These names and fields are
semantic inferences, not proof of strict object matching.

## Residual frontier and parks

The 17 shared residuals total 8,144 bytes: 16 are unclassified (7,904 bytes)
and `_player_handle_powerup` is the 240-byte control-flow/return-shape case.
Only four have evidence sufficient for `config/parked.json`:

| Function | Class | Target / base size, relocs | Target / base normalized SHA-256 |
|---|---|---|---|
| `_players_compute_combined_pvs` | instruction-scheduling | 368/22; 368/22 | `9e3ab79a...` / `82b53aac...` |
| `_player_teleport_on_bsp_switch` | instruction-scheduling | 320/21; 320/21 | `73f7fae1...` / `6a0e944e...` |
| `_players_reconnect_to_structure_bsp` | instruction-scheduling | 736/40; 736/40 | `de7f7f71...` / `f11d3315...` |
| `_unit_should_autopick_weapon` | register-allocation | 144/7; 144/7 | `1c1da7cd...` / `6a23f548...` |

The manifest contains the complete hashes, relocation counts, measured fuzzy
percentages, and reopen evidence. The other 13 residuals are merely **left
fuzzy and active**, not parked: `_players_initialize`,
`_players_initialize_for_new_map`, `_player_teleport_internal`,
`_player_set_action_result`, `_player_handle_powerup`,
`_player_handle_powerup_minor`, `_player_update_powerups`,
`_player_examine_nearby_vehicle`, `_player_examine_nearby_device`,
`_player_examine_nearby_item`, `_player_examine_nearby_objects`, and
`_players_update_before_game` / `_players_update_after_game` (the latter two
count as two owners). `_player_teleport_internal` is expressly an active
structural residual (1,296/61 target versus 1,312/62 base), not a compiler-tie
park. The target-only stub is not parkable.

## Validation

```text
ninja halobetacache_build
python -m tools.residual_frontier --unit source/game/players
python tools/fake_match_scan.py source/game/players.c source/game/players.h --fail-on-findings
python -m tools.parked_functions
git diff --check
```

All commands above pass in this lane (the full build retains unrelated legacy
warnings only).  Reopen a fuzzy function only with source/provenance that
explains its remaining schedule or lifetime naturally.
