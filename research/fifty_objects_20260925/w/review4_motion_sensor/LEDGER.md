# review4_motion_sensor - adversarial review of hud_stack_check P6 (motion_sensor.c shared stack frame)

Tree 16542e46, git status clean at start. Worker slug scratch/w/hud_stack_check/ (P6 depends on P1 hud_draw.h).

## R0 reading
- WORKER_BRIEF 0-9, house rules, lane log held table (hud_unit row = this packet's reopen path; P1-class law (i)).
- worker LEDGER (R0..RESULT). Sibling wave-4 reviews of the SAME packet: review4_hud_draw (EXCLUDE P6: law (i)),
  review4_hud_unit (DROP P6, owner-gated: law (i)), review4_hud_weapon (P6 follows hud_draw ruling),
  review4_hud_nav_points (approves packet incl. P6 without separately assessing P6's COMDAT callers).
- Parallel wave-4 motion_sensor packet: w/motion_sensor production.patch (bss inits + render_motion_sensor weight local),
  reviewed APPROVE (bss hunk only) in review4_motion_sensor_____bss_symbol_layout__object_admission_. Composition with P6 to check.
- parked.json: all 3 sentinel callers (_motion_sensor_update, _render_motion_sensor, _update_motion_sensor) PARKED; park base shas
  40e4819d.. / de9701ad.. / 5bae910d.. .

## V1 patch applicability (current tree 16542e46)
- patches copied verbatim to patches/ (sha256 P6 e8caeaff..., P1 cd30e78e...). git apply --check: P6 alone CLEAN, P1 alone CLEAN,
  P1+P6 CLEAN, P1..P7 CLEAN; composition with the parallel motion_sensor packet: ms production.patch then P6 CLEAN,
  P1+P6+ms production CLEAN, P6+bss_only CLEAN.
- `git show HEAD:` blobs + `patch -p1` (P1, P2, P6) -> tree/; motion_sensor.c and hud_draw.h == worker cand files (EOL-normalised).
## V2 compile (gate --all; TU copies gtu_{ctrl,hdronly,cand}.c with the include redirected to slug headers)
- prod / ctrl (verbatim HEAD header) / hdronly (P1 header, HEAD TU) / cand (P1 header + P6 TU): all 16 EXACT / 3 residual,
  identical rows (_motion_sensor_update 1136!=1168 relocs 44!=43; _render_motion_sensor 784!=768; _update_motion_sensor 816!=864 relocs 37!=39).
- own rawcmp.py (bytes, relocs by normalised target name, flags, COMDAT sel+checksum, full symbol table, section order; skips .debug$S/T):
  base==prod 0; prod vs ctrl 0; prod vs hdronly 0 (=> P6 is NOT needed for motion_sensor bytes once P1 lands);
  prod vs cand: ONLY +.text _check_stack_buffer (48 B, sel 2 = ANY, external). Parked bodies byte-identical (no park drift).
## V3 admission tools on cand (out/cand.obj)
- surplus_identity_cand.py (slug copy taking an obj): prod 9 surplus code COMDATs all IDENTICAL; cand 10, the new
  _check_stack_buffer IDENTICAL to build/split hud_draw's (only January definer).
- provider_link.py (repo, production providers) --baseline=prod: _check_stack_buffer FAIL, LNK2005 both orders vs production
  NODUP hud_draw => P6 ALSO depends on P2 (worker lists only P1). provider_link_ovr.py (slug copy, hud_draw provider =
  out/hud_draw_p12.obj = HEAD+P1+P2, gate 22/0/1unw, rawcmp vs build/base: only sel 1->2 of _check_stack_buffer + order):
  new surplus PASS both orders; full surplus list PASS.
- object_audit prod vs cand: text differs ONLY by `+.text sym:_check_stack_buffer size 48`. FAIL(5) both = 3 parked residual
  bodies + 2 .bss symbol offsets (next_vertex_index/sweep_theta; owned by the parallel bss packet, not P6).
- data: tinfo --data prod vs cand: identical contents (only section numbers +1 and the .debug$S temp path).
- /W3 /Zs (slug warn.py, compiles in place, absolute-path include): prod / hdronly / cand warning multisets identical (17 lines).
- cachebeta publics: _check_stack_buffer PUBLIC 000C08F0, _get_return_eip PUBLIC 000C08E0 (non-static __inline storage consistent).
## V4 first-party evidence
- January split bytes: each of the 3 functions has ONE sentinel loop (mov eax,7Fh; mov ecx,62626262h) and TWO `push <line>`
  (642 / 756 / 830) => both asserts share one line (macro attestation, as worker F2). Cand identical counts.
- /Od build (odbuild str): no "corrupt stack" / "corrupt return" strings; RTC descriptors of 0x641b60 / 0x642ff0 / 0x642868 name
  no stack_buffer => no /Od declaration-order evidence for or against the macro block (law (d) not applicable).
- HCEX.pdb: DIA2Dump -sym check_stack_buffer / get_return_eip -> no symbol (helpers absent in 2011).
- object_admission_rejections.json: no motion_sensor entry.
## V5 strip test (labs/lab_strip_nomacro.c: macro expanded as plain statements, `short corrupt_index;` at function top)
- 16/3, rawcmp vs cand: 0 differences => macro block scope byte-inert; nothing in P6 buys a match (P6 buys NO bytes at all: V2 hdronly==prod).
## V6 composition with the parallel motion_sensor wave-4 packet (w/motion_sensor production.patch = bss inits + render weight)
- HEAD+ms production (== w/motion_sensor/cand_prod.c) vs +P1 header: 0 diffs; +P1+P6 (P6 applies with offset 1 on hunk 6):
  ONLY +ANY _check_stack_buffer. Gate 16/3 in all three. Re-baselined render park unaffected by P6.
## FINDING F-A (blocking): law (i) / rule 6 condition 2 / P1 class
- P6's ONLY object effect is to newly emit the shared-header __inline COMDAT _check_stack_buffer in motion_sensor.obj.
  Its ONLY users in the TU are the 3 sentinel sites, all in RESIDUAL + PARKED functions (_motion_sensor_update 1136!=1168,
  _update_motion_sensor 816!=864, _render_motion_sensor 784!=768). No strictly exact caller exists => rule 6 / law (i) fails;
  same class as the HELD 'damage patch A / hs real_random_range cleanup' (zero-credit house-clean, new header COMDAT from a
  non-exact caller) and actor_combat P1. Identity + pair link + sweep conditions pass but do not cure condition 2.
- Contrast: hud_nav_points P4 / hud_weapon P5 have strictly exact callers (sibling reviews' labs). motion_sensor has none.
## FINDING F-B: worker dependency/optionality statements are wrong for P6
- "Depends on P1" is incomplete: P6 also needs P2 to link (LNK2005 vs production NODUP hud_draw, both orders).
- Worker A5 "P4-P6 are NOT optional" is contradicted for P6 by the worker's own A5 numbers and V2 here: P1 alone leaves
  motion_sensor 0-diff; only P5 is load-bearing among P4-P6. Excluding P6 is raw-inert for the whole packet.
## V7 NEW EVIDENCE: P6 on top of the owner-gated w/motion_sensor cand_full.c (m5 = m4 update + u9 /Od update body + r9 render + bss)
- hypothesis (M8, IL-local ordinals of inlined helpers): replacing the goto hand copy (2 block locals) by the genuine inline
  (param buffer + local index + block corrupt_index) shifts IL-local ordinals in _update_motion_sensor, whose only residual is
  the inlined distance_squared3d sum association (w/motion_sensor u9: Jan (jj+ii)+kk vs ours (ii+kk)+jj).
- measured (P1 header both): cand_full (no P6) 17/2 (_update_motion_sensor residual sha, 864/39 sizes equal);
  cand_full + P6 (patch fuzz 2 on hunk 1; stack_buffer/sites verified) => 18/1: _update_motion_sensor EXACT.
  rawcmp: +ANY _check_stack_buffer and _update_motion_sensor bytes/chk change only.
- V7b hybrid WITHOUT the owner-gated m4 (hyb/: w/motion_sensor cand_prod.c [= HEAD + production.patch] with ONLY
  update_motion_sensor replaced by cand_full's u9 /Od body): no P6 16/3 (update residual sha); + P6 => 17/2,
  _update_motion_sensor STRICT EXACT 864; rawcmp: only +ANY _check_stack_buffer and update bytes differ.
  => P6 is LOAD-BEARING for u9: the genuine inline+macro closes the u9 sum-association residual. With an exact caller,
  law (i) is satisfied for _check_stack_buffer (and u9's _distance_squared3d gets its own exact caller). Pursue as amendment.
- V7c goto-free lab (labs/hyb_u9_p6_nested.c: the two `goto update_motion_sensor_done` become single-exit nested ifs
  `if (motion_sensor_globals->update) { ...; if (unit_index != NONE) { ... } }`, label removed): 17/2, update EXACT,
  rawcmp vs hyb_u9_p6: 0 differences => the gotos are byte-inert; the /Od build (0x642ff0) jumps both early-outs straight
  to the epilogue, which cannot discriminate goto vs nested-if. Land the goto-free form (single-exit law, invented-goto class).
## V8 strip tests in the u9 context (base labs/hyb_u9_p6_nested.c)
- stripA (inline called directly, `short corrupt_index;` at function top, no macro block): update EXACT, rawcmp 0 diffs
  => macro block scope byte-inert.
- stripB (goto-free HAND loop with break, no inline; same asserts): update RESIDUAL (sha) => the load-bearing construct is the
  check_stack_buffer INLINE itself, which is first-party attested (26/26 January sites carry the inlined-function return-merge
  shape; _check_stack_buffer PUBLIC in cachebeta C08F0; sole definer hud_draw; Sept-2001 map 0x30 gap after get_return_eip 'i').
  Not a decoration: it is the genuine January helper replacing a hand copy (the M8 IL-ordinal mechanism).
## V9 /Od declaration-order audit of u9 (od 0x642ff0 frame, slots in declaration order)
- /Od: player -8, (time) -0xc, active_sensor_index -0x10, unit_index -0x14, sensor -0x18, reference_point#1 -0x28,
  blip_index -0x30 (short), blips -0x34, object_index -0x38, unit_center -0x48, player_center -0x5c, point -0x6c, dummy -0x78,
  draw -0x7d, reference_point#2 -0x90, customs -0x118, custom_index -0x120 (long), out_of_range_count -0x124, custom_temp -0x134.
- u9 declares customs at FUNCTION TOP (before block-1 reference_point) - contradicts /Od order (customs follows
  reference_point#2 => C89 home is block 2's head). blip_index/blips after block 1 in /Od: C++-style later source; C89 top-level
  hoist is ambiguous. -> labs C (customs into block 2) / D (blip_index+blips block) next.
## V10 declaration-placement labs (base labs/hyb_u9_p6_nested.c)
- labC (customs moved from function top into block 2 after reference_point, /Od order): EXACT, rawcmp 0 diffs.
- labD (labC + `{ short blip_index; struct motion_sensor_blip *blips = sensor->blips; for ... }` block after block 1, /Od
  order player..sensor, ref#1, blip_index, blips, loop locals, ref#2, customs, custom_index, out_of_range_count, custom_temp):
  EXACT, rawcmp 0 diffs. All placements byte-inert => choose the fully /Od-consistent labD (law (d)).
## V11 AMENDED PATCH P6A (patches/P6A_motion_sensor_c_stack_frame_and_update_od_body.patch) = P6 + update_motion_sensor
   replaced by the labD body (u9 /Od body of w/motion_sensor, goto-free single exit, /Od declaration order). Against HEAD.
- git apply --check: P6A alone CLEAN; P1+P6A CLEAN; ms production.patch then P6A CLEAN; P6A then ms production CLEAN.
  `patch` replay onto the HEAD blob == amend/source/interface/motion_sensor.c (cmp).
- gate --all (P1 header): 17 EXACT / 2 residual; _update_motion_sensor EXACT 864 (NEW strict function).
- rawcmp prod vs amend: update bytes + new surplus (+__real@00000000 .rdata ANY, +_check_stack_buffer, +_distance_squared3d,
  +_vector_from_points3d); _motion_sensor_update / _render_motion_sensor bytes UNCHANGED (their parks hold).
- object_audit: update now `ok 864/864`; FAIL(4) = 2 parked residuals + 2 .bss offsets (parallel bss packet). Surplus adds 4 rows.
- surplus identity: 12 candidate-only code COMDATs all IDENTICAL (_distance_squared3d/_vector_from_points3d vs action_alert,
  _check_stack_buffer vs hud_draw). provider_link_ovr (hud_draw = HEAD+P1+P2): new surplus 4/4 PASS both orders; full list PASS.
- nodup_census: _distance_squared3d 44 ANY definers / 0 NODUP; _vector_from_points3d 78 / 0; _check_stack_buffer only
  production hud_draw NODUP (=> P2 required).
- /W3 /Zs: amend warning multiset == prod.
- V11b exact-caller-only lab (labs/lab_exact_caller_only.c: P6A but _motion_sensor_update/_render_motion_sensor keep HEAD's
  goto hand copies): 17/2, rawcmp vs amend 0 differences => _check_stack_buffer emission is owned by the strictly exact
  _update_motion_sensor (law (i) satisfied); converting the two parked callers is byte-inert hygiene (P5 precedent).
- V11c January tail check: _update_motion_sensor sentinel tail in January == amend (mov eax,7Fh loop at +0x2c6, frame slot
  -0x2c0, `or esi,-1; call $+5`, push 0x2f4 x2, out-of-line `mov esi,eax; jmp` merge at +0x34d). Prod differs (loop pad, -0x2c4).
- V12 composition: HEAD + ms production.patch + P6A (P1 header): 17/2, update EXACT; vs ms production+P1: only update bytes +
  4 surplus; object_audit FAIL(2) = only the 2 parked residuals, 44 January symbols 0 differ (bss packet fixes the offsets).
- V13 alternative P6Aalt (labC form: blip_index/blips hoisted to function top): CLEAN, EXACT, rawcmp vs amend 0 diffs.
  Primary stays P6A (labD) under law (d); P6Aalt offered only if the owner rejects the loop-1 bare block.
- V14 config: parked.json entry for _update_motion_sensor must be removed (tools/parked_functions.py invalidates a park whose
  function is now exact) -> patches/P6A_parked_json_unpark_update_motion_sensor.patch (-U2, CRLF body, CLEAN; full edited copy
  config/parked.json, 86 -> 85 entries). symbols.json already has `_update_motion_sensor` static:true (not public in cachebeta);
  _distance_squared3d / _vector_from_points3d PUBLIC in cachebeta (external header inlines). pdb_storage production: 44/0.
- P6A dependencies: P1 (macro/constants, compile) + P2 (hud_draw ANY provider, link) and therefore P1's own required set
  (P3|01RS, P5, P7 per review4_hud_draw/hud_unit). No header edit in P6A itself; blast radius = motion_sensor.obj only.
## RESULT
- P6 as submitted: approve=false (law (i)/rule 6 cond. 2; P1 class like held damage patch A). Dependency statement incomplete (P2).
- Amendment P6A (+ parked.json unpark) verified: _update_motion_sensor STRICT EXACT (+1 fn / 864 B) with P1 header; law (i)
  satisfied (exact-caller-only lab identical); goto-free; /Od declaration order; all admission tools pass for the new surplus.
  Conditional on the P1+P2 packet landing. REVIEW.md written. git status clean.
