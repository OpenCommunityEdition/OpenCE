# Throughput-recovery campaign: +40K checkpoint (2026-09-16)

Supersedes the +10K record's running totals (`opus5_throughput_recovery_checkpoint_10k_20260916.md`, whose rejected
list still applies). Frozen baseline `9062b1e96`; branch `opus/throughput-recovery-150k-20260916`; nothing pushed.
Machine-readable manifest: `opus5_throughput_recovery_checkpoint_40k_20260916.json`.

## Measured result (vs the frozen 9062b1e96 stable snapshot)

| measure | baseline | now | delta |
| --- | ---: | ---: | ---: |
| strict meaningful exact bytes (credited) | - | - | **+44,224** |
| strict padded exact bytes (board) | 1,397,165 | 1,441,821 | +44,656 |
| exact functions (board) | 7,279 | 7,335 | +56 |
| all-function-exact objects (board) | 309 | 314 | +5 |
| whole-object admission candidates | 8 | 12 | +4 |
| accepted ledger | 7,315 / 1,347,469 | 7,371 / 1,391,693 | +56 / +44,224 |
| active parks | 268 | 236 | -32 (31 reopened as strict exact, 1 re-measured fuzzy) |
| inherited regressions | - | - | **0** |
| coherent fuzzy bytes | - | - | 0 credited |

Newly all-function-exact objects: devices, rasterizer_xbox, rasterizer_xbox_widgets, rasterizer_xbox_screen_effect,
rasterizer_xbox_transparent_geometry.

## Commits since the +10K record

| commit | content | functions | padded |
| --- | --- | ---: | ---: |
| `c2593b885` | wave 3 (screen_effect, hs_compile, geometry, render_cameras, sound_manager) | 8 | 7,792 |
| `efb4624aa` | wave 4 (particle_systems, stack_walk_windows) | 2 | 3,232 |
| `4d6c7f99c` | wave 5 (vehicles, network_connection, transport endpoints; owner-unblocked units) | 11 | 5,392 |
| `abfb9abfe` | header packet: transport_endpoint.h char flags / long blocking / endpoint_blocking owner prototype | 4 | 1,008 |
| `6bcdbba5a` | symbols.json in-place rename `_code_00070420` -> `_net_startup_debug` (2001-09-25 map files) + transport.h prototype | 1 | 16 |
| `64fd27d28` | header packet: shader_base radiosity/physics nesting; `_rasterizer_transparent_geometry_group_draw` | 1 | 15,120 |

## Checkpoint verification

Full `ninja` after every packet; stable diff vs 9062: 56 gained, 0 regressions. `tools.parked_functions`: 236 active /
0 stale / 0 invalid. `audit_object_admission`: 12 candidates / 0 contradicted / 7 rejected / 0 revoked. pytest 1,151
passed, 5 skipped, 26 subtests. `git diff --check` clean per commit. Fake-match scan: 22 findings (= baseline, no new
rule/path). `_point_from_line3d` emitters: 7 (= baseline). Per-file audit of all 28 changed C files: guard pass
(real_math pre-existing), fake leads unchanged; owner deltas vs baseline are only January-defined string literals
(particle_systems, network_connection, screen_effect), January-defined `_net_startup_debug`, the disclosed A30-class
`_dot_product3d` select-any COMDAT in devices, and removals (players `_cosine`, vehicles' two invented static
carriers). Header packets were each measured board-wide by full ninja + stable diff; the shader packet's single drift
is the fuzzy park `__rasterizer_model_transparent_geometry_submit` (non-exact before and after, objdiff 89.129 ->
89.008), re-measured with evidence.

## Owner-disclosed judgement calls (landed)

- `_particle_system_update`: a block-top `= NULL` initializer whose value is never read is required (laws_w2 A10
  conditional-initializer form).
- `_rasterizer_transparent_geometry_group_draw`: two colour component loops are code-neutral where they sit and select
  the glass texscale operand order; component loops are proven elsewhere in the same function.
- Byte-proven non-UB original bugs kept under BUG comments: `_hs_parse_cond_recursive` (always-true negation test,
  also in HCEA), `_hs_parse_begin` (extra sprintf argument), chicago extra-layer loop that never advances.

## Rejected since the +10K record (never landed)

| function | reason |
| --- | --- |
| `_encounter_test_rule`, `_action_obey_command_begin`, `_action_charge_setup` | exact only with redundant stores |
| `_add_endpoint_to_set` | identical FD_SET branches |
| `_poll_endpoint_set` | negative-result clamp spelled to steer block order |
| `_render_camera_mirror`, `_infection_swarm_control`, `_weather_particle_update_physics`, `__rasterizer_environment_lightmap_draw`, `_update_alien_fighter_physics_new` | need an emitted `_point_from_line3d` COMDAT |
| `_actor_unit_control`, `_actor_action_try_to_dive`, `_ai_communication_find_actor_to_reply_to_player` | need units.h short impulse prototypes contradicting units.c (or costing two units.obj exact functions) |
| `_sound_set_definition_end` | header prototype contradicting its definition |
| `_biped_check_discard`, `_network_server_close_client_connection`, `_biped_update_physics` | varargs mismatch / out-of-bounds reads (UB) |
| `_biped_update_moving`, `_object_test_vector` | parenthesised-argument steering |
| `_biped_accelerate` | invented file-local helper body |

## Staged, not launched

No new wave is launched until the owner says so. Remaining portfolio: `scratch/tr/portfolio.json` minus landed rows.
