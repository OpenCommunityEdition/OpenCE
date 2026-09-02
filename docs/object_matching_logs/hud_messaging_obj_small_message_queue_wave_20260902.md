# `hud_messaging.obj` small message-queue wave (2026-09-02)

## Result

Starting from canonical commit `9de998a2c`, this packet reconstructs the
small HUD message queue cluster from the January target and the pinned HCEA
semantic donors.  The focused gate advances from **22 exact / 0 residual / 14
unwritten** to **26 exact / 0 residual / 10 unwritten**.  The four new strict
matches contribute **503 meaningful / 528 padded bytes**.

| Function | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_find_free_hud_message` | `126 / 128` | `0` | `a9816a40ccaa45aba9a8cd621db246e92461e705945eb35d9417203cf65a86ae` |
| `_hud_print_message` | `121 / 128` | `5` | `a9d1bba56909ad816ab880fdc2770e432174e68087f4a012b60e31fc0746da4b` |
| `_hud_add_item_message` | `157 / 160` | `4` | `d652a85db29fdc70fd5723c842af77fc319546928ae11e0fd9a6c90034d61a66` |
| `_hud_broadcast_team_message` | `99 / 112` | `7` | `d5ff1c73a5e16e6ebc5568bf4b6ddcbba8dcdda789fcabce78c52af647bda28e` |

All 22 inherited exact functions remain exact.

## Reconstruction and provenance

The HCEA donors establish the ordinary queue semantics: prefer a free or
matching slot, otherwise evict the oldest message; stamp a new message with
the game time and monotonic byte marker; and broadcast only to local players
on the victim's team.  January remains authoritative for the PC ABI and
layout.  In particular, January iterates all four `MAXIMUM_LOCAL_PLAYERS`,
uses the 0x8C message and 0x460 per-player strides, and compiles the static
queue helper with `item_definition_index` carried in `esi` across its two
callers.

The recovered C uses `LONG_MAX`, `NONE`, `TRUE`, `FALSE`,
`MAXIMUM_LOCAL_PLAYERS`, `NUMBEROF`, and the typed `player_get` accessor.  It
does not contain raw datum casts, address-named owners, manual raw offsets,
forced inlining, assembly, volatile/barrier controls, or invented operations.
The public broadcast declaration lives in the owning
`interface/hud_messaging.h`; the static queue-helper declaration remains in
its translation unit.  `game_engine_running` comes from its owning
`game/game_engine.h` rather than a local declaration.

The HCEA `compare_messages` body is also recorded in the source with its
authenticated private name.  It is not credited in this packet because VC7
correctly dead-strips the static function until the deferred
`hud_messaging_update` caller is reconstructed; no artificial anchor was
introduced to force emission.

## Validation

- `tools/campaign/gate.py source/interface/hud_messaging --all`: `26 exact /
  0 residual / 10 unwritten`.
- Direct instruction and relocation comparison confirms all four new
  sections exactly.
- Header-consumer gates after adding the owner prototype:
  - `hs.obj`: `445 exact / 3 residual / 0 unwritten`;
  - `hud.obj`: `19 / 3 / 0`;
  - `players.obj`: `54 / 15 / 1`;
  - `game_state.obj`: `24 / 0 / 0`.
- `tools/fake_match_scan.py --fail-on-findings` on the changed source/header:
  zero review leads.
- Focused tool tests: `101 passed`.
- `git diff --check`: pass.

No parked entry, semantic exception, completion label, target data change, or
large `hud_messaging_update` reconstruction is part of this packet.
