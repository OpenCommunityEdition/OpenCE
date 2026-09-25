# triage_ai_b LEDGER (round 2, base f6d00a8c)
Objects: actor_looking, actor_moving, ai_communication, ai_debug. Triage only; no candidate source edits.

## hints.json snapshot
- actor_looking 15/16: _actor_look_update S2 4720/4704 (insn 1432/1425, frame 88/88); link_fail _cross_product2d
- actor_moving 31/36: test_avoidance_vector S1 (752/544; missing _global_zero_vector3d, extra 2x _actor_move_transform_avoidance_vector), destination_update T, path_refresh S2, move_update S2, vector_avoidance S2 4144/4128
- ai_communication 45/48: actor_talk_weight T (park instruction-scheduling), finished S1 (missing _ai_communication_find_global_actor_to_talk; park register-allocation), event S2 8064/8128; data_gap 276; storage diffs for 2 fns
- ai_debug 59/60: ai_debug_render_actor S2 24976/25008 (insn 6829/6874); 34 audit fails (storage 2/3 on statics, MISSING _jmptable); data_gap 56

## Probe 0: gate of production source (f6d00a8c)
- actor_looking 15/1/0: _actor_look_update size 4704!=4720 sha
- (other rows below)
- actor_moving 31/5/0: destination_update [sha], test_avoidance_vector [size 544!=752, relocs 17!=16], move_update [sha], vector_avoidance [size 4128!=4144], path_refresh [sha]
- ai_communication 45/3/0: talk_weight [sha], event [size 8128!=8064], finished [size 1568!=1584, relocs 86!=87]
- ai_debug 59/1/0: render_actor [size 25008!=24976]

## actor_moving evidence (reads)
- Ledgers read: n4 (0915), astra avoidance packet (0920), lane A waveA4 (0920), w1 worker notes (opus5-150k scratch/workers/actor_moving.md).
- Last touch of the transform family is 0920; owner ruling 1 (0921, point_from_line3d guard RELAXED for actor_moving,
  one of 17 January objects referencing _point_from_line3d out of line) post-dates every actor_moving probe.
- January relocs: calculate_movement and try_evasion_vector each call _point_from_line3d OUT OF LINE (1 each), same in base.
- /Od 0x466b10 = test_avoidance_vector (asserts lines 0x8a0..0x8a2 "avoidance_data && avoidance_ray"...): two locals
  [ebp-0x14],[ebp-0x28] each = *global_zero_vector3d then 3x call 0x42e0d0 (point_from_line3d) (&v,&data->forward/left/up (+0x18/+0x24/+0x30), ray->field, &v);
  then point_from_line3d(&data->origin(+0xc), &offset, data->avoid_width(+0x6040), ray_origin); then 0x42e220(&dir, ray->length*data->avoid_distance, ray_direction) (scale_vector3d);
  then 0x42e3a0(origin, ray_origin, &tmp) (vector_from_points3d). NO call to transform (0x467840).
- /Od 0x467840 = transform_avoidance_vector: *dir = *global_zero; 3x point_from_line3d(dir, &basis, v->i/j/k, dir).
- Production source: test_avoidance_vector calls transform x2 and hand-expands point_from_line3d/scale_vector3d;
  vector_avoidance perpendicular site hand-expands 2 point_from_line3d accumulations; TU has #define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D (line 207).
- w1 ta5 (guard-failing diagnostic, 0914): point_from_line3d form reached 18 blocks; remaining: component def fld/fst vs int prefetch.
  ta5 used a named component / point casts; /Od shows the field passed DIRECTLY as the t argument (no component local).

## actor_moving probes (lab --edits only; production untouched)
- P1 drop `#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` only: all 36 rows UNCHANGED (31 exact); calculate_movement and
  try_evasion_vector keep their out-of-line calls (January gating reproduced); object now defines _point_from_line3d
  48 B sha 9b763841 = January selected copy (action_charge). => admissible under owner ruling 1 (0921), row-neutral.
- P2 P1 + w1 ta5 body (zero inits first, 6x point_from_line3d basis terms, point_from_line3d ray_origin, scale_vector3d) at HEAD:
  test_avoidance_vector 768/16 vs 752/16 (relocs now EQUAL, were 544/17); vector_avoidance 4128->4112 (private-ABI coupling).
  alndiff: residual = the 4 inline `t` defs (Jan fld [edi+8]; fst [ebp-4]; ours hoisted int prefetch mov eax,[edi+8] .. mov [ebp-4],eax)
  + 1 global_zero pointer-load transposition. SAME residual w1 recorded for ta5 on 0914.
- /Od RTC 0x466f38: origin_offset, divergence_direction, collision_result(1048), vector_to_origin, dummy_normal, intersect_t.
- P3 P2 + separate vector_to_origin (fn scope) + dummy_normal (loop scope) per /Od: frame 0x438 -> 0x444 (WORSE; escape-scope law:
  January's frame proves one 12-byte slot [ebp-0x14] serves origin_offset/vector_to_origin/normal). t-def residual unchanged.
- P4 P1 + vector_avoidance perpendicular site as /Od 0x46b40c..0x46b47f (rotation=zero; 2x point_from_line3d left/up, drop fake
  `perpendicular` local): size stays 4128/4144, frame still 0x60e4 vs Jan 0x60e0 (+4 scalar, unidentified), alndiff blocks 222 -> 231. No gain.
- /Od move_update 0x4684d2/0x4684fd: scale_vector3d(&rot,1-blend,&rot); point_from_line3d(&rot,&avoidance_rotation,blend,&rot).
  Production hand-expands the point_from_line3d (3 `+=` lines) = rule-6 hand expansion.
- P5 P1 + that point_from_line3d at move_update blend: 3136 both, blocks 97 -> 90 (blend region now aligned);
  remaining = OURS pins zero in edi (xor edi,edi; cmp word,di; mov [esi+0x50a],di) where January uses imm 0 / xor eax,eax (reverse LAW Z),
  edi/ebx swap on object_get_and_verify_type result, crouch region at +0x493 (recorded 45-shape crouch family).
- January move_update +0x79c..+0x7ab: `test bl,bl; je; test dh,dh; jns; jmp; test dh,0x40; je` = crouch selects between two CONSTANT bit tests
  (control flow); ours `TEST_FLAG(flags, crouch ? A : B)` emits a variable shl. P6 (P5 + `crouch ? TEST_FLAG(A) : TEST_FLAG(B)`) -> 3120/3136
  (value materialised: shr/and/cmp), blocks 96; P6b (define drop + ternary only) 3136, 104 blocks. Lane A A4 `(c && T(A)) || (!c && T(B))` = 3104. Not closing.
  move_update verdict: RETIRE (45-shape history + Lane A A4 + research11; remaining zero-pin/crouch/unit-pointer register family).

## actor_looking
- Ledgers read: evidence_closeout 0919, astra research10 + independent (0920), round-1 wave1 update/find_prop + REVIEW.
- Round-1 stop rule fired on update after 9 /Od shapes (pp/zero edi-ebx colouring, secondary_aim_valid spill [ebp-7]); 139-candidate census; packet10 byte-inert.
- Round-1 reopen (2) "another header-helper spelling at an update site via /Od callee map": checked. /Od update 0x45f5d0 callees:
  cosine 0x455220 x5 (already landed), dot_product3d 0x42dd80 x3, dot_product2d 0x42dd30 x2, valid-normal 0x42e020 x4, 0x42dec0 x1.
  0x42dec0 (at realcmp site 0x460ffe, `fabs(k - 0.f) < 1e-4`) -> 0x42e400 (cvtss2sd; CRT) is the TWO-level C++ fabs(float)->fabsf chain,
  unlike cosine's THREE-level cosine()->cos(float)->cosf chain => realcmp's direct fabs is already the /Od spelling. NEGATIVE; no new helper site.
- Admission also blocked by provider link FAIL _cross_product2d (actor_combat NODUP; RESERVED Codex packet).

## actor_moving other residuals
- path_refresh: 0 REAL regions; 3 && failure edges bind to far epilogue copy (0x2dd) vs January near (0x542) = 12 B. w3 spent 6 forms, Lane A v1 nest inert, v2 worse.
  /Od 0x46c720 tail: A-test then `if (!success) actor_path_clear` in the same flow; early paths `jmp 0x46cf1d` into the !success test = same structure as ours. No new evidence. RETIRE.
- destination_update: T (281/281 insns), laws_w3 D 5 shapes register-role + x87-lifetime tie. Not re-probed. RETIRE.
- vector_avoidance: frame 0x60e4 vs 0x60e0 (+4, unidentified scalar), 222 alndiff blocks; P4 perpendicular pfl form no gain. RETIRE (keep frame lead as reopen).

## ai_communication
- Data: .data 276 (3 name tables, 69 relocs) coff_compare IDENTICAL to January; objdiff 3.3.1 50% = '$'-literal scorer defect (docs/objdiff_data_relocation_defect_20260922.md lists it as one of 3 uncredited). Scorer-version effect, not a source gap.
- Storage: pdb_storage: update_speech_timers & reset_idle_vocalization_timer split 2 / ours 3 / PDB not public => ours right (csplit artifact; symbols.json "static": true fix);
  _ai_conversation_new split 2 / ours 2 / PDB NOT public, no January object references it (833 split objs scanned) => January static; ours external + ai_communication.h prototype.
- TU carries consumer-local prototypes of other files' functions (actor_is_fighting, unit_test_speech, unit_speak, sound_scripted_dialog_is_playing, actor_iterator_new/next, actor_perception_find_sense_position, actor_audibility_at_point, actor_handle_communication, ai_handle_allegiance_broken_notification) = admission blocker class (brief s9).
- talk_weight (parked instruction-scheduling): single real region at +0x35f: Jan `mov al,[ebp-3](cause); or al,[ebp-2](subject)`, ours operands swapped; slots identical.
  /Od 0x48ca30 (found by 10.0/5.0 constant scan; RTC weight/vocalization_lookup_type/sound_definition_index): subject_matches at [ebp-0x39] declared BEFORE cause_matches [ebp-0x3a];
  final `movzx ecx,[ebp-0x39]; movzx edx,[ebp-0x3a]; or ecx,edx` = `subject_matches | cause_matches`. Ours declares cause first and writes `cause_matches | subject_matches`.
- Q1 decl swap (subject first) / Q2 `subject | cause` / Q3 both: ALL byte-inert for talk_weight (or operands stay [ebp-2],[ebp-3]; slots unchanged).
  => operand canonicalisation not reachable from local spelling; consistent with park class (instruction-scheduling). talk_weight RETIRE.
- P7 P1 + transform_avoidance_vector per /Od 0x467840 (zero; 3x point_from_line3d): transform stays EXACT 144, get_avoidance_direction EXACT,
  unit 31/5 unchanged => production's hand-expanded transform can take the authentic helper form (rule-6 hygiene) at zero risk.
- P8 P2 with /Od literal order (divergence zero-init after the offset terms): 768, alndiff blocks 47 (P2: 18). /Od order worse (matches w1 ta9).
- P9 P3 but vector_to_origin in its own block (escape law) + dummy_normal loop-scope: frame back to 0x438 (= January), 18 blocks, SAME t-def residual.
- P10 P9 + /Od declaration order (offset, divergence, result, collision; loop: cylinder, dummy_normal, object_t): inert (18 blocks).
- STOP RULE for test_avoidance_vector: 5 own shapes (P2,P3,P8,P9,P10) + w1 ta1-ta9 leave the same residual: January loads ray->offset.j/k,
  divergence.j/k (fld [edi+8]..) only AFTER the preceding point_from_line3d's store to offset.z (fstp [ebp-0xc]) and defines the inline `t` by fld/fst;
  ours hoists those loads above the stores as integer prefetches. = a memory-dependence (alias) ordering difference with no identified source construct.
- actor_looking /Od callee recheck: 0x42e020 is normalize2d (magnitude2d 0x42def0, fabs(m-0.0)<1e-4) x4 = our 4 normalize2d; dot2 x2, dot3 x3, cosine x5, fabs(realcmp) x1 all equal our spellings. No helper-spelling lead left.

## ai_communication_event (structural lane 0923 v10h, NOT landed)
- claude-structural-20260923 scratch/lane/w/ai__ai_communication/REPORT.md: v10h = size 8064 (=Jan), relocs 336/336, frame 0x126c, 74 real hunks;
  needs allegiance_incident_type enum moved to game_allegiance.h (12-includer sweep). Remaining: (1) play_type read uninitialised on reply path
  (January-bug class, HELD; v6 shows it costs frame +8 anyway), (2) normalize2d at alignment site was provider-link-blocked (action_charge NODUP) --
  NOW action_charge.c uses header normalize2d (round-1 provider repair) so (2) is likely unblocked (not measured here), (3) register ties.
- 74 real hunks with ties left: not an exactness route. RETIRE (fidelity packet v10h+v8 may land at zero credit).

## ai_debug
- render_actor: 97 REAL regions of 1165 (rest addressing-only); frame/slots/relocs equal (0923 structural b5). Sample REAL regions = x87 staging:
  Jan `fmul 0.1f; fst [ebp-0x1c]; fadd [ebp-0x10]` vs ours `fstp; fld; fadd` at the hand-written mid_point/p1/p3 (lines ~1398-1406) - CSE'd product
  shared by + and - so January wrote the product twice (not two point_from_line3d calls). No lever. RETIRE.
- .data 56 coff-identical (scorer '$' defect). 33 storage DIFFs: split 2 / ours 3 / PDB not public => ours right; symbols.json "static": true needed.
- 19 `_code_` placeholders have atlas names (25 named anchors verified at off-0x400000); rename batch measured byte-inert in 0914 w2 / 0915 n2 (pending since).

## FINAL
All four: RETIRE for closure (see MANIFEST). No ADMISSION (every object has a non-exact residual), no OWNER-only path
(ai_communication_event's held play_type read does not close it: v6 frame +8).
