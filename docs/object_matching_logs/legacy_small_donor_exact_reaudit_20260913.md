# Legacy small donor exact re-audit (2026-09-13)

## Result

The small exact-only delta in the frozen Fable and Claude reports was checked
against the current source and the existing canonical admission history. None
of the eight claims is admissible under the current house rules. The packet
therefore contributes **zero exact functions and zero meaningful exact code
bytes**. No production C, header, configuration, target, or build output was
changed.

The rejected donor reports are:

- `C:/halo-worktrees/fable-exact-finish-20260829/build/semantic_report.json`;
- `C:/halo-worktrees/claude-breakable-surfaces-20260830/build/semantic_report.json`.

Together they expose 2,819 meaningful bytes that are absent from the current
accepted ledger only because the old exact source violates a proved ownership,
type-safety, or anti-steering boundary:

| Unit / owner | Meaningful bytes | Current disposition |
| --- | ---: | --- |
| `action_guard::_action_guard_perform` | 437 | Reject donor; retain typed 448-byte fuzzy body |
| `hud_sounds::_hud_play_sound` | 349 | Reject `volatile` local-pointer steering; retain natural fuzzy body |
| `actor_perception::_actor_perception_desire_prop` | 549 | Reject unauthenticated public-ABI carrier union; retain typed fuzzy body |
| `action_converse::_action_converse_perform` | 248 | Reject gameplay-function inline assembly; retain ordinary-C fuzzy body |
| `wind::_wind_variance_get` | 213 | Reject inactive-union representation pun; retain defined-C fuzzy body |
| `decals::_bsp3d_get_plane_from_designator` | 101 | Reject forced standalone duplicate; retain naturally emitted shared helper residual |
| `thread_win32::_get_mutex_from_pool` | 59 | Reject false cross-module datum type; retain resolved relocation-alias residual |
| `real_math::_pill_test_vector3d`, `_point_in_triangle3d` | 863 | Reject point/vector pointer casts; retain typed fuzzy bodies |

## Controlling evidence

- `action_guard_perform` is already rejected in
  `claude_remaining_donor_batch_20260908.md`; the genuine firing-position
  context and `path_state` ownership debit is measured in
  `hs_actor_owner_reconciliation_20260907.md`.
- `hud_play_sound` is covered by
  `hud_sounds_obj_source_authenticity_review_20260904.md` and the explicit
  349-byte policy debit in `input_hud_player_ui_followup_20260904.md`.
- `actor_perception_desire_prop` is covered by
  `actor_perception_obj_props_api_typed_followup_20260904.md`. January reuses a
  parameter home after its last use; that is not evidence for a union in the
  source-level public contract.
- `action_converse_perform` is covered by
  `action_converse_obj_jonas_ordinary_c_fuzzy_park_20260830.md`. The exact
  donor directly forces January's register selection with three assembly
  regions in gameplay code.
- `wind_variance_get` is covered by
  `wind_obj_jonas_defined_fuzzy_closeout_20260831.md` and
  `wind_obj_jonas_freshness_audit_20260830.md`. The only historical exact
  candidate reads an inactive union member.
- The Decals helper is covered by
  `claude_wave_small_exact_batch_20260831.md`,
  `opus_wave_canonical_reconciliation_20260902.md`, and
  `decals_obj_collision_caller_graph_fuzzy_20260908.md`. The current helper is
  emitted by the genuine collision caller graph; the old exact used macro
  aliasing plus a copied standalone body to manufacture ownership.
- The Thread result is covered by
  `thread_win32_obj_house_rule_correction_20260831.md`. The donor names the
  following translation unit's `char[256]` transport buffer as a mutex record
  solely to obtain csplit's relocation spelling.
- The Real Math pair is covered by
  `real_math_obj_jonas_safe_owner_restoration_20260826.md`,
  `real_math_obj_jonas_global_typed_boundary_20260830.md`, and
  `real_math_obj_jonas_helper_restoration_exact_20260831.md`. Independent HCEA
  bodies support the retained typed algorithms, not the exact donor casts.

No new evidence in either frozen donor report overrides these decisions. An
exact normalized section is not credited when its source invents a false
owner/type, relies on undefined representation access, or explicitly steers
the compiler.

## Focused verification

The PID-safe current-tree gates were rerun without configuring or writing
`build/`. All inherited exact owners remained exact:

- `action_guard`: 14 exact / 1 residual / 0 unwritten;
- `hud_sounds`: 0 exact / 1 residual / 0 unwritten;
- `actor_perception`: 32 exact / 6 residual / 6 unwritten;
- `action_converse`: 8 exact / 1 residual / 0 unwritten;
- `wind`: 7 exact / 2 residual / 0 unwritten;
- `decals`: 26 exact / 7 residual / 0 unwritten;
- `thread_win32`: 8 exact / 1 relocation-identity residual / 0 unwritten;
- `real_math`: 80 exact / 4 residual / 0 unwritten.

The forbidden `_point_from_line3d` emitted-symbol guard passed for every
applicable measured unit. `real_math.obj` itself currently emits the inherited
shared inline owner, so its ordinary 80/4 census was measured without treating
the function's own definition as a newly introduced caller-side COMDAT. No
Real Math source was changed by this audit.
