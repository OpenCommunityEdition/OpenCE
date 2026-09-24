# review4_hud_unit - adversarial review of hud_stack_check packet (hud_unit OBJECT_COMPLETE_CANDIDATE) - LEDGER

Tree 16542e46, git status clean at start. Reviewer writes only under scratch/w/review4_hud_unit/.

## R0 reading
- WORKER_BRIEF 0-9, house rules, lane log (held table: hud_unit row reopen path = this hud_draw.h packet),
  review3_hud_unit REVIEW (blocker 2 = hand-copied check_stack_buffer + invented gotos), worker LEDGER (R0..RESULT),
  review3_damage rule-6 rationale (law (i): new header COMDAT only from a strictly exact caller).
## V0 consumer list (independent grep over source/, all extensions, case-insensitive "hud_draw.h")
- 23 .c consumers + the header itself; no header includes hud_draw.h; no .c includes another .c; identical to worker's
  consumers.txt. No spaces-in-path consumer. STACK_BUFFER_*/check_stack_buffer/match_assert_stack_frame names appear in no
  other source file (no collision risk in the 18 non-sentinel consumers).
## V1 git apply --check on 16542e46: all 12 patches CLEAN alone; full set (01RS) CLEAN; full-alt (01RaltS) CLEAN.
## V2 patch replay (patch -p1 on `git show HEAD:` copies in tree/ and tree_alt/): every patched file == worker's cand files
   (EOL-normalised): hud_draw.h/.c, nav_points, weapon, motion_sensor, models; hud_unit 01RS == cand01R, 01RaltS == cand01Ralt.
## V3 reviewer sweep (tools/sweep.py = redirected TU copies, gate --all --out; tools/rawcmp.py = STRICT raw COFF compare:
   section order, flags, bytes, relocs by target NAME, COMDAT selection, whole symbol table; only $L<n> label NUMBERS normalised)
- ctrl (verbatim HEAD header): 23/23 RAW-IDENTICAL to build/base (harness inert).
- cand (full packet): 17 non-sentinel consumers + models RAW-IDENTICAL; gate rows identical except hud_unit 20+2 unwritten
  (the two renames, vs production split - expected). hud_draw: _check_stack_buffer NODUP->ANY AND its .text section now
  PRECEDES _get_return_eip (sec 12/13 swapped; January image has get_return_eip C08E0 < check_stack_buffer C08F0). Not scored;
  production hud_draw section order already differs from January's image order (header inlines first), so advisory only.
- nop7 (P1 without P7): models __rasterizer_model_transparent_geometry_submit EXACT->residual (confirms worker H1). P7 load-bearing.
- hdronly (P1, all TUs production): hud_draw/hud_unit fail to compile (duplicate definitions - expected), hud_weapon
  _render_weapon_hud residual bytes drift (unparked), nav_points/motion_sensor raw-identical.
- amend (full packet minus P6): motion_sensor RAW-IDENTICAL to production -> P6 is NOT needed by anything else.
## F-A (FINDING, law (i) / owner ruling 20260921 #2): motion_sensor's three sentinel callers (_motion_sensor_update,
   _update_motion_sensor, _render_motion_sensor) are ALL residual AND parked (config/parked.json). P6 newly emits an ANY
   _check_stack_buffer COMDAT in motion_sensor.obj whose every caller is non-exact = exactly the damage-patch-A / actor_combat
   P1 class ("strict-exact caller"). P6 must be dropped from the packet (owner-gated); dropping it is raw-inert (V3 amend).
   hud_nav_points (exact hud_update_nav_point_local_player + parked custom_render) and hud_weapon (exact render_grenade_hud,
   hud_update_weapon + 3 residuals) each have >=1 strict-exact caller -> acceptable under ruling #2, disclosed.
## V4 /W3 /Zs census (tools/warn.py, temp TU inside slug, absolute header redirect): warning multisets identical prod vs cand
   for hud_draw 26, hud_unit 12 (01RS and 01RaltS), nav 13, weapon 14, motion 17, models 13 (pre-existing C4013 unchanged).
## V5 emulated split (config_emu = HEAD config + 4 in-place symbols.json line edits via python, CRLF kept;
   byte-identical to the worker's config/symbols.json copy). csplit (build/tools/csplit.exe -> slug):
   split_ctrl == build/split 833/833; split_emu differs ONLY in source/interface/hud_unit.obj.
## V6 hud_unit vs split_emu (tools/gate_split.py, gate.py rule + storage): 01RS 22/22 EXACT, storage 0 mismatches;
   01RaltS 22/22 EXACT; 01RS and 01RaltS objects RAW-IDENTICAL. Production obj vs split_emu 20/0/2 (renames).
## V7 object_audit (slug copy reading SPLIT_ROOT=split_emu): 01RS and 01RaltS PASS: 30 January-owned sections ok,
   30 January symbols 0 differ; surplus .drectve, 5 pooled strings, 5 __real, _fast_ftol, _check_stack_buffer.
## V8 surplus identity (slug copy taking obj paths): hud_unit _fast_ftol==actor_combat, _check_stack_buffer==hud_draw
   (IDENTICAL, both candidates); nav_points 5/5, weapon 3/3, motion 10/10, hud_draw 2/2 IDENTICAL.
## V9 provider link (slug copy provider_link_ovr.py; Link.Exe temp under slug/tmp): cand01RS with PRODUCTION hud_draw
   provider -> _check_stack_buffer LNK2005 both orders (FAIL, control discriminates); with the packet's hud_draw.obj
   (out_cand) -> 12/12 PASS both orders. => P1+P2 must land atomically with 01RS.
## V10 strict emulation (objdiff-cli 3.3.1 single-unit projects in slug, target split_emu, complete=True; slug copy of the
   worker's strict_emulate.py = repo semantic/admission pipeline): 01RS and 01RaltS with 04R -> 22/22 fns, 6176/6176 code,
   152/152 data, rejections/revoked [], classify candidates/contradicted/rejected all []. With the HEAD rejections file
   (hud_unit _fast_ftol veto kept) -> contradicted. 04R required; 04R result == worker's rejections copy.
## V11 hygiene scans: fake_match_scan (explicit files) prod vs cand: hud_unit 0/0, hud_draw.h 0/0, hud_draw.c 2/2 (pre-existing
   naked/asm get_return_eip), nav/weapon/motion/models 0/0. 01RS has NO goto left in hud_unit.c.
- 01RaltS keeps `goto update_finished` (the /Od build has three per-arm last_shield_hit_time stores and no goto; review3) ->
  01RaltS is /Od-contradicted invented-goto class: NOT admissible; use 01RS only.
- cachebeta publics: no public at C6590/C65A0/C65C0/C6600/C6B80 -> the 4 static claims of 02 hold; _check_stack_buffer PUBLIC
  C08F0 (external) matches non-static __inline. Names: HCEX_Release + Sept-2001 maps (review3/hud_unit_audit, not re-derived).
- January split census: _check_stack_buffer defined only by hud_draw, 0 references/relocations anywhere; _get_return_eip UNDEF
  in hud_nav_points/hud_unit/hud_weapon/motion_sensor. 26 macro sites in the candidate tree = January's 26 (14/2/2/5/3).
- keycmp (owner-keyed raw compare): hud_unit 01RS vs review3 cand_final_amended.obj: ONLY the new ANY _check_stack_buffer.
  nav/weapon/motion cand vs production: ONLY the new COMDAT (parked bodies byte-identical). hud_draw: NODUP->ANY + text order.
- models dead include: of 26 hud_draw.h names only `bitmap_data` (a forward-declared struct tag, declared by bitmap headers)
  appears in models.c; include added by project commit 635bd83d (2026-09-09); removal raw-inert + warning-inert.
## F-B (ADVISORY) emission order: VC7 emits referenced header inlines at their DEFINITION position (hud_unit sec6, nav sec13,
   weapon sec6: right after the earlier headers' inlines, before the TU's own functions). So with get_return_eip in hud_draw.c
   and check_stack_buffer in hud_draw.h, hud_draw.obj emits check_stack_buffer FIRST; January's image has get_return_eip C08E0
   then check_stack_buffer C08F0. Consistent January arrangements: get_return_eip ALSO a hud_draw.h inline defined before
   check_stack_buffer (Sept-2001 'i' flag), or get_return_eip in an earlier header. Not scored; production hud_draw order already
   differs from January's image elsewhere; moving get_return_eip would newly emit _get_return_eip in motion_sensor (all callers
   non-exact) -> not proposed. Record for hud_draw's own future admission.
## F-C (DISCLOSE) P7 is count shielding by a genuine dead-include removal (100K-lane duplicate-removal precedent; the P-A
   fallback = invented focused header conflicts with owner ruling 20260921 #4). Not the Layer-2/main-header_swap class
   (those ADD declarations chosen by count), but it is load-bearing only relative to P1 -> state it in any ruling request.
## V12 variants: strict (P1,P2,01RS,P7 only; nav/weapon/motion production .c): nav + motion KEYED-IDENTICAL, hud_weapon
   _render_weapon_hud (unparked residual) bytes drift at same size/relocs; everything else identical. min (drop P4+P6):
   nav raw-identical. => P4 optional; P5 keeps hud_weapon byte-stable and removes invented gotos from 2 EXACT functions.
## AMENDED PACKET (patches/ in this slug, byte-identical copies of the worker's, SHA256SUMS.txt): P1, P2, 01RS, P4, P5, P7,
   02, 03, 04R  (P6 DROPPED; 01RaltS NOT to be used). git apply --check CLEAN on 16542e46 as a set. Verified: 23-consumer
   sweep = production except hud_unit (22/22 vs split_emu) + new identical ANY COMDAT in hud_unit/nav/weapon + hud_draw
   NODUP->ANY/order; motion_sensor raw-identical; audit PASS; surplus identical; provider link 12/12 both orders; strict
   emulation 22/22 + data 152/152; warnings unchanged.
