# source/interface/hud_nav_points

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\cand\\interface\\hud_nav_points.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\P4_hud_nav_points_c_shared_stack_frame.patch"
]

## production_changes
P4_hud_nav_points_c_shared_stack_frame.patch:
- Both goto sentinel blocks become match_assert_stack_frame("c:\\halo\\SOURCE\\interface\\hud_nav_points.c", 675); and (..., 496);. The labels were corrupt_stack_found_custom_render_nav_point/stack_buffer_checked_* and corrupt_stack_found/stack_buffer_checked.
- `long stack_buffer[0x80];` becomes `long stack_buffer[STACK_BUFFER_LENGTH];` (x2).
- Depends on P1.

## evidence
Full-packet sweep:
- gate rows unchanged: 31 EXACT / 1 residual (parked _custom_render_nav_point, bytes identical, no park drift).
- objcmp: the only difference is the new external ANY _check_stack_buffer, which is IDENTICAL to January's selected hud_draw copy. Its pair link against the packet's hud_draw.obj passes in both orders.
- All other surplus: IDENTICAL.
- object_audit: only the added surplus line.
- /W3 warnings: unchanged.

## blockers
Whole object is still blocked by the parked _custom_render_nav_point (pre-existing). The sentinel no longer contains invented gotos.

## reopen_criteria
The existing park criteria for _custom_render_nav_point.

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
