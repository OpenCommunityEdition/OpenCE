# `director.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Outcome

This wave did not change `source/camera/director.c`. All three non-exact rows are parked, so the work was done on scratch copies only. Two parks are proposed for reopen with new evidence. The third has a strict-exact scratch body, but it needs an owner ruling.

The real-file gate before and after is identical: `== exact 25 residual 3 unwritten 0`. The `_point_from_line3d` guard passes, the three parks show no drift, and `fake_match_scan` reports 0 leads.

| function | January | park candidate | wave result |
| --- | --- | --- | --- |
| `_director_update_controls` | 960 / 45 relocations | 960 / 45, sha | **strict EXACT in scratch** → park-reopen proposed |
| `_director_process_variables` | 544 / 27 | 544 / 27, sha | **sha identical**; strict EXACT once 3 data labels are renamed → park-reopen proposed, needs a symbols.json rename |
| `_director_script_camera` | 304 / 23 | 256 / 19 | strict EXACT in scratch only with an unused lookup local → owner ruling needed |

Scratch candidates, each built from the current real file with only the named bodies or declarations changed:

- `scratch/workers/director_cinematics_etc/director_reopen_update_controls.c`
- `scratch/workers/director_cinematics_etc/director_reopen_process_variables.c`
- `scratch/workers/director_cinematics_etc/director_reopen_combined.c`: both reopens together. Result: 26 exact, and process_variables differs only in relocation identity.
- `scratch/workers/director_cinematics_etc/director_all3_ownerblocked.c`: evidence only.

## `_director_update_controls`: park-reopen

The park's premise was "register/x87 scheduling within aligned paths". Aligned diff (alndiff) refutes it. The residual came from two source-shape errors:

1. **Statement order.** January builds `control_flags` from `[gamepad+0x17]` into bit `0x10` first. Only after that does it test `[gamepad+0x16]` with `or 0x20` / `and ~0x20`. So January's source runs `SET_FLAG(_camera_control_up_bit, right trigger)` before `SET_FLAG(_camera_control_down_bit, left trigger)`. Three other places use the same up-before-down order:
   - the camera_control enum;
   - the keyboard path in the same function (`r` = up, `f` = down);
   - the order January evaluates the flags in.
2. **Single-exit law.** January keeps `switch_camera` in AL at the join. Both "camera is first-person/following" and "debug controls off" go to one shared epilogue with no reload, and so does the no-input path (`xor al,al`). That is one bottom return with no early returns, where the no-input value is the declared initial `FALSE`.

Shapes tried:

| shape | result |
| --- | --- |
| u1: SET_FLAG order only | flag packet aligned; remaining allocation diff |
| u2: u1, plus `= FALSE` initialiser, early returns kept | 944 bytes (worse, reverted) |
| u3: u1, plus `else return FALSE;` | 944 bytes (worse) |
| u4: u1, plus nested positive `if`s (single exit), else-store kept | 960; AL fixed; epilogue layout left |
| **u6: u4, plus `boolean switch_camera = FALSE;` with the else arm removed** | **strict EXACT 960 / 45** |

Checks on u6:

- all 25 baseline-exact rows stay exact;
- the other two parks show no drift;
- the guard passes;
- the owner census shows no candidate-only or lost owners against `build/base`;
- fake scan reports 0 leads.

The source is house-rule clean: declare-and-initialise, one exit, typed enums, cseries `SET_FLAG`.

## `_director_process_variables`: park-reopen, needs a data-label rename

The park named two residuals: the load width of the 1.3 constant, and the x87 multiplication order. Both come from literals that are really named file constants.

Evidence:

- **January data ownership.** `.rdata` in the January `director.obj` split is a 32-byte section containing, in order: `director_game_camera_modes`, `director_script_camera_record_camera_modes`, 0.03f, 5.0f (`friction`), 25.0f and 1.3f. These are plain data, not literal COMDATs. `config/symbols.json` carries a second copy of `__real@3cf5c28f`, `__real@41c80000` and `__real@3fa66666` at other offsets (2406328, 2374420, 2381628). Those other copies are the PDB-public COMDAT literals. The copies inside the director object (2452264, 2452272, 2452276) do not appear in January's PDB publics at all.
- **HCEX.pdb.** DIA2Dump `-psr` / `-ps` around `friction` (RVA 0xB0248) lists the director compiland's file statics in the same order: `director_game_camera_modes`, `director_script_camera_record_camera_modes`, `ticks_per_millisecond`, `friction`, `acceleration_scale`, `genius_boy`. All of them are `const float` / `const short[]`.
- **HCEX_debug.exe bytes** at those RVAs, read as data only: `3cf5c28f` (0.03), `40a00000` (5.0), `41c80000` (25.0), `3fa66666` (1.3). These are exactly January's values.

Shapes tried:

- **pa.** Define `real const ticks_per_millisecond = 0.03f;`, `acceleration_scale = 25.f;` and `genius_boy = 1.3f;` around `friction`. Use them as `power(genius_boy, speed_delta)` and `... * hyper_scale * acceleration_scale`.
  - The normalized sha becomes identical to January's (`bef5cd3d…`). This fixes both the dword load and the 25-product operand order without any expression permutation.
  - The candidate `.rdata` becomes the byte-identical 32-byte January section. It was 20 bytes before.
  - Two relocation-identity differences remain: the label names, and the `dtime` / `friction` pair.
- **pb.** pa, plus `PIN(friction * director_globals.dtime, 0.f, 1.f)`. With the three target labels renamed in memory (`scratch/workers/director_cinematics_etc/renamecheck.py`), this is strict EXACT at 544 / 27.

How the operand order was established, by a one-token measurement in the same TU:

- `dtime * friction` compiles to `fld friction; fmul dtime`.
- `friction * dtime` compiles to `fld dtime; fmul friction`.

For two absolute memory operands, VC7 loads the right operand first. In the same function, `dtime * instance->velocity` and `scale * dtime` both load the absolute operand first. January's `fld dtime` therefore identifies `friction * dtime` as the spelling. The verifier should review this one order-dependent token.

**ORCHESTRATOR-PROPOSAL.** In-place edits to `config/symbols.json`, then regenerate the `director.obj` split with csplit:

```
line 14067: { "file_offset": 2452264, "flags": 0, "name": "_ticks_per_millisecond" },
line 14069: { "file_offset": 2452272, "flags": 0, "name": "_acceleration_scale" },
line 14070: { "file_offset": 2452276, "flags": 0, "name": "_genius_boy" },
```

- **Consumers:** only the `director.obj` split. The same literal values in other objects sit at different offsets and keep their names.
- **Optional follow-up (not needed for exactness):** HCEX marks all six objects as File Static, and January's PDB publics omit them. A separate `"static": true` packet plus source `static` could be measured.

## `_director_script_camera`: owner-blocked

January emits three inline `director_get` assertion packets (line 179): one at the loop top, one in the scripted arm and one in the else arm. The scripted arm's three stores (`camera_proc`, `debug_input_scale = 1.0f`, `debug_controls = FALSE`) follow `director_set_camera`'s statement order exactly, with the `interpolate` store folded away. So that arm is `director_set_camera(local_player_index, scripted_camera_update, FALSE)` inlined. The else arm is the inlined `director_choose_game_perspective`.

- **sc1:** keep `struct director *director = director_get(local_player_index);` at the loop top, and use `director_set_camera(..., FALSE)` in the scripted arm. Result: strict EXACT 304 / 23, with all siblings and parks preserved.
- **sc2:** the same without the loop-top lookup. Result: 256 / 19.

The loop-top local is never used; its only emitted effect is the bounds assertion. That is the "unused lookup local" class the 100K lane's verifiers declined (`_post_evaluator_global`, `_pre_evaluator_combatmove`, actor_firing_position). The body is therefore not proposed as an admissible reopen. It is recorded for the owner ruling in handoff item 1.

The 20260903 ledger's `director_set_camera` trial stayed at 256 / 19 because it also dropped the loop-top lookup.

## Reopen criteria

- **update_controls, process_variables:** met (see above). The orchestrator verifies, applies the rename packet, and unparks.
- **script_camera:** reopen when the owner admits unused lookup locals, or when January local records show the loop-top lookup.
