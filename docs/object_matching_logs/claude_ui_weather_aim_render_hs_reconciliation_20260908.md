# Claude UI, Weather, AimAssist, RenderDebug, and HS reconciliation

Date: 2026-09-08

## Admission verdict

This batch reconciles the strongest admissible work from Opus lane
`opus/30k-ui-script-profiles-20260906` at `08d6bea92` and Fable lane
`fable/50k-resume-20260906` at `740bbc33a` onto canonical commit
`24092ea0f88ddd27fd76b52f953f8bbc3403277f`.

The canonical progress ledger advances from **965,130 to 975,317 meaningful
exact code bytes** and from **6,427 to 6,467 credited functions**: **+10,187
meaningful exact bytes / +40 credited functions net**. Data remains
**2,150,145 exact bytes** and the object count remains **390/833**. No fuzzy
body is included in those totals.

The independent strict-section snapshot records 45 newly exact sections and
three deliberately debited sections. The gross strict gain is 12,706
meaningful / 13,024 padded bytes; the debit is 394 meaningful / 432 padded
bytes, for 12,312 meaningful / 12,592 padded bytes net and 42 net strict
sections. This strict snapshot and the canonical progress ledger intentionally
measure different populations; neither total is substituted for the other.

The exact packet was replayed from the clean `24092ea0f` tree in
`scratch/packet-replay-24092-20260908a`, with only the intended patch applied.
The clean replay and the shared integration tree produce the same final
progress totals. This excludes unrelated uncommitted lane work from the claim.

## Strict exact gains

| Object | New exact owners | Meaningful | Padded |
| --- | ---: | ---: | ---: |
| `weather_particle_systems.obj` | 9 | 2,091 | 2,176 |
| `aim_assist.obj` | 7 | 1,391 | 1,456 |
| `hs.obj` | 1 | 116 | 128 |
| `ui_widget.obj` | 24 | 7,638 | 7,760 |
| `render_debug.obj` | 3 | 974 | 1,008 |
| `error_geometry.obj` | 1 | 496 | 496 |
| **Gross** | **45** | **12,706** | **13,024** |

The admitted exact owners are:

- Weather: `weather_particle_system_wrap_point`,
  `weather_particle_system_new`,
  `weather_particle_system_box_offset_from_point3d`,
  `weather_particle_system_build_clipping_planes`,
  `weather_particle_system_transform_clip_planes_to_box`,
  `weather_polyhedra_find`,
  `weather_particle_system_update_particle_count`,
  `weather_particle_system_update`, and
  `weather_particle_systems_render`.
- AimAssist: `compute_attenuation`, `compute_composite_attenuation`,
  `unit_get_aim_assist_parameters`, `compare_targets`,
  `autoaim_compute_target`, `find_aim_assist_targets_recursive`, and
  `aim_assist`.
- HS: `hs_tokens_enumerate_add_string`.
- UIWidget: `column_list_update`, `draw_string_and_hack_in_icons`,
  `event_handler_dispatch`, `get_icon_type`, `process_ui_widgets`,
  `push_widget`, `render_state_bitmap`, `render_state_text`,
  `spinner_list_update`, `string_has_icons_to_draw`,
  `ui_widget_add_child`, `ui_widget_delete`,
  `ui_widget_delete_children_recursive`,
  `widget_instance_can_receive_events`,
  `widget_instance_find_by_tag_index`,
  `widget_instance_get_cumulative_alpha_modifier`,
  `widget_instance_get_tail_child_widget`,
  `widget_instance_go_back_to_previous`,
  `widget_instance_render_spinner_list`,
  `widget_instance_render_text_box`,
  `widget_instance_set_focused_child_by_index`,
  `widget_instance_tab_to_next_valid_widget`,
  `widget_instance_tab_to_previous_valid_widget`, and
  `widget_instance_update_animation_parameters`.
- RenderDebug: `build_height_matrix`, `render_debug_cylinder`, and
  `render_debug_pill`.
- ErrorGeometry: `error_geometry_polygon`. Its already-recovered natural body
  becomes exact under the admitted authentic header context. The obsolete
  fuzzy park is removed only after strict equality is remeasured; there was no
  source-body edit or exception credit.

Every row above passes normalized byte, section-size, and ordered relocation
comparison. Semantic address aliases are not used to convert a code mismatch
into an exact claim.

## Deliberate debits and zero credit

Three inherited strict sections are no longer counted:

| Object / owner | Meaningful | Padded | Reason |
| --- | ---: | ---: | --- |
| `decals.obj::decals_delete_permanent_from_cluster` | 392 | 400 | Restoring `structure_clusters_in_cone` to the Structures owner header changes only two adjacent independent VC7 stack loads. The source body, size, and all 29 relocation rows remain unchanged. It is recorded in `config/parked.json` as fuzzy with zero exact credit. |
| `ui_widget.obj::widget_instance_reload_recursive` | 1 | 16 | January COFF proves private/static linkage. Correcting the source from external to static lets VC7 eliminate the currently unreferenced empty body, so it is honestly `UNWRITTEN`, not credited as an external no-op. |
| `ui_widget.obj::ui_widget_reload_by_tag` | 1 | 16 | Same target-static proof and honest de-credit as the preceding owner. |

No declaration shuffling or function-body tuning is retained to recover these
fragile code-generation states. The four actually relevant cone/sector
prototypes are kept in `real_math.h`; six unused reconstructed declarations
are deferred because their unverified declaration context flips an unrelated
Rasterizer exact section.

## Best complete fuzzy bodies retained

These bodies are useful reconstruction progress but remain residual and earn
**zero exact credit**. Sizes below are target/candidate padded bytes. A single
size means both are equal.

### UIWidget

| Owner | Padded bytes | Current boundary |
| --- | ---: | --- |
| `display_error` | 896 | normalized code schedule |
| `display_scenario_help` | 608 | normalized code schedule |
| `draw_bitmap_in_rect` | 1,168 | normalized code schedule |
| `render_ui_widgets` | 560 | 28/29 relocations |
| `render_ui_widgets_postgame` | 384 | 6/7 relocations |
| `search_and_replace` | 464 | normalized code schedule |
| `ui_check_for_pause_game` | 672/640 | size and relocation topology |
| `ui_widget_launch_widget` | 304 | normalized code schedule |
| `ui_widget_load_by_name_or_tag` | 544 | normalized code schedule |
| `ui_widget_load_children_recursive` | 672/688 | size and code schedule |
| `widget_instance_initialize` | 496 | normalized code schedule |
| `widget_instance_process_one_event_recursive` | 2,016/1,984 | size and relocation topology |
| `widget_instance_render_column_list` | 176/160 | size and code schedule |
| `widget_instance_render_recursive` | 752 | normalized code schedule |
| `widget_instance_text_box_is_focused` | 64 | normalized code schedule |

### Weather

| Owner | Padded bytes | Current boundary |
| --- | ---: | --- |
| `weather_particle_system_new_particle` | 704 | normalized code; all 41 relocation rows agree |
| `weather_particle_update_physics` | 688 | x87/scheduling and relocation offsets |
| `weather_particle_system_render` | 1,680 | scheduling and relocation offsets |

### AimAssist

| Owner | Padded bytes | Current boundary |
| --- | ---: | --- |
| `aim_assist_compute_target` | 416 | normalized code schedule |
| `find_aim_assist_targets` | 304 | normalized code schedule |
| `limit3d` | 96 | naturally emitted helper schedule |
| `local_player_aim_assist` | 368 | normalized code schedule |
| `object_compute_autoaim_target` | 416 | x87 helper schedule; the stronger `point_from_line3d` form is rejected |

`player_aim_projectile` and `reciprocal_square_root` remain unwritten and are
not presented as recovered work.

### RenderDebug

| Owner | Padded bytes | Current boundary |
| --- | ---: | --- |
| `build_circle_points` | 112 | pointer-anchor/compiler choice |
| `build_pill_points` | 448 | commuting-operation/compiler choice |
| `render_debug_circle` | 304 | pointer-anchor/compiler choice |
| `render_debug_sphere` | 400 | commuting-operation/compiler choice |

The pre-existing `render_debug_add_cache_entry` and `render_debug_point`
parks remain fuzzy and unchanged. The lists above are the canonical record of
the newly retained Claude bodies; future work must remeasure them rather than
assuming the donor's reported status.

## Ownership, names, and house-rule audit

- All recovered private Weather, AimAssist, HS, and UI data owners use semantic
  names in `config/symbols.json`; no `code_<address>`, `data_<address>`, or
  `bss_<address>` source name is admitted.
- January COFF storage classes determine private versus external linkage.
  `aim_assist`, `aim_assist_compute_target`, and
  `aim_assist_clear_line_of_sight` are declared in `aim_assist.h` as external;
  the two UI reload helpers remain private/static.
- Weather owns `extern boolean weather` and its lifecycle/render prototypes in
  `weather_particle_systems.h`. Render owns
  `extern boolean render_particle_systems_enabled` in `render.h`.
  `hs_globals_external.c` consumes those typed owners instead of incompatible
  byte-array declarations.
- The HS enumeration storage is split into six source-static named owners at
  target offsets 0, 4, 8, 12, 16, and 17:
  `hs_enumeration_result_count`, `hs_enumeration_maximum_count`,
  `enumeration_results`, `hs_enumeration_substring`,
  `hs_recompile_pending`, and `hs_syntax_data_allocated`. Explicit zero
  initialization is required for the exact 18-byte layout. The target literal
  `!enumeration_results` independently supports the central pointer name.
- Typed tag access goes through subsystem macros and typed object access uses
  the established object macros. Project `real`, `word`, named enum values,
  and cseries flag macros replace generic float/unsigned-short/manual-bit forms
  introduced by donors.
- Owner declarations were moved to `hs.h`,
  `ui_widget_event_handler_functions.h`, `render_debug.h`, `real_math.h`, and
  `structures.h`; no consumer-local prototype facade is retained.
- UIWidget keeps its still-incomplete private widget layout in the owning `.c`
  file because the present public definition header is partial and
  incompatible. This avoids perturbing unrelated translation units while
  preserving semantic fields rather than raw offsets.
- No new inline assembly, forced inline/noinline, volatile/register steering,
  compiler pragma, fake dependency, representation-punning match trick, or
  nonsensical branch is admitted. No `/QIfist` change was needed.
- The January inline schedule remains intact, and all five candidate unit
  guards prove that no `point_from_line3d` owner is emitted.

## Verification record

- Clean replay from `24092ea0f` plus the intended packet: full `ninja` build,
  semantic report, parked-evidence validation, and progress report pass.
- Shared-tree full build produces the same 975,317-byte / 6,467-function
  canonical result.
- Stable snapshot: 8,245 target functions, 6,514 strict exact sections; 45
  gains and three explicit debits relative to the baseline snapshot.
- Per-unit PID-safe gates: Weather 17/20 exact, AimAssist 9/16 exact,
  RenderDebug 22/36 exact, UIWidget 85/102 exact, HS 446/448 exact.
- All fuzzy bodies remain ordinary residuals; no semantic allowlist entry was
  added for this batch.
- Canonical tooling suite: 1,140 passed, two skipped, and 26 subtests passed.
