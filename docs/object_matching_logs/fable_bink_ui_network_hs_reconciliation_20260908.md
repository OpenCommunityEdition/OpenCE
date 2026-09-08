# Bink, UI server graph, network server, HS runtime, RandomMath, and SoundManager reconciliation

Date: 2026-09-08

## Admission result

This packet reconciles reviewed Fable/Opus work into canonical base
`f7afb70475c862d1934c26d698b49be76053d418`. The canonical progress ledger
advances from **975,317 to 982,215 meaningful exact code bytes** and from
**6,467 to 6,491 credited functions**: **+6,898 meaningful exact bytes / +24
credited functions**. The object count remains **390/833**.

The rename-stable strict-section sweep measures **24 new exact sections / 7,072
padded bytes / zero regressions**. The exact progress ledger credits the same
24 owners; `_server_list_menu_update` is admitted through hardened COFF
comparison because objdiff reports it as a false negative. No fuzzy body is
included in either exact-byte total.

## Exact packet

| Object | New strict owners | Padded bytes |
| --- | ---: | ---: |
| `bink_playback.obj` | 10 | 1,808 |
| `ui_widget_game_data_input_functions.obj` | 2 | 2,288 |
| `network_server_manager.obj` | 6 | 1,792 |
| `hs_runtime.obj` | 1 | 176 |
| `sound_manager.obj` | 5 | 1,008 |
| **Total** | **24** | **7,072** |

`random_math.obj` remains 17/17 exact. The supplied Bungie source authenticates
the private `direction3d_from_table` name, signature, parameter order, table
type, and common constants; that source-quality correction claims zero new
bytes.

## Honest fuzzy frontier

Nine complete useful bodies are retained and parked with frozen measurements,
but receive zero exact credit:

- `hs_evaluate_inequality`: direct project-`real` loads are retained. The
  Opus donor's exact-only redundant `double` widening/narrowing is rejected as
  unsupported compiler steering.
- `bink_alloc`, `bink_free_texture_cache`, `bink_playback_idle`, and
  `bink_query_analog_controller_buttons`: coherent Bink graph bodies whose
  remaining differences are documented TU-context or register-allocation
  choices.
- `bink_draw_frame`: coherent target-static texture, timing, and diagnostic
  body at the exact 912-byte envelope; its two-relocation and stack-schedule
  residual remains zero-credit.
- `network_pregame_status_screen_update`: complete typed graph with correct
  relocation ownership; two impossible pointer-null checks from the exact-only
  donor are rejected.
- `looping_sound_new` and `sound_refresh_looping`: complete typed SoundManager
  graph bodies retained at their measured natural fuzzy boundaries.

`network_game_server_add_new_client` is no longer fuzzy. Using Bungie's
source-attested `ipv4_address` union member from its proper `transport.h` owner
naturally closes the 368-byte function.

`splitscreen_pregame_status_screen_update` remains unwritten. No placeholder
body receives credit.

## Ownership and house rules

- All newly identified private functions and Bink/UI data owners use semantic
  names in `config/symbols.json` with January storage class preserved.
- `_code_001b5850` is an inherited exact one-byte **external** Bink owner, not a
  private function. Exhaustive prior repository, worktree, PDB, and source
  searches found no credible identity; it remains explicitly documented under
  its target name rather than inventing one.
- Bungie's attached networking source proves the shared error member spelling
  `_error_network_connection_lost`. Canonical's complete anonymous error enum
  is moved verbatim from `ui_widget.c` into its proper owner `ui_widget.h`.
  Temporary local duplicate members are removed from the two dependent TUs.
- The advertised-game validator prototype is placed in
  `network_client_manager.h`; narrow incomplete private networking layouts are
  not promoted into a shared header.
- The HS function-local `parameter_types[2]` and four-byte runtime global are
  split atomically. Restored existing consumers are not counted as gains.
- Typed tag/object access, project scalar types, enum members, explicit returns,
  and the January inline schedule are preserved. No new inline assembly,
  inline/noinline directive, volatile/register steering, optimizer pragma,
  fake dependency, raw-address access, representation-punning match trick, or
  nonsensical branch is admitted.

## Verification

- Full `ninja`, strict report, semantic report, and canonical progress: pass.
- Rename-stable snapshot: 8,245 target functions, 6,538 strict exact; 24 gains,
  7,072 padded bytes, zero regressions.
- Focused gates: RandomMath 17/0/0, HS Runtime 60/5/1, UI game-data 37/8/1,
  Bink 19/5/0, Network Server 65/5/0, and SoundManager 32/8/25.
- Full `ui_widget.h` 19-TU blast radius compiles, and the whole-tree stable
  sweep preserves every inherited exact owner.
- Park manifest: 345 active, zero stale, zero invalid.
- Object-admission audit: zero candidates, zero contradictions, zero
  revocations; five prior reviewed rejections remain rejected.
- Changed-source fake-match scan: zero findings.
- Focused emitted-symbol guards: no `point_from_line3d` owner emitted.
- Full tracked tools suite: 1,140 passed, 2 skipped, 26 subtests passed.
- Scoped `git diff --check`: pass.

## Treemap cadence

The last forced refresh was at 975,317 meaningful exact bytes. The next required
10,000-byte checkpoint is **985,317**. This packet reaches 982,215, so it is
published without falsely advancing the treemap checkpoint; the following
verified batch needs at least 3,102 additional meaningful bytes before refresh.

Detailed provenance and per-object boundaries are in:

- `bink_playback_obj_fable_graph_reconciliation_20260908.md`;
- `ui_server_graph_reconciliation_20260908.md`;
- `network_server_manager_crossbuild_reconciliation_20260908.md`;
- `hs_runtime_inequality_reconciliation_20260908.md`;
- `random_math_obj_bungie_source_name_reconciliation_20260908.md`;
- `sound_manager_obj_small_looping_graph_20260908.md`.
