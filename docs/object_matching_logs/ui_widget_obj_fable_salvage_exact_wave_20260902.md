# `ui_widget.obj` Fable salvage exact wave (2026-09-02)

## Result

- Recovered and reconciled the unfinished Fable 5 `ui_widget.c` wave onto canonical commit `6ad1964cf`.
- Strict result: **53 exact / 3 residual / 46 unwritten** of 102 target functions.
- Canonical comparison: **30 newly exact functions, zero exact-function losses**.
- Strict padded-byte gain: **2,896 bytes** (`730,979` to `733,875` campaign-wide when combined with the already-integrated HUD update pair).
- Data and every unrelated object retain their canonical exact sets.

## Newly exact functions

- `_display_error_abort_to_dashboard`
- `_display_error_damaged_media`
- `_display_error_deferred`
- `_display_errors_deferred_until_cinematic_stop`
- `_dispose_widget_stack`
- `_filesystem_initialization_thread_proc@4`
- `_main_menu_screen_is_active`
- `_main_screen_shell_begin_fade`
- `_modulate_pixel32_by_real_alpha`
- `_network_game_reset_to_pregame_ui`
- `_perform_filesystem_initialization`
- `_play_sound_tag`
- `_pool_alloc`
- `_pool_free`
- `_pop_widget`
- `_should_flip_sticks_for_local_player`
- `_ui_play_audio_feedback_sound`
- `_ui_set_next_level`
- `_ui_start_main_menu_music`
- `_ui_stop_main_menu_music`
- `_ui_widgets_active`
- `_ui_widgets_active_for_local_player`
- `_ui_widgets_close_all`
- `_ui_widgets_close_all_for_local_player`
- `_ui_widgets_delete_history`
- `_ui_widgets_dispose`
- `_ui_widgets_initialize`
- `_ui_widgets_pop_stack`
- `_widget_instance_get_child_index_from_parent`
- `_widget_instance_get_topmost_parent`

## Evidence and provenance

- Source reconstruction was recovered from the exhausted Fable lane at `C:\halo-worktrees\fable-small-families-20260901` and preserved independently on `root/fable-small-families-salvage-20260902` before reconciliation.
- Function bodies and layout were checked against January target disassembly and the campaign's function/relocation gates.
- Names and declarations were corroborated against the existing HCEA/PDB-derived symbol inventory and subsystem definitions already present in the tree.
- The `__stdcall` worker uses the target-authenticated decorated private symbol `_filesystem_initialization_thread_proc@4`.
- The previously anonymous BSS prefix is named `ui_widget_globals_storage`, reflecting its recovered `widget_globals` and `we_are_at_the_main_menu` contents and established subsystem naming.

## House-rule reconciliation

- Removed cross-module prototype copies from `ui_widget.c`; declarations now live in their owner or closest associated headers.
- Kept UI-private recovered structure definitions local rather than perturbing broad shared headers. The cross-subsystem 24-byte `game_input_preferences` type now lives in its narrow `input_abstraction.h` owner after the authenticated `player_ui` caller recovery required it; the combined full-tree rebuild preserved all 53 UI exact functions.
- Added the narrow Xbox shell owner header `shell_xbox.h`; putting the declaration in broad `shell.h` changed an unrelated target data owner under the C2 declaration-position oracle.
- Corrected profile enumeration counts to the owner-defined `word` type and retained `real` rather than raw `float`.
- Kept private functions semantic and target-authenticated; no `code_<address>` names remain.
- Replaced the anonymous `bss_<address>` source/config name with a semantic subsystem name.
- Added no anchors, forced ABI, volatile steering, fake calls, fake conditionals, forced inline/noinline directives, pragmas, or assembly.
- Public no-argument prototypes use `void` on its own line; parameters use one line each; recovered function bodies have explicit returns.

## Verification

- `ninja all_split all_source progress semantic_progress`: pass.
- Full split-object exact-set comparison against canonical: **0 lost / 30 gained** for `ui_widget.obj`; no other losses.
- Strict board after the wave: **278/619 objects, 733,875/1,922,413 padded bytes, 5,120/8,245 functions**.
- Semantic report: 473 units scanned, 5,293 functions evaluated, 5,133 semantic-exact, 5,146 accepted-exact, 0 unit errors.
- Remaining residuals are honestly classified: `_get_icon_type` (SHA), `_main_screen_shell_load` (relocation identity), and `_remap_sticks_for_local_player` (SHA).

## Do not repeat

- Do not move the recovered TU-private layout types into broad headers merely for tidiness; C2 allocation is declaration-position sensitive.
- Do not rename away the `@4` decoration in the target config; the undecorated alias falsely leaves the real function unwritten.
- Do not paper over the three residuals with anchors, dead branches, fake calls, or relocation aliases without independent source evidence.
