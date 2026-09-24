# source/interface/hud_unit

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\cand01R\\hud_unit.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\cand01Ralt\\hud_unit.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\cand\\interface\\hud_unit.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\config\\object_admission_rejections.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\01RS_hud_unit_c_01R_plus_shared_stack_frame.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\01RaltS_hud_unit_c_01Ralt_plus_shared_stack_frame.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\P3_hud_unit_c_shared_stack_frame.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\02_symbols_json_hud_unit_names_static.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\03_config_json_hud_unit_matching.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\04R_admission_rejections_retire_hud_unit_fast_ftol.patch"
]

## production_changes
Apply as ONE atomic set, all in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\hud_stack_check\patches\: P1_hud_draw_h_check_stack_buffer_inline_and_macro.patch (shared header, separate), P2_hud_draw_c_shared_stack_frame.patch, 01RS_hud_unit_c_01R_plus_shared_stack_frame.patch, P4_hud_nav_points_c_shared_stack_frame.patch, P5_hud_weapon_c_shared_stack_frame.patch, P6_motion_sensor_c_shared_stack_frame.patch, P7_rasterizer_xbox_models_c_drop_dead_hud_draw_include.patch, 02_symbols_json_hud_unit_names_static.patch, 03_config_json_hud_unit_matching.patch, 04R_admission_rejections_retire_hud_unit_fast_ftol.patch.
- 01RS is review3's 01R plus P3 in one patch. Equivalent: 01R then P3 applied in sequence (verified).
- The minimal alternative is 01RaltS in place of 01RS.
- 04R replaces hud_unit_audit's 04, which no longer applies on 16542e46 because the rejection entry moved.
- Full files for the integrator:
  - cand01R\hud_unit.c (and cand01Ralt\hud_unit.c)
  - cand\interface\{hud_draw.h, hud_draw.c, hud_nav_points.c, hud_weapon.c, motion_sensor.c, rasterizer_xbox_models.c}
  - config\symbols.json: 4 in-place line edits, CRLF kept
  - config\object_admission_rejections.json
- git apply --check on 16542e46: every patch is clean alone, and the full set is clean together (also with 01RaltS).
- `patch` replay onto copies reproduces every slug file byte for byte.
- hud_unit.c change: both goto blocks (lines 0x201 and 0x3C9) become `match_assert_stack_frame("c:\\halo\\SOURCE\\interface\\hud_unit.c", 0x201);` (and 0x3C9), and the TU-local STACK_BUFFER enum is dropped because it is now in hud_draw.h.

## evidence
January bytes (scratch\w\hud_stack_check\sites2.py):
- All 26 sentinel sites across the 5 objects are the same inlined shape: a 0x7F..0 loop, then `or esi,-1`, `call _get_return_eip`, and an out-of-line `mov esi,eax; jmp back` return merge.
- Both asserts at every site carry one line literal. hud_unit uses 0x201 and 0x3c9; full list in the ledger F2. This is one shared macro.
- January's _check_stack_buffer (hud_draw section 2, 48 B) disassembles to `short check_stack_buffer(long *buffer)`. It has 0 relocations anywhere and is PUBLIC at 0xC08F0 in cachebeta publics.
- The Sept-2001 map flags _get_return_eip as 'i hud_draw.obj', with a 0x40 gap that exactly holds get_return_eip plus check_stack_buffer.
- build/split's COMDAT selection carries NO January information: every code COMDAT in build/split/source is NODUP (7,561 of them, including the proven header inline _fast_ftol), so NODUP vs ANY cannot be read from the split.

Results with the packet:
- gate vs production split: 20 EXACT plus 2 rows that only lack their new names.
- gate_split vs the emulated csplit (slug config; the control equals build/split 833/833, and only hud_unit.obj differs): 22/22 EXACT for both 01RS and 01RaltS.
- object_audit_split: PASS, 30 January symbols with 0 differing.
- Surplus: _fast_ftol and _check_stack_buffer, both IDENTICAL to January's selected copies (actor_combat and hud_draw).
- Compared with review3's cand_final_amended.obj, the only difference is the new ANY _check_stack_buffer.
- Pair links (plink.py and provider_link_override.py with the packet's hud_draw.obj as provider): 12/12 PASS in both orders.
- Control: the production NODUP hud_draw.obj gives LNK2005 in both orders, so P1 and P2 must land with this change.
- strict_emulate (objdiff 3.3.1 single-unit report plus the repo's semantic/admission pipeline): 22/22 functions, 6176/6176 code, 152/152 data; contradicted/rejected empty. With the old _fast_ftol veto kept the result is 'contradicted', so 04R is required.
- fake_match_scan on hud_unit: 0 leads. /W3 warning census: unchanged.

## blockers
None for hud_unit once the atomic set lands.

Owner/reviewer decision points:
- (a) P7 removes rasterizer_xbox_models' dead `#include "interface/hud_draw.h"`. models uses none of the header's names, and the removal is byte-inert on all 62 models sections. It is needed because the inline's roughly 9 declared names flip __rasterizer_model_transparent_geometry_submit. K-oracle, lab only: dummies alone are exact at K 0-3, 5, 27+; with the inline, exact only at K>=18.
- Fallback if (a) is refused: P-A, a focused header included only by the 5 sentinel TUs (lab name interface/hud_draw_stack_check.h), with models untouched. Measured objcmp-identical.
- (b) The macro name match_assert_stack_frame is descriptive. The /Od build has no sentinel and HCEX has none, so Bungie's own name is lost.

## reopen_criteria
If the owner rejects both P7 and a focused header: January's original hud_draw.h/models include topology or declared-name set, recovered from first-party evidence, that tolerates the inline in models.

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
