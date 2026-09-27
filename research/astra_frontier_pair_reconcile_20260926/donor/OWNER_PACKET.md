# Owner packet: remaining-frontier lane (2026-09-26)

Every item below is measured and waiting on an owner decision; nothing here is applied. Numbers are strict-exact
padded bytes from full or keyed measurements in this lane (worker report cited). "Canary" = an inherited exact
function that flips only through the C1-number / declaration-count mechanism (memory: vc7-c1-number-mod64-law,
declaration-count-oracle), not through any change to its own source.

## A. Exact functions waiting only on a ruling
| # | Function | Bytes | What it needs | Gain / loss | Evidence |
|---|---|---:|---|---|---|
| A1 | `_player_teleport_internal` (players.c) | 1,296 | Form A: `static` definition under the existing extern prototype in players.h (C4211 at /W4; recorded owner-gated by fifty-objects) | +1,296 / 0 | workers/RF-K/REPORT.md (o1 body + /Od if/else + static; Sept/Oct retail both match; January storage file-static per cachebeta publics and HCEX) |
| A1' | same | 1,296 | Form B'': both January-static functions' prototypes moved out of players.h (rule-16 form) | +1,296 / -4,176 (`_rasterizer_frame_statistics_draw`, canary) | RF-K follow-up; B' (teleport only) loses 2 |
| A2 | `_compute_ground_plane` + `_bsp3d_test_sphere_recursive` | 336 + 800 | Remove the decorative outer parentheses at real_math.h:1533 (`return (dot_product3d(...) - plane->d);`) | +1,136 / -5,792 (`_rasterizer_frame_statistics_draw` 4,176, `_item_accelerate` 944, `_ai_communication_update_speech_timers` 672; all canaries) | workers/RF-O/REPORT.md (557-unit consumer sweep) |
| A3 | `_player_effect_add_continuous_effect` | 320 | /Od named `periodic` local + Marathon `CEILING(n,ceiling)` spelling in cseries.h | +320 / -4 functions (`_path_state_traverse`, `_rasterizer_transparent_geometry_group_draw`, `_update_human_boat_physics`, `_update_human_plane_physics`) | workers/RF-J/REPORT.md (84-unit sweep) |
| A4 | `_real_random_range_evaluate` | 96 | Authorise a one-compile held-form lab: `(arguments[1].real_value)` (decorative parentheses) | unknown (not compiled) | workers/RF-D/REPORT.md (forced-key oracle proves one hidden record decides it) |
| A5 | `_render_weapon_hud` | - | Rule-27/28 ruling on a per-site /Od-attested integer view of `numbers_real[]` (HW-C1); then allocator + slot-ID residue remains | not exact yet | workers/RF-J/REPORT.md |
| A6 | `_virtual_keyboard_render_internal` | - | Rule-22 ruling on separate identical switch arms with per-arm fetch (V07; reproduces Oct retail) | not exact yet (25 hunks) | workers/RF-K/REPORT.md |
Question on the canaries (A1', A2): `_rasterizer_frame_statistics_draw` is lost by three independent genuine header
corrections. Is its exactness a C1-number coincidence the owner will accept trading under rule 62 (explicit approval,
disclosed rows, debited credit), or does it stay a hard block?

## B. Review items on landed commits (lane-verified; the owner may prefer an alternative)
| # | Commit | Item | Alternative |
|---|---|---|---|
| B1 | 9ed00bf7 `_actor_path_refresh` (reconciled upstream as 9c9118d1) | descriptive flag name `build_path` | P03 adds the byte-inert /Od `if (!path_available) success = FALSE; else` nesting (workers/RF-A/actor_path_refresh_P03.patch) |
| B2 | 9f0b3af9 `_display_scenario_help` (reconciled upstream as 81a11ff9) | redundant conjunct `text_box && text_box->type == ...` in a message-only assert | A+B form with the /Od empty-body loop header (workers/RF-E/display_scenario_help_AB_alternative.patch) |
| B3 | b659a777 `_widget_instance_render_recursive` | `0xFFFFFFFF` spelling of the attested -1 colour; effect runs through a VN ordinal (rules 21/36 disclosure) | `-1` or `NONE` spelling (not measured; expected byte-identical since the value is the same constant) |
| B4 | 06645ef4 `_player_effect_update_camera_impulse` (park retired) | `realcmp` at a site without direct attestation (macro first-party; rule 12; Sept/Oct retail match only with it); effect via VN ordinal | keep the hand-expanded form (residual) |

## C. Research facts for existing holds (information)
- /RTCu attests uninitialised declarations in the /Od build: `_effect_allowed_by_environment` `allowed`
  (0x56e50b/0x56e5b0), `_ai_test_line_of_sight` `collision_t` (0x48aee6/0x48b4d6/0x48b50c) (workers/RF-H/REPORT.md).
- /Od `cross_product3d` has two locals where real_math.h has three; it explains `_physics_update_old`'s x87 order but
  costs `_particle_system_new_particle_jet` unless that function's /Od dead pointer local is admitted
  (workers/RF-C/REPORT.md); /Od `rotate_vector2d` has the same one-local shape (workers/RF-M/REPORT.md).
- `_ai_test_ballistic_line_of_fire`'s frame is decided by C1 numbers set inside `_ai_test_line_of_sight`, whose /Od
  single-exit form is held for its uninitialised `collision_t` (workers/RF-B/REPORT_W4.md).
- Decoded, oracle-proved single decisions with no attested source fact yet (reopen criteria in each report):
  `_cinematic_render` (VN parity), `_bsp3d_test_sphere_recursive` / `_biped_accelerate` (VN windows),
  `_poll_ep_array_compare_proc` (allocator multiplier), `_rasterizer_bitmap_new` (post-MARK cross-jump),
  `_ui_check_for_pause_game` (47/47 tie), `_real_random_range_evaluate` (hidden 0x267 key).
