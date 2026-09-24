# source/interface/hud_weapon

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\cand\\interface\\hud_weapon.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\P5_hud_weapon_c_shared_stack_frame.patch"
]

## production_changes
P5_hud_weapon_c_shared_stack_frame.patch:
- removes the TU-local goto macro hud_weapon_stack_buffer_check (labels inside a macro) and the HUD_WEAPON_STACK_BUFFER_LENGTH/FILL enum constants;
- the 5 sites become match_assert_stack_frame("c:\\halo\\SOURCE\\interface\\hud_weapon.c", 0x3A2 / 0x19E / 0x4E2 / 0x308 / 0xD4);
- stack_buffer arrays use STACK_BUFFER_LENGTH;
- depends on P1.

## evidence
Full-packet sweep:
- gate 13 EXACT / 3 residual, unchanged; all residual bodies byte-identical.
- objcmp: the only difference is the new ANY _check_stack_buffer, IDENTICAL to January's copy. Pair link PASS in both orders.

Subset measurement (P1 without P5): hud_weapon's residual _render_weapon_hud bytes DRIFT through the declaration count. P5 is therefore required, not optional.

## blockers
Pre-existing residuals: _crosshairs_draw, _hud_update_weapon_local_player, _render_weapon_hud.

## reopen_criteria
Existing hud_weapon residual ledgers.

## task notes
**hud_unit (primary goal):** the packet makes source/interface/hud_unit an OBJECT_COMPLETE_CANDIDATE.

**What January's bytes settle:**
- build/split's COMDAT selection cannot tell a header inline from a plain .c function, because csplit writes NODUP for every code COMDAT.
- The decisive evidence is elsewhere:
  - _check_stack_buffer has zero relocations in any split object.
  - All 26 sites in 5 objects are identical inlined return-merges.
  - Each site uses a single line literal for both asserts (a macro).
  - The helper is PUBLIC in cachebeta.
  - The Sept-2001 map gives an 'i' (inline) flag to its sibling get_return_eip, with the 48-byte gap next to it.

**Packet:**
- hud_draw.h gets:
  - `__inline short check_stack_buffer(long *buffer)`, the unchanged body;
  - the STACK_BUFFER constants;
  - `match_assert_stack_frame(file, line)`.
- The 5 TUs use the macro. The invented gotos and the TU-local duplicates are gone.
- All 23 hud_draw.h consumers keep every section byte+reloc identical. The only exception is the new SELECT_ANY _check_stack_buffer in hud_unit, hud_nav_points, hud_weapon and motion_sensor, which is identical to January's selected copy and pair-links in both orders against the packet's hud_draw.obj.
- This holds only with P7, which removes rasterizer_xbox_models' dead hud_draw.h include. Without P7, the inline's declared names flip __rasterizer_model_transparent_geometry_submit.

**Integration:**
- The set is atomic: P1 and P2 cannot be split, and P3-P6 also depend on P1. The P4-P6 edits are required too, because without P5 hud_weapon's residual _render_weapon_hud drifts.
- After applying, the integrator must run a full ninja sweep plus a parks check on the live tree. Other wave-4 header packets can interact through declaration counts.
- Two tracked files would need CRLF-safe handling: symbols.json (the slug copy keeps CRLF) and object_admission_rejections.json.
- In this lab, repo provider_link.py reports FAIL for _check_stack_buffer only because build/base still holds the production NODUP hud_draw.obj. After the rebuild it should pass; provider_link_override.py demonstrates this.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_stack_check\LEDGER.md (sections R0, F1, F2, B0, C0, H1, H1b, K-oracle, L1, S1, S2, A1-A5, H2, P-A, RESULT)
