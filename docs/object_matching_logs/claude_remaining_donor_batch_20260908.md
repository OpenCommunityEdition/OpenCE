# Remaining Fable/Opus donor reconciliation (2026-09-08)

## Result

The remaining donor-only candidates from
`opus/30k-ui-script-profiles-20260906` and
`fable/50k-resume-20260906` were reviewed as one admission batch. Six natural,
house-rule-compliant functions were admitted and independently reproduced with
the current tree's isolated per-TU gate:

| Unit | Function | Meaningful | Padded |
| --- | --- | ---: | ---: |
| `action_flee.obj` | `_action_flee_begin` | 92 | 96 |
| `action_flee.obj` | `_action_flee_update` | 152 | 160 |
| `s3tc.obj` | `_EncodeBlockAlpha4` | 111 | 112 |
| `s3tc.obj` | `_EncodeBlockAlpha3` | 615 | 624 |
| `interface.obj` | `_interface_draw_bitmap_modulated_p32` | 345 | 352 |
| `interface.obj` | `_profile_graph_toggle` | 113 | 128 |
| **Total** | **6 functions** | **1,428** | **1,472** |

The meaningful sizes are the January function sizes from `build/report.json`,
not padded COMDAT sizes. All three gates passed the forbidden-emitted-symbol
check for `_point_from_line3d`, and the inherited exact functions remained
exact:

- `action_flee.obj`: 5 exact to 7 exact;
- `s3tc.obj`: 10 exact to 12 exact;
- `interface.obj`: 12 exact to 14 exact.

Moving `profile_graph_toggle`'s declaration from the consumer-local prototype
block in `hs.c` to its true owner, `interface.h`, left `hs.obj` at 446 exact and
2 residual functions. `EncodeBlockAlpha3` was likewise declared in its owner
header. The added AI code uses `_actor_panic_none` rather than a raw enum value.

The profile tables reconstructed alongside `profile_graph_toggle` are also a
complete exact data recovery: the current scratch `interface.obj` and January
target `.data` sections compare equal at 100,632 bytes, 18 relocations, and the
same normalized digest. This is useful recovered data but is not counted as
exact code.

## Admission evidence

`action_flee_begin` and `action_flee_update` implement coherent state-entry and
per-tick behavior using the established actor/unit accessors, game-time owner,
named state fields, and existing panic enum. They introduce neither guessed
layouts nor compiler-steering constructs.

The two S3TC encoders are complete block-compression algorithms. Their shifts
and masks implement the encoded bitstream and therefore have no suitable
`cseries.h` flag-macro replacement. `EncodeBlockAlpha4` overwrites every output
nibble; the removed pre-clear was redundant to the completed byte and was the
sole cause of the former residual. Both functions use the existing named S3TC
records and constants.

The interface functions and profile-table layout have unusually strong prior
evidence in
`interface_obj_opus_bitmap_family_and_data_20260906.md`: January disassembly,
the HCEA cross-build source, an exact same-compiler sibling, and the complete
January data section independently establish the bitmap topology, profile
record layout, table contents, linkage, and names. No debug-profiler stall
function was admitted in this batch.

## Rejected or parked donor-only verdicts

These donor verdicts receive zero exact credit and no production source was
copied:

- `action_guard_perform`: the donor invents enormous `unresolved[]` records,
  writes a raw offset inside one, and replaces semantic firing-position enums
  with numeric values. Its exact packet is not acceptable source.
- `pre_evaluator_combatmove`: the only useful code-generation delta is an
  unused `actor_get(actor_index)` call. This is side-effect-shaped compiler
  bait and not credible logic.
- `bitmap_2d_uncompress_from_mipmap`,
  `decals_delete_permanent_from_cluster`, and
  `solo_level_select_list_update_displayed_items`: current and donor function
  sources are identical. Recompiling the donor source with the current gate
  does not reproduce the donor snapshot's claimed new verdict, so these are
  stale-build artifacts rather than transferable source progress.
- `hs_evaluate_inequality`: the donor adds `double` staging values around
  project `real` data and expands assertions into raw
  `display_assert`/`system_exit` pairs. This violates the project types and
  assertion abstraction without adding credible semantics.
- `game_options_menu_update_text_desc` and
  `game_options_menu_update_pic_desc`: the donor makes `description_index`
  uninitialized on the no-focus path and then consumes it.
- `network_pregame_status_screen_update`: the donor tests a pointer produced
  by `&game->machines[index]` for null (always true) and replaces team enum
  constants with `0` and `1`.
- `widget_instance_reload_recursive` and `ui_widget_reload_by_tag`: January
  COFF marks both functions private. The donor removes `static` solely to force
  two otherwise-eliminated empty functions to be emitted. The existing source
  correctly retains private linkage and gives them no exact credit.
- `progress_bar_create_noise_texture`: the address-derived 16-byte target has
  no authenticated semantic name or implementation. The donor's guessed empty
  function is already rejected by
  `progress_bar_obj_base_layer_reconciliation_20260907.md`.
- `render_debug_profile_stall_tick`: the donor uses raw numeric switch cases,
  and the owning stall enum has not been recovered. It remains parked pending
  named enum evidence and reconstruction together with its sole caller.

This review deliberately did not tune any residual register allocation or x87
schedule. A candidate was admitted, rejected, or parked after one source audit
and one reproducibility gate.
