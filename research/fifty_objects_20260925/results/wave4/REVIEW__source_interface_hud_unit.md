# REVIEW source/interface/hud_unit

approve: False

## per_object
[
 {
  "unit": "source/interface/hud_unit",
  "approve": true,
  "issues": "Approve only with the amended packet (01RS, not 01RaltS). Results against the emulated split: 22/22 EXACT, audit PASS (30/30), surplus _fast_ftol and _check_stack_buffer IDENTICAL, provider link 12/12 in both orders with the packet's hud_draw.obj, strict emulation 22/22 + 152/152 data. Needs P1+P2+P7+02+03+04R in the same atomic set. 01RaltS keeps an /Od-contradicted invented goto: reject."
 },
 {
  "unit": "source/interface/hud_draw",
  "approve": true,
  "issues": "P2 is byte-identical on every section; the only changes are _check_stack_buffer NODUP->ANY and its emission order (now before _get_return_eip, while January's image has it after). The object is not being admitted; the order is an advisory for its own future admission."
 },
 {
  "unit": "source/interface/hud_nav_points",
  "approve": true,
  "issues": "P4 adds only the new ANY COMDAT (strictly exact caller: hud_update_nav_point_local_player); the parked custom_render_nav_point stays byte-identical. Optional hygiene: without P4 the object is byte-identical to production."
 },
 {
  "unit": "source/interface/hud_weapon",
  "approve": true,
  "issues": "P5 adds only the new ANY COMDAT (exact callers render_grenade_hud and hud_update_weapon). It keeps the unparked _render_weapon_hud residual byte-stable, which drifts under P1 alone, and removes the invented gotos from two EXACT functions."
 },
 {
  "unit": "source/interface/motion_sensor",
  "approve": false,
  "issues": "P6 newly emits _check_stack_buffer from three residual, parked callers only, which breaks owner ruling 20260921 #2 / law (i) (P1 class). Drop P6; motion_sensor is then byte-identical to production. Reopen when a sentinel caller is strict-exact or the owner admits the class."
 },
 {
  "unit": "source/rasterizer/xbox/rasterizer_xbox_models",
  "approve": true,
  "issues": "P7 removes a dead include; the object is RAW-IDENTICAL and warnings are unchanged. It is load-bearing only as count shielding against P1 (without it __rasterizer_model_transparent_geometry_submit goes EXACT to residual). Disclose this in the ruling request (precedent: duplicate-declaration removal)."
 }
]

## checks
All runs are my own, under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_hud_unit\. The tree was 16542e46 and git status was clean before and after.

(1) git apply --check on the current tree: all 12 patches are clean alone, as the full set, as the full alt set, and as the amended set.
(2) Patch replay with patch -p1 onto `git show HEAD:` copies (tree/, tree_alt/). Every patched file equals the worker's candidate files; 01RS equals cand01R and 01RaltS equals cand01Ralt.
(3) Consumer list from my own grep over source/ (all extensions, case-insensitive): 23 .c files, no header includes, no .c-includes-.c, no paths with spaces. The new names collide with nothing.
(4) My own sweep harness (tools/sweep.py: redirected TU copies, gate.py --all --out) plus a strict raw COFF comparer (tools/rawcmp.py: section order, bytes, relocs by target name, COMDAT selection, full symbol table; only $L numbers normalised) and an owner-keyed comparer (tools/keycmp.py).
- Control (verbatim header): 23/23 RAW-IDENTICAL to build/base.
- Full packet: the 17 non-sentinel consumers and models are RAW-IDENTICAL.
- nop7: models __rasterizer_model_transparent_geometry_submit goes EXACT to residual, so P7 is load-bearing.
- hdronly: hud_weapon residual drift.
- amend (drop P6): motion_sensor RAW-IDENTICAL.
- min (drop P4+P6): hud_nav_points identical.
- strict (P1,P2,01RS,P7 only): only unparked _render_weapon_hud bytes drift.
- Sentinel objects: the only change is the new ANY _check_stack_buffer; parked bodies are byte-identical.
- hud_draw: NODUP to ANY, plus a text-order swap.
(5) csplit (build/tools/csplit.exe) into the slug. The control config equals build/split 833/833. The emulated config (4 in-place symbols.json edits, CRLF kept, byte-identical to the worker's copy) differs only in hud_unit.obj.
(6) gate_split (gate.py rule plus storage) against split_emu: 01RS and 01RaltS each 22/22 EXACT, 0 storage mismatches.
(7) object_audit (slug copy, SPLIT_ROOT=split_emu): PASS for both. 30 January-owned sections ok, 30 January symbols with 0 differences. Surplus: .drectve, 5 pooled strings, 5 __real, _fast_ftol, _check_stack_buffer.
(8) surplus_identity (slug copy taking obj paths): hud_unit _fast_ftol is IDENTICAL to actor_combat and _check_stack_buffer to hud_draw. All surplus in nav/weapon/motion/hud_draw is IDENTICAL.
(9) provider_link (slug copy with provider override; Link.Exe temp dir inside the slug):
- With the packet's hud_draw.obj: 12/12 PASS in both orders.
- Control with production NODUP hud_draw: LNK2005 in both orders.
(10) objdiff-cli 3.3.1 single-unit projects plus the repo semantic/admission pipeline (strict_emulate):
- With 04R: 22/22 functions, 6176/6176 code, 152/152 data, and contradicted/rejected/revoked all empty.
- With the HEAD rejections: contradicted.
(11) /W3 /Zs warning census (own script, temp TU inside the slug): multisets identical for all 6 edited TUs, including 01RaltS.
(12) fake_match_scan on explicit files, prod vs cand: lead counts unchanged (hud_unit 0).
(13) cachebeta publics: no public at C6590/C65A0/C65C0/C6600/C6B80; _check_stack_buffer PUBLIC at C08F0.
(14) January split census: _check_stack_buffer is defined only by hud_draw, with 0 references or relocations; _get_return_eip is UNDEF in the other 4 sentinel TUs. 26 macro sites in the candidate equal January's 26.
(15) House rules and law (g)/(i)/(j) review of every hunk.
- Strip test: the plain goto-free loops are not exact (review3 S1/S2). The inline is attested by first-party evidence (inlined return merge at 26 sites, PUBLIC, never called).
- /Od: the later build has no sentinel code, so no declaration-order evidence applies.
- History of the models include: from commit 635bd83d. Only the struct tag bitmap_data overlaps.
(16) Precedents consulted: owner rulings 20260921 #2/#4/#6, the review3_damage patch-A rule 6, the review3_main header_swap class, and the review3_hud_unit blockers.

## issues
APPROVE=FALSE AS SUBMITTED. There are two defects, and an amended packet that fixes both is verified and ready.

(A) P6 (motion_sensor.c) breaks owner ruling 20260921 #2 / law (i), which requires a strictly exact caller.
- All three motion_sensor sentinel callers are residual AND parked: _motion_sensor_update, _update_motion_sensor, _render_motion_sensor.
- P6 therefore newly emits the ANY _check_stack_buffer COMDAT in motion_sensor.obj from non-exact callers only. This is the same class as damage patch A and actor_combat P1, both of which are held.
- Fix: drop P6. The hud_unit admission does not need it, and dropping it is raw-inert: motion_sensor with the production .c and the new header is byte-identical to production.
- Revisit P6 when one of those callers becomes strict-exact or the owner admits the P1 class.

(B) 01RaltS is not admissible. It keeps `goto update_finished`, while the /Od build has three per-arm last_shield_hit_time stores and no goto. That is the invented-goto held class. Use 01RS only.

AMENDED PACKET: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_hud_unit\patches\. The files are byte-identical copies of the worker's patches; hashes are in SHA256SUMS.txt.
- Apply atomically, in this order: P1_hud_draw_h_check_stack_buffer_inline_and_macro.patch (shared header, separate), P2_hud_draw_c_shared_stack_frame.patch, 01RS_hud_unit_c_01R_plus_shared_stack_frame.patch, P4_hud_nav_points_c_shared_stack_frame.patch, P5_hud_weapon_c_shared_stack_frame.patch, P7_rasterizer_xbox_models_c_drop_dead_hud_draw_include.patch, 02_symbols_json_hud_unit_names_static.patch, 03_config_json_hud_unit_matching.patch, 04R_admission_rejections_retire_hud_unit_fast_ftol.patch.
- git apply --check is CLEAN as a set on 16542e46.
- Verification results:
  - 23-consumer sweep: identical to production except the intended changes.
  - hud_unit: 22/22 EXACT against the emulated split, audit PASS, surplus IDENTICAL, provider link 12/12 in both orders.
  - Strict emulation: 22/22 functions, 6176/6176 code, 152/152 data.
  - Warnings: unchanged.
- P1+P2 must land with 01RS: the production NODUP hud_draw gives LNK2005.
- 04R is required: without it the result is contradicted.
- P4 is optional. The strict fallback (P1,P2,01RS,P7,02,03,04R) was also measured; its only effect is a same-size drift of the unparked residual _render_weapon_hud.
- Header consumers of hud_draw.h, for the integrator's full sweep: source/interface/{hud_draw,hud_messaging,hud_nav_points,hud_unit,hud_weapon,motion_sensor,ui_widget}.c, source/objects/widgets/lightning.c, source/rasterizer/{rasterizer_debug,rasterizer_lights}.c, source/rasterizer/xbox/{rasterizer_xbox,rasterizer_xbox_debug,rasterizer_xbox_dynavobgeom,rasterizer_xbox_environment,rasterizer_xbox_environment_fog,rasterizer_xbox_models,rasterizer_xbox_screen_effect,rasterizer_xbox_shadows,rasterizer_xbox_transparent_geometry,rasterizer_xbox_water,shader_transparent_generic_preprocessor}.c, source/render/{render_contrails,render_sprite}.c. There are no paths with spaces. After the packet, models no longer includes it.

DISCLOSE:

(1) P7 is count shielding by removing a genuinely dead include.
- models uses none of hud_draw.h's names except the struct tag bitmap_data, which bitmap headers already declare.
- The include was added by project commit 635bd83d.
- Removing it is raw-inert and warning-inert, and follows the precedent of removing verbatim duplicates.
- It is load-bearing only relative to P1: the inline costs about 9 name units in models.
- It differs from the held Layer-2 / main header_swap class, which ADDS declarations chosen by count. It must still be stated in any ruling request.
- The P-A fallback (an invented focused header) conflicts with owner ruling #4.

(2) Newly emitted symbols (ruling #6):
- An ANY _check_stack_buffer (48 B, identical to January's hud_draw copy) in hud_unit.obj, hud_nav_points.obj and hud_weapon.obj. The last two each have strictly exact callers.
- hud_draw.obj's copy changes from NODUP to ANY.

ADVISORY (not scored): VC7 emits a referenced header inline at its definition position, so the candidate hud_draw.obj emits _check_stack_buffer BEFORE _get_return_eip. January's image has get_return_eip at C08E0 before check_stack_buffer at C08F0. That fits get_return_eip also being a hud_draw.h inline defined first, matching the Sept-2001 'i' flag. Relevant to hud_draw's own admission; not proposed now, because it would emit _get_return_eip from motion_sensor's non-exact callers.

Integrator: accumulation with other wave-4 header packets must be re-swept on the live tree (23 consumers plus parks). Full notes: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review4_hud_unit\REVIEW.md and LEDGER.md.
