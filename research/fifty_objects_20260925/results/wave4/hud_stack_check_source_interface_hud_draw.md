# source/interface/hud_draw

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\cand\\interface\\hud_draw.h",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\cand\\interface\\hud_draw.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\P1_hud_draw_h_check_stack_buffer_inline_and_macro.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\hud_stack_check\\patches\\P2_hud_draw_c_shared_stack_frame.patch"
]

## production_changes
SHARED HEADER patch P1_hud_draw_h_check_stack_buffer_inline_and_macro.patch adds four things to hud_draw.h:
- `enum { STACK_BUFFER_LENGTH = 0x80, STACK_BUFFER_FILL = 0x62626262 };`
- the macro `#define match_assert_stack_frame(file, line) { short corrupt_index = check_stack_buffer(stack_buffer); match_vassert(file, line, return_eip==get_return_eip(), "corrupt return address!"); match_vassert(file, line, corrupt_index==NONE, csprintf(temporary, "corrupt stack at %d!", corrupt_index)); }`
- section markers
- `__inline short check_stack_buffer(long *buffer)`: the unchanged hud_draw.c body, placed in the header's public code section.

P2_hud_draw_c_shared_stack_frame.patch changes hud_draw.c:
- drops its plain check_stack_buffer definition, its TU-local STACK_BUFFER enum and its TU-local hud_draw_stack_buffer_check macro;
- the 14 sites become match_assert_stack_frame("c:\\halo\\SOURCE\\interface\\hud_draw.c", N);
- get_return_eip (naked) is unchanged.

P1 and P2 must land together; the header inline and the plain definition cannot coexist.

hud_draw.h consumer TUs for the full sweep (23; no header includes it; grep also covered paths with spaces):
- source/interface/: hud_draw.c, hud_messaging.c, hud_nav_points.c, hud_unit.c, hud_weapon.c, motion_sensor.c, ui_widget.c
- source/objects/widgets/lightning.c
- source/rasterizer/: rasterizer_debug.c, rasterizer_lights.c
- source/rasterizer/xbox/: rasterizer_xbox.c, rasterizer_xbox_debug.c, rasterizer_xbox_dynavobgeom.c, rasterizer_xbox_environment.c, rasterizer_xbox_environment_fog.c, rasterizer_xbox_models.c (P7 removes its include), rasterizer_xbox_screen_effect.c, rasterizer_xbox_shadows.c, rasterizer_xbox_transparent_geometry.c, rasterizer_xbox_water.c, shader_transparent_generic_preprocessor.c
- source/render/: render_contrails.c, render_sprite.c

The canaries _bitmap_copy (bitmap_drawing.c), _rasterizer_frame_statistics_draw and _get_edge_vertex (geometry.c) do not include hud_draw.h.

## evidence
Harness control: a verbatim header copy in the slug with an include-swapped TU copy is objcmp-identical for all 23 consumers.

Full packet sweep (h2_out), objcmp covers every owned section, anonymous sections except .debug$S, and symbol sets:
- hud_draw: 0 differences, 22 EXACT / 1 unwritten (_fast_ftol_C) as before.
- The emitted _check_stack_buffer is now SELECT_ANY. It is section_infos_equal to January's selected section (48 B, 0 relocs, sha c7049c3184d2ed3e, COMDAT checksum 0x4957c551 unchanged).
- object_audit text is identical for production and candidate.
- 18 non-sentinel consumers: 0 differences, provided P7 is applied.
- /W3 warnings: unchanged.

Bisect of the v1 header on models:
- enum only: EXACT; macro only: EXACT.
- inline, with or without the enum, at any position: models residual.

## blockers
Whole-object hud_draw admission is still blocked by pre-existing items outside this packet:
- _fast_ftol_C is UNWRITTEN (COMDAT tail-padding / hud_draw_meter inline trade, per the n4 ledger);
- the hud_draw _fast_ftol candidate-only rejection;
- surplus _square_root.

## reopen_criteria
Unchanged pre-existing hud_draw items: _fast_ftol_C placement/padding and the _fast_ftol veto.

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
