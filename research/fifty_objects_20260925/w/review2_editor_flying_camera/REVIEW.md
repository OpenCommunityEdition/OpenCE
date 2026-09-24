# REVIEW2 editor_flying_camera (adversarial reviewer, base 931ed8dc)

Proposal: OBJECT_COMPLETE_CANDIDATE; packet A (production.patch) + optional packet B (storage_optional.patch + symbols.json).

## Log
- HEAD 931ed8dc confirmed; tracked tree clean. `git apply --check` production.patch OK; storage_optional.patch OK.
- cand.c = HEAD file + production.patch (applied in a scratch copy): LF-normalised == worker F2.c (23447 B). CRLF file.
- /Od 0x527a20 re-read (od_spar.txt) + RTC 0x527c9c re-read with rtc_at.py: matrix -0x40/52, forward -0x54, left -0x68,
  up -0x7c, diff -0x90, hack_angles -0xa0/8. Statement list: set_focus(point, angles) pass-through (ebx+0xc pushed
  unchanged); cross(&up,&forward,&left) [push -0x68,-0x54,-0x7c]; normalize(&left); cross(&up,&matrix.up,&diff);
  normalize(&diff); angle_between(&up,&matrix.up); dot(&forward,&diff). -0x68 never read after normalize -> left dead
  in /Od too. Worker's /Od transcription VERIFIED.
- gate --all on cand.c (vs real split build/split): **21/21 EXACT**; raw fn sha 71253260755d0770 == worker F2.obj.
- object_audit(cand.obj): PASS (47 January symbols, 0 differ). production object_audit FAILs only on the fn.
- surplus list identical to production; per-section section_infos_equal cand vs base: 81 compared, only
  set_position_and_roll and .debug$S differ (debug$S not January-owned). surplus_identity (production): 17 code
  COMDATs 0 not identical. provider_link(cand.obj): SELECTED-PROVIDER LINK PASS (35 rows).
- report.json: .rdata 100%; .bss 124 + .data 52 credited by camera-static-sections grouped entry (unchanged bytes).
  allow_incomplete_unit only relaxes a check (semantic_progress.py:678) -> harmless after completion.
- /W3 (worker warn.py): cand vs HEAD identical warning set (15 lines). fake_match_scan: 0 leads both.
- NEW independent attestation (assert-anchor drift, /Od fn dumps od_0x*.txt in this dir): January line -> /Od line:
  get_focus 120/121->124/125, set_focus 129/130->133/134, move_to_point 139->143, set_position 148/149->152/153,
  **set_position_and_roll 169/170->173/174, set_mode 302/308->306/312 (all +4)**, then update 340/356->345/361,
  set_scripted 387/414->392/419, flying_update 448-450->453-455, 518->523, 571->576 (all +5).
  Zero drift across January lines 170..308, which spans the whole set_position_and_roll body. The later revision's
  body includes `real_vector3d left;` + the cross/normalize pair (>=3 lines), so January's same span has the same line
  count -> the left lines were present in January too (unless an exactly compensating edit happened in the same span).
  This double-attests the /Od statement pair against January independently of the codegen argument.
- STRIP / sensitivity tests (mine, gate --all vs real split; raw fn sha):
  R1 strip `left` decl + cross/normalize pair: residual 20/21, sha c334539c (= worker P1, the 3 x87 blocks) -> pair is LOAD-BEARING.
  R2 drop only normalize3d(&left): 21/21 byte-identical (VC7 drops the dead call) -> /Od-verbatim pair and F1 equal.
  R3 drop the euler cast: 21/21 byte-identical (+C4133) -> cast BYTE-INERT.
  R4 keep `real_vector3d left;` declaration only: residual, sha c334539c -> NOT a declaration/name-count effect.
  R5 content control `cross_product3d(&forward, &matrix.up, &left)`: residual (sha 30733bf1) -> content-specific.
  R6 left pair moved BEFORE normalize3d(&forward): 21/21 identical (worker S5 = after diff cross: not exact). The /Od
     position lies inside the matching window; no position trickery needed.
- Packet B (storage) VERIFIED with a REAL csplit to scratch (not emulation): csplit -p <copy of config with the worker's
  symbols.json> -o scratch: control run with unmodified config == build/split byte-identical; packet-B run: of 833
  objects only editor_flying_camera.obj changes, and it is byte-identical to the worker's emulated split (2 storage
  bytes). candB (cand + storage_optional.patch, == worker F2_static.c) vs real B split: 40/40 January sections exact;
  object_audit (pointed at B split) PASS 47/0; provider_link PASS; /W3 unchanged.
  cachebeta_publics.txt: _editor_camera_speed and _editor_camera_speed_steps ABSENT (file-static); _editor_custom_render
  public. pdb_storage(production) shows exactly these 2 pre-existing disagreements. No other split/base object names them.
  symbols.json copy differs from config/symbols.json in exactly lines 14089 and 22443 (in place, +", \"static\": true").
- objdiff 3.3.1 (build/tools sha1 3130e428) scratch project: prod reproduces build/report.json exactly (code 4634/5065,
  .bss 50 / .data 22.24 / .rdata 100, fn 79.92618). candA: code 5065/5065, 21/21 fns, data unchanged. candB on real B
  split: identical data percentages (.rdata stays 100%) -> the feared static-anchor undercredit does NOT occur.
- grouped entry camera-static-sections snapshot (tools.semantic_progress internals): A -> both member pins unchanged
  (passes as-is); B -> only `_editor_camera_speed` owner.storage 2->3 in target AND base; sha/size/flags unchanged.
- /Od 0x527700 confirmed = editor_camera_set_focus (asserts "position"/"angles" at 133/134 = January 129/130 +4).
- Patches: `git apply --check --whitespace=error-all` clean for A, B, and A+B together at 931ed8dc. Worktree file is
  CRLF (autocrlf=true), blob LF; the patch is LF and applies cleanly.
- No other config pins: editor_camera_speed appears only in semantic_data_matches.json:115; no admission-rejection
  entry for the unit; audit_object_admission does not read PDB storage.

## House-rule / owner-gate review of the diff
- Rule 2/3/7/8/9/17/19: signature unchanged, explicit `return;` kept, RTC-authentic local names, no prototypes, `real`.
- Rule 6 / helpers: only existing header inlines (cross_product3d, normalize3d); surplus COMDAT set unchanged and
  identical to production; no new emission.
- Euler 3d->2d view cast (brief sec. 2 criteria): per-site /Od attestation (0x527ab6 pushes the angles param unchanged
  to set_focus 0x527700) + January bytes (fall-through pushes the parameter itself); prefix-compatible unions
  (yaw@0, pitch@4, real_math.h:236-248); byte-inert strip test (R3); strictly exact caller (21/21); disclosed in the
  worker ledger. NOT owner-gated (sec. 7 gates only load-bearing casts). First euler view cast in the tree -> the
  integrator's docs ledger must carry the disclosure. The parked.json evidence sentence preferring "copying instead of
  representation-pointer punning" is superseded: the copy form is not January (464 B), the pass-through is attested.
- Dead `left` pair (sec. 7 "dead or unattested named locals whose only role is the match"): RULED ADMISSIBLE.
  It is not a filler: (1) first-party /Od RTC name `left` and the verbatim statement pair with the same operands,
  position and declaration order; (2) independent January attestation via assert anchors (zero line drift across the
  function body, see above); (3) content-specific (R5, worker C1) and not a declaration/count effect (R4); (4) the
  function is MOD8-insensitive per the corpus. Its role is authentic January source, so it is outside "only role is
  the match" (cf. Lane A rule 26: dead locals/duplicate calls allowed when independent evidence authenticates that
  exact construct). The in-source comment discloses that `left` is never read. Integrator: record this ruling and
  the anchor-drift evidence in the object ledger; escalate to the owner only if they read sec. 7 as covering attested
  dead statements.
- fake_match_scan 0 leads; /W3 unchanged; no header edit, so no consumer TU can move.

## Verdict
APPROVE packet A (object code-complete: 21/21 strict EXACT, object_audit PASS, surplus identical, provider link PASS,
data unchanged and still credited). APPROVE packet B (real csplit regen verified; only this object changes; 40/40
sections exact on the regenerated split; objdiff 3.3.1 data credit unchanged; the grouped entry needs exactly
`_editor_camera_speed` owner.storage 2->3 for target AND base in the same commit). Recommend landing A+B together so
the Matching label also has 0 cachebeta-publics storage disagreements (with A alone the 2 pre-existing ones remain).
Integrator config ops: retire parked.json `_editor_camera_set_position_and_roll` (source/camera/editor_flying_camera);
config.json line 741 NonMatching->Matching after the full chain; reword the stale "remaining camera code function stays
residual" sentence in the camera-static-sections reason.
- Housekeeping: removed scratch csplit config copies and the 833-object split_B_all (results recorded above); kept split_ctl/ and split_B/ objects.
