# source/interface/hud_draw

## decision
OWNER

## est_probability
0.85

## route
Owner ruling on the held 'hud_draw optimizer pragma': put a tightly bracketed #pragma optimize("", off)/on around fast_ftol_C only. The ai.c precedent is admitted. The 150k_w1 v2.c measurement gives _fast_ftol_C 56/0 EXACT, keeps _hud_draw_meter EXACT, and the whole-TU gate reaches 23/23. Two other fixes are mechanical: retire the stale candidate-only-comdat-owner rejection for _fast_ftol (round-1 provider_repair: actor_combat now emits ANY and all copies are identical), and nothing else blocks the object.

## why_new
This is a new census, not a new byte route. build/split has exactly two non-16-byte code sections board-wide: _fast_ftol_C (56) and _ai_handle_spatial_effect (1130). build/base has exactly one of 8,918: ai.obj _ai_handle_spatial_effect (1130), and that function sits inside ai.c's #pragma optimize("", off) region. So code in an /Od region is the only thing VC7 leaves without nop padding. This corrects the n1 ledger's claim that ai had zero non-16 COMDATs, and it independently supports the CC-tail signature (150k_w1). The Sept-2001 map's 'i hud_draw.obj' flag and January's 12 out-of-line calls in hud_draw_meter are consistent with it.

## prior_negatives_checked
hud_draw_obj_opus5_next150_n1 and n4: an __asm body at /O2 is 64 bytes and inlines into meter (1312 -> 1536); a naked form is still 64; position in the file is inert; (long) casts call __ftol2. hud_draw_obj_opus5_150k_w1 sec.4 and v2.c: the pragma route works. Round-1 provider_repair_source_interface_hud_draw, hud_stack_check and REVIEW__source_interface_hud_draw. Production checks at f6d00a8c: object_audit FAIL(2), both lines _fast_ftol_C; provider_link PASS; surplus_identity 2/2 IDENTICAL; pdb_storage 0 disagreements.

## notes
If the owner approves, the object completes. Nothing else is outstanding: data, symbols and surplus all pass. I did not re-probe the held pragma, as the brief requires. Evidence is in scratch/w/triage_hud_interface/LEDGER.md and non16.py/non16b.py.