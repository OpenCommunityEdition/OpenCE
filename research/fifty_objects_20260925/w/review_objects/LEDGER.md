# Review: source/objects/objects OBJECT_COMPLETE_CANDIDATE (Layer1 provider diffs)
Reviewer slug scratch/w/review_objects. Started 2026-09-23T18:47:41-07:00.

## R1 inputs verified
- Layer1 diffs (scratch/w/provider/patch/layer1/*.c.diff) apply to production (HEAD e9e62b78, source/ clean) and
  the patched files == cand1.c/cand2.c modulo CRLF (files are i/lf w/crlf; diffs carry CR; integrator should use git apply).
## R2 objects (objects.c unchanged; compiled production objects.c -> cand.obj)
- gate --all: 121/121 EXACT (objects.gate.txt). cand.obj == build/base objects.obj in every section except .debug$S; symbols identical.
- object_audit: PASS, 220 January-owned sections ok (incl .data ?section@?1??objects_update@@9@9 1530 B, .bss 13 B),
  225 January symbols 0 differ. pdb_storage: 225 symbols 0 disagreements.
- surplus_identity: 26 candidate-only code COMDATs, 0 not identical.
- provider_link production: FAIL x4 exactly as claimed (_point_in_sphere, _real_random, _signed_angular_difference,
  _valid_real_vector3d_axes2); _valid_real_vector3d PASS (bored_camera copy already ANY) -> rejection entry's 5th symbol is stale.
## R3 providers (independent compiles, *.cand.obj vs *.prod.obj)
- gate rows byte-identical text: action_charge 21+1 (pre-existing _action_charge_perform size), player_control 49/49,
  bored_camera 9/9, path_obstacles 15+1 (pre-existing _obstacles_test_circle sha).
- objcmp2.py (own COFF parser incl COMDAT selection): only SEL 1->2 on _real_random / _signed_angular_difference +
  _interpolate_scalar / _valid_real_vector3d_axes2 / _point_in_sphere; bored_camera NEW _magnitude_squared3d,
  _valid_real_normal3d (ANY) + __real@3f800000. No content/storage/offset change anywhere.
- object_audit prod vs cand: January-owned lines identical in all 4 (player_control PASS, bored_camera PASS; the
  other two FAIL(1) pre-existing both sides).
- provider_link: all 4 cands PASS for new surplus; path_obstacles full surplus keeps 3 PRE-EXISTING invented-name
  COMDATs (_distance_squared2d_inline etc., NO January provider) - unchanged by cand2, not an objects issue.
- surplus identity of cand providers: all identical except those 3 pre-existing invented names.
- Board census (symcensus.txt): every other definer of the 5 helpers + _valid_real_normal3d/_magnitude_squared3d is ANY and
  == January selected; the ONLY NODUP definers are the 5 hand copies being removed.
## R4 links (Link.Exe, pairlink.py)
- objects x each cand provider: 0 LNK2005/LNK1169 both orders; positive control objects x each PROD provider: LNK2005 on
  the expected symbol (detector works). Combined objects+4 cands: 0 forward, 0 reverse.
## R5 genuine uses (January relocs, tinfo)
- action_charge_setup @16b _get_global_random_seed_address/@171 _real_seed_random (real_random inlined, cand line 976);
  player_control_modify_desired_angles @51f _interpolate_scalar; bored_camera_update @1e8 _valid_real_vector3d_axes2;
  obstacles_get_discs_in_sphere @114 _point_in_sphere. signed_angular_difference 3 uses at cand 1806-1808.
## R6 cross-lane composition
- action_charge lane (scratch/w/action_charge/candidate.c, perform EXACT, name-count-sensitive) + provider action_charge diff
  (plain `patch`, CRLF->LF only): ac_combined.c gate 22/22 EXACT; objcmp2 vs lane candidate.obj: only _real_random SEL 1->2.
  weapons lane p4 action_charge == provider cand1 (modulo CR) -> same edit, apply once.
- `git apply --check` of all four layer1 diffs on the clean tree: OK.
## R7 house rules / fake-match
- Diff only REMOVES #define renames, #undefs, 2 consumer-local prototypes (bored_camera) and 5 hand copies; every remaining
  call site is a genuine January use (R5). No new construct -> no strip test needed (the candidate IS the plain spelling).
- fake_match_scan on objects.c + 4 cands + 4 prod files: 0 leads. objects.c: no float/_code_/bss_/pragma/asm/volatile/rename.
- Pre-existing, NOT introduced (non-blocking): objects.c has 15 single-line parameter lists (14 static prototypes lines
  114-140, `void object_pvs_clear(void)` line 517) vs house rules 1/2; 47/361 Matching Halo objects carry the same kind of
  deviation (e.g. game_engine_stub.c 30, rasterizer.c 5) so it is not an admission precedent blocker; whitespace-only fix.
  path_obstacles keeps 3 invented-name COMDATs (_distance_squared2d_inline etc.) and player_control keeps the limit2d rename
  (both pre-existing; outside objects admission). bored_camera cand ends with an extra blank line (cosmetic).
## R8 DATA CREDIT GAP (blocking for the production-change list as written)
- build/report.json (frozen objdiff 3.3.1): objects .data (?section@?1??objects_update@@9@9, 1530 B, 1 reloc to
  ??_C@_0P@LFOALBOL@objects_update?$AA@) scores 99.74% -> strict matched_data 3785/5315 after all semantic passes.
  coff_compare section_info_resolved target==base (identical) => the known '$'-literal objdiff defect.
- config/semantic_data_matches.json has NO objects entry. Simulation (tools.audit_object_admission.audit on copies with
  objects metadata.complete=True + rejection removed): WITHOUT an entry -> revoked "source/objects/objects (0 unmatched
  functions, 1530 unmatched data bytes)", decision completion-label-contradicted. WITH the single-section entry in
  semantic_data_entry_objects.json (size 1530, relocation_count 1, normalized_sha256 e94bb794...b74) -> 0 revoked,
  0 contradicted. apply_semantic_data_matches also requires the unit be marked complete (entry and status must land together).
## VERDICT: approve (conditional: land semantic_data_entry_objects.json together with the Matching status). See REVIEW.md.
