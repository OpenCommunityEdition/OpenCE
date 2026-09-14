# `ui_widget_game_data_input_functions.obj` — opus5 150K house-clean lane, wave w1 (2026-09-14)

Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, base `12f7375d4`. Worker scope: `source/interface/ui_widget_game_data_input_functions.c` only. No header, config, build or git operation was performed. Worker totals are not credit; only the orchestrator's full build counts.

## Baseline

`gate.py --all --forbid-emitted-symbol _point_from_line3d`: **39 exact / 7 residual / 0 unwritten**, guard passed.

| function | padded | baseline tag |
|---|---:|---|
| `_multiplayer_game_directions` | 320 | sha |
| `_network_pregame_status_screen_update` (PARKED) | 1,984 | sha |
| `_player_profile_1wide_list_update` | 1,152 | sha |
| `_player_profile_3wide_list_update` (accepted ledger, semantic tier) | 1,232 | reloc-identity |
| `_player_profile_update_cache_for_nwide_list` | 272 | size 288!=272, relocs 11!=13, sha |
| `_solo_level_select_list_update_displayed_items` (PARKED) | 704 | sha |
| `_splitscreen_pregame_status_screen_update` | 1,216 | sha |

Ledgers read first: fable_salvage_20260902, profile_fuzzy_reconciliation_20260909, ui_server_graph_reconciliation_20260908, claude_remaining_donor_batch_20260908, fable_bink_ui_network_hs_reconciliation_20260908, devices_game_input_hud_integration_batch_20260904, player_profile_s3tc_batch_reconciliation_20260907, playlist_profile_obj_display_name_owner_hold_20260907, and the opus5 100K handoff. `branch_sweep`: no historical blob beats the base. None of these functions is in the 100K admission manifest; only 3wide is in `accepted_ledger`.

## Final

Real file: **39 exact / 7 residual / 0 unwritten**, guard passed, parks drift 0, owner census 0 new owners, fake scan 0 leads. Two rows are now **byte-identical to January (normalized SHA equal)**. Their only strict-gate difference is a csplit one-past-the-end relocation alias, the same class as the accepted 3wide row.

| function | final | classification |
|---|---|---|
| `_player_profile_update_cache_for_nwide_list` | 272/13, sha equal | csplit alias only (landed) |
| `_splitscreen_pregame_status_screen_update` | 1,216/48, sha equal | csplit alias only (landed) |
| `_network_pregame_status_screen_update` | strict EXACT in scratch | park reopen proposed |
| `_player_profile_1wide_list_update` | 1,152/70, sha equal in scratch | owner ruling needed (not landed) |
| `_multiplayer_game_directions` | 122/122 instructions in scratch, one block-placement difference | not landed, 5 shapes spent |
| `_solo_level_select_list_update_displayed_items` | unchanged | skipped: parked scheduling tie, no new evidence |
| `_player_profile_3wide_list_update` | unchanged | skipped: already accepted |

## Key new evidence: `network_machine_is_valid`

January's pregame code tests the pointer after taking `&game->machines[i]`:

```
lea ecx,[eax-40]
test ecx,ecx
je ...
```

Three earlier ledgers rejected this as an impossible, nonsensical null check. It is in fact the expansion of Bungie's shared macro:

```c
#define network_machine_is_valid(machine) \
	((machine) && (machine)->machine_index >= 0 && \
	(machine)->machine_index < MAXIMUM_NETWORK_MACHINE_COUNT)
```

- **January attestation.** Assertion literals in four January objects stringify the macro *name*: `network_client_manager.obj`, `network_game_manager.obj`, `network_server_manager.obj` and `network_server_message_handler.obj` (`network_machine_is_valid(client_machine)`).
- **Byte precedent.** The admitted strict-EXACT `_network_game_add_machine` emits the identical `lea edx,[ecx-40]; test edx,edx; je` from `!network_machine_is_valid(&game->machines[machine_index])` (network_game_manager.obj +0x46).

No header owns the macro; each of those four TUs defines its own local copy. This TU now carries the same local copy, with a disclosure comment that names the existing copies.

## Per-function outcomes

### `_player_profile_update_cache_for_nwide_list` — landed (bytes exact, alias-only tag)

Evidence:
- The January diagnostic relocation is `not enough cache profiles` (a `match_vassert` message, line 0xCA2). The old body stringified the expression instead, which emitted a literal January does not have.
- Loop compares are signed (`jl`), so the source uses `(long)NUMBEROF`.
- `profile_indices[requested_index]` is re-read at +0xb8 and +0xd7, so there is no cached local.
- The `mov word [ebp-3],si` initializer is `{ FALSE }`.
- The body is instruction-identical to the already-exact sibling `_variant_profile_update_cache_for_nwide_list`, which proves the assert-in-loop topology.

The 20260909 ledger had recorded that this body flipped `_mutliplayer_settings_select_list_update_displayed_items`. Re-measured today: that function stays EXACT.

Only relocations +0x40 and +0x79 differ. January names `_local_player_index_for_draw_string_and_hack_in_icons+0`; ours is `_cached_player_profile+156`, which is 3 × 0x34 past the start of the array (one past its end).

### `_splitscreen_pregame_status_screen_update` — landed (bytes exact, alias-only tag)

The loop now reads `if (network_machine_is_valid(machine) && machine->machine_index == local_machine_index)`. Result: 1,216/48 with SHA equal.

The single remaining relocation is at +0x4a1:
- January: `??_C@_0CL@DNDKGOAJ@invalid game_data_input_referenc@+2`
- ours: `_local_player_controller_bitmap_frames+26`

The table is 24 bytes at 0x276E0C, so 0x276E26 is the strength-reduced loop end pointer, 2 bytes past the table's end. csplit attributes that address to the next `.rdata` symbol.

### `_network_pregame_status_screen_update` — PARK REOPEN PROPOSED

- **Candidate:** `scratch/workers/ui_widget_game_data_input_functions/np_m.c`, which is the current real file plus two changed lines.
- **Change:** both machine loops change `VALID_INDEX(machine->machine_index, MAXIMUM_NETWORK_MACHINE_COUNT) &&` to `network_machine_is_valid(machine) &&`.
- **Result:** strict EXACT, 1,984/81, zero relocation differences and zero instruction differences.
- **Whole-TU gate:** 40/6; the only changed row is this one. Census, fake scan and guard are clean.

The park's documented premise ("impossible `machine &&` tests", house rule 21) is refuted by the macro attestation above.

### `_player_profile_1wide_list_update` — not landed; owner ruling required

**Structural evidence from the January bytes:**
1. **The retry is a tail self-call.** The loop head at +0x10 (aligned by `lea esp,[esp]`) sits before the 0x85E parent assert and both definition asserts. The retry path ends `jmp 0x10` after the filter and `PIN` steps. That makes the source a tail self-call compiled to jmp-to-top (tail-recursion law):
   - if `profile`: fill the widgets;
   - else if `number_of_items > 0`: filter, `PIN`, then call `player_profile_1wide_list_update(list_widget)`;
   - else: clear the widgets.

   HCEA describes the same compact/clamp/retry semantics.
2. **The two description strings are named locals.** `unicode_string_list_get_string(joystick…)` is evaluated before the button call, and its result is held in `edi`. Inline arguments would evaluate right to left. HCEA uses the same two locals.
3. **One path stores `text[0xFF] = 0` twice.** The default-profile valid path stores at +0x370, then jumps to +0x405, which stores again at +0x408. Every other path stores once.

**Shapes tried:**

| shape | change | result |
|---|---|---|
| A | tail recursion + locals | 363 instructions; the first 260 are identical |
| B | A + per-branch inner store | 1,168 bytes (worse) |
| C | A + inner default-valid store + one common store after the default/non-default if/else | 1,152/70, SHA equal; tag reloc-identity only |

Shape C's remaining relocation differences:
- the csplit alias at +0x1a9;
- the `L""` and `"\r\n"` literal COMDATs, which are undefined in January and defined in ours.

Both classes also appear on the accepted 3wide row.

**Candidates:**
- `scratch/workers/ui_widget_game_data_input_functions/p1_final.c`: final real file plus this body only.
- `scratch/workers/ui_widget_game_data_input_functions/combined.c`: adds the pregame reopen; gate 40/6.

**Why it is not landed.** The brief lists "redundant/duplicated stores" as prohibited, and the previous lane declined `_extract_sequence` for January-proven redundant logic. The duplicate store is proven by January's bytes, not added to steer codegen, but admitting it needs an owner ruling.

### `_multiplayer_game_directions` — not landed (5 shapes)

The teams tail is a phi copy:

```
mov ecx,[ebp+8]
pop edi
mov eax,ecx
...
mov word [ecx+40],1a
mov byte [eax+10],1
```

That pattern proves `if (a && b) sli = teams_ready; else sli = waiting_for_teams; widget->visible = TRUE;` (a local join store).

| shape | change | result |
|---|---|---|
| A | local join store | 122/122 instructions; the loop and tail are now identical |
| B | `else if` inside the server block | same as A |
| C | flat single-exit chain | same as A |
| D | one global visible join | worse |
| E | `boolean waiting_for_machines` local | same as A |

The residual is layout only: January places the waiting_for_machine body (0x22) after the waiting_for_player body (0x23). The best candidate is `scratch/workers/ui_widget_game_data_input_functions/mgd_a.c`.

**Reopen criterion:** a spelling with evidence that makes VC7 lay out the machine-count body after the player-count body.

### `_solo_level_select_list_update_displayed_items` — skipped

This is still the parked two-load transposition at +0x9a/+0x9d, driven by player_profile.h context. There is no new evidence, and the park says no source-pressure tuning.

## Orchestrator proposals

1. **Unpark `_network_pregame_status_screen_update`.** Apply the two-line change from `np_m.c` and remove its parked.json entry in a dedicated commit.
2. **Semantic-tier credit** for `_player_profile_update_cache_for_nwide_list` (268 meaningful) and `_splitscreen_pregame_status_screen_update` (1,208 meaningful), both already landed. They are expected to be credited through objdiff-coff-shape like 3wide. Confirm objdiff accepts the +26 / next-symbol +2 alias.
3. **Owner ruling for `_player_profile_1wide_list_update`** (1,142 meaningful) on admitting the byte-proven duplicated terminator store.
4. **Optional header move.** Move `network_machine_is_valid` into a genuine shared networking header and remove the five TU-local copies:
   - `network_client_manager.c:468`
   - `network_game_manager.c:101` (spelled with `< 4`)
   - `network_server_manager.c:543`
   - `network_server_message_handler.c:323`
   - `ui_widget_game_data_input_functions.c`

   The move is preprocessor-only, but the header needs `MAXIMUM_NETWORK_MACHINE_COUNT` in scope.
