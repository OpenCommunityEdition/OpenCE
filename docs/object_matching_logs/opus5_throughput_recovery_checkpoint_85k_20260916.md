# Throughput-recovery campaign: +85K checkpoint (2026-09-16)

Supersedes the running totals of the +75K record; earlier rejections and disclosures still apply. Frozen baseline
`9062b1e96`; branch `opus/throughput-recovery-150k-20260916`; nothing pushed. Manifest:
`opus5_throughput_recovery_checkpoint_85k_20260916.json`. Covers the +80K boundary.

## Measured result (vs the frozen 9062b1e96 stable snapshot)

| measure | baseline | now | delta |
| --- | ---: | ---: | ---: |
| strict meaningful exact bytes (credited) | - | - | **+85,973** |
| strict padded exact bytes (board) | 1,397,165 | 1,484,125 | +86,960 |
| exact functions (board) | 7,279 | 7,400 | +121 |
| all-function-exact objects (board) | 309 | 335 | +26 |
| whole-object admission candidates | 8 | 28 | +20 |
| accepted ledger | 7,315 / 1,347,469 | 7,436 / 1,433,442 | +121 / +85,973 |
| active park entries (incl. 2 asm-implemented) | 268 | 191 | -77 |
| inherited regressions | - | - | **0** |

Remaining to the 150,000 target: 64,027 meaningful bytes.

## Since the +75K record

| commit | content | functions | padded |
| --- | --- | ---: | ---: |
| `bf0f4cc7d` | wave 18: _game_load (`game_globals` static; symbols.json `_game_globals` static), _circle_tangents, _circular_queue_dequeue_data | 3 | 592 |
| `af9f444aa` | wave 17: _complete_key_exchange, _debug_realloc, _lra_allocate, _shader_transparent_chicago_create, _game_state_open_persistent_storage, _progress_bar_create_noise_texture (symbols.json static, provisional name) | 6 | 2,080 |
| `2f24377a5` | wave 19: _collision_bsp_test_vector_recursive, _collision_surface_find_closest_point2d, _ai_communication_update_speech_timers, _cached_map_file_read_header, _update_client_local_ticks, _rasterizer_sort_external | 6 | 2,800 |
| `6be078ec9` | wave 20: _encounter_update_status, _encounters_test_activation, _encounter_compute_activation_cluster_bit_vector, _encounter_update_respawn, _network_connection_write, _network_connection_new, _symbol_name_from_address | 7 | 5,584 |

Newly all-exact objects since +75K: key_agreement, debug_memory, player_queues_new, circular_queue, lra_cache,
rasterizer_transparent_geometry, shader_transparent_chicago_preprocessor, game_state_xbox. Wave 20 found nothing
in aim_assist, actions, hs_compile or structure_detail_objects.

## Checkpoint verification

Full `ninja` after every packet; stable diff vs 9062: 121 gained, 0 regressions. `tools.parked_functions`: 191 active,
0 stale, 0 invalid. `audit_object_admission`: 28 candidates / 0 contradicted / 8 rejected / 0 revoked. pytest 1,151
passed, 5 skipped, 26 subtests. Fake-match scan 22 (= baseline, no new rule/path). `_point_from_line3d` emitters 7
(= baseline). Per-file audit of all 77 changed C files: 0 flagged. New owner deltas since +75K all exist in January's
own objects: `_progress_bar_create_noise_texture`, the eight `rasterizer_transparent_geometry` file-private globals,
and January's `network_connection` assert-text literals. Re-measured caller park: `_cache_files_open_cache_files`
(closer, still non-exact).

## Disclosed judgement calls (landed)

- `_network_connection_write`: the retry loop tests `bytes_written <= 0 && bytes_written == would_block`. January's
  bytes contain both tests, and an independent reconstruction of the Xbox binary (Stian) has the same double test.
- `_update_client_local_ticks`: `long machine_index = 0;` is passed to both server calls. A literal `0` argument does
  not reproduce January's register load around the out-of-line build helper.
- `_cached_map_file_read_header`: the wait helper copies the `volatile` completion flag into `completed` before
  returning, which reproduces January's second volatile load at both inlined sites.
- `_ai_communication_update_speech_timers`: the talk/chatter label is a named `timer_name` local.
- `_encounters_test_activation`: the activation OR chain does not short-circuit, as January's bytes show and HCEA
  confirms.
- `_progress_bar_create_noise_texture`: an empty static function at January's own symbol offset, with a provisional
  descriptive name.

## Owner-ruling queue (unchanged, measured unlocks)

point_from_line3d COMDAT (~13K B across ~9 functions), P7 projection_from_vector3d header body (1,328 B), January-proven
redundant stores (`_encounter_test_rule` 656, hardware geometry pair 720, action_obey_command/charge/converse),
`_vector_from_points3d` COMDAT (512), `_player_control_modify_desired_angles` factor order (1,584), units.h short
impulse prototypes (`_actor_unit_control` 784 and two AI callers), interface profile header packet
(`_render_debug_profile_stall_tick` 272), UB forms (uninitialized/NULL/out-of-bounds reads).
