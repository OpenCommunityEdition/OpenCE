# RF-E wave 2 evidence (lane HEAD 1b00eeea) - written 2026-09-26 16:17:11 -0700

Baseline: scratch copy of HEAD ui_widget.c gates 97/5/0; keyed diff vs rebuilt build/base ui_widget.obj: 0 changed.

## _widget_instance_render_recursive (743 B): improved-not-exact, ONE instruction from January
Patch (research only, zero credit): render_recursive_RR2_improved_not_exact.patch.
Changes, each first-party attested:
 A (RR-1) argb argument = modulate_pixel32_by_real_alpha(0xFFFFFFFF, alpha) replacing `alpha *= 255.0f` +
   `(fast_ftol(alpha) << 24) | 0x00FFFFFF`. /Od 0x66a020 +0x31d: push alpha; push -1; call 0x6627d0; 0x6627d0 =
   our EXACT _modulate_pixel32_by_real_alpha (od_dis.py). January defines that public helper and NO January object
   calls it (0 relocations) - consistent with every call site inlined (/O2). January's bytes = the inline
   (fmul 255.0 folded from 0xFFFFFFFF>>24, named-real store/reload, fistp, shl 24, or 0xFFFFFF).
 B (structural R1, recorded alone) clip handled through the pointer: if (clip) { clipped = *clip; clip = &clipped;
   clip->x0/x1/y0/y1 += ... } (/Od +0x220..+0x2a4, RTC aggregate "local_clip").
Results: A alone: clip moves into the dead widget home [ebp+8] (January) but the phi stays a register; B alone
(recorded): phi memory-homed but alpha takes [ebp+8]. A+B: EVERY frame slot equals January (alpha_modifier -4,
bounds -0xc, clipped -0x14, alpha -0x18, clip [ebp+8]); the only difference left is the game-data-input loop
`mov dx,[ecx+eax]` (January) vs `[eax+ecx]` (ours).
gate --all 97/5/0; keyed diff vs build/base: 1 changed (render_recursive, residual), 0 added/removed; /W3 17 = 17
identical multiset; fake_match_scan 0 leads.
Remaining difference, traced (sortdump.py, C2 0x1070d420/0x1070d42d): the pre-marker commutative sort of
`address + input_index*0x24` orders [MUL key 0x10201d7 (input_index id 11), memory key 0x1020185 (definition id 6
base)]. Every January-consistent source first-references definition, alpha_modifier, render_children and the two
offset fields before the loop, so the decoded key predicts the SAME pre-marker order in January; the base/index swap
is decided later (addressing-mode formation after strength reduction), not yet traced. Diagnostics RR-X1 (early
initialiser: IL changed, unclean), RR-X2/RR-X3 (evaluation-free sizeof references: ids do not change) settled
nothing about that later stage.
Reopen: trace the strength-reduced load's operand order (the IV and address temp) from SR to the encoder; a source
fact must be attested before any compile.

## _ui_widget_launch_widget (300 B): NEGATIVE (mechanism decoded further; no compile beyond 2 diagnostic readouts)
- At the current definition position the entry load differs (widget loaded before tag_get); the recorded fix is the
  J-position function-order move, which the brief lists as an owner hold.
- Every recorded diamond spelling pins -1. Oracle readout of the if/else form: the -1 constant web has one hoisted
  materialisation and five register co-operand uses (benefit -1 = not pinned, as in HEAD), and is lifted to +1 by
  two spill bonuses (C2 0x107794ca) when the switch-arm phi-copy web (12 op268 copies, benefit -18) and the
  parent-tag/child web (4 copies, -6) are skipped in round 1. January keeps both coloured (parent tag ebx,
  local_player_index edi), so January's IL has fewer phi copies on those webs.
- Reopen: a first-party source fact that removes the switch-arm phi copies (or the diamond's parent-tag copy) at the
  J position, checked with tools/oracle (popsdump/deftrace) before compiling; the move itself needs an owner ruling.

## _ui_check_for_pause_game: no compile spent in wave 2
The 47/47 tie (wave 1) needs one pressure unit in one block of the controller web's range. Examined /Od facts not
yet applied: `widget_name = NULL` before the network switch (predicted by the priority model to move the tie the
WRONG way if it survives to pricing, inert otherwise) and the /Od return flag (a 2020 semantic change). No attested
fact predicted to flip the tie; nothing compiled.
