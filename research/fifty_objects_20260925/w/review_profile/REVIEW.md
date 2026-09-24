# Adversarial review: source/cseries/profile (worker verdict FUNCTIONS_EXACT_OBJECT_BLOCKED)
Reviewer notes appended as I go.

## Step 0 - inputs read
- WORKER_BRIEF.md, scratch/w/profile/LEDGER.md, production.patch, storage_only.patch, symbols.patch.
- House rules doc, ledgers profile_obj_dump_comparator_natural_boundary_20260905.md, profile_obj_opus5_150k_w3_20260914.md,
  profile_obj_crossbuild_two_arm_authentication_20260919.md, parked.json entry (lines 224-242),
  astra item 8 (names this fn's uninit return as excluded), claude_structural_owner_packet_20260923.md s.4
  (row _compare_profile_sections pending "admit RTC-attested rows as first class?").
- Context: Packet B is correctly HELD by worker (astra item 8 + brief: authentic-bug reproduction = report, don't land).
  Owner ruling 2026-09-20 (memory lane-a) admits uninit reads as target-proven BUG only once strict exact, with owner approval;
  s.4 decision still open. So the review question is (a) is the 44/44 claim true, (b) is the /Od evidence accurate,
  (c) is Packet A (storage metadata) correct and safe to land now.
- git status of worktree: clean (no tracked edits). HEAD e9e62b78.

## Step 1 - patch reproducibility (applied in a non-repo temp copy; NOTE git apply inside a repo subdir silently skips paths)
- production.patch on HEAD profile.c == scratch/w/profile/final.c (cmp identical, CRLF kept).
- storage_only.patch on HEAD profile.c == storage_only.c (identical).
- symbols.patch on HEAD symbols.json == symbols.json.edited (identical; symbols.json is CRLF and the patch keeps CRLF).
- git apply --check on the worktree: all three rc=0. Worktree still clean.
- Integrator caveat: production.patch CONTAINS Packet A's profile.c hunks; after Packet A lands it will not apply
  cleanly - Packet B needs a body-only re-diff at that time.

## Step 2 - independent gates
- final.c: gate --all => exact 44 / residual 0 / unwritten 0 (scratch/w/review_profile/cand.obj). CONFIRMED.
- storage_only.c: 43 / 1 (_compare_profile_sections [size 304!=288, sha]) = production baseline 43/1. CONFIRMED.

## Step 3 - storage evidence (Packet A)
- object_audit(cand.obj vs current split): all January-owned sections ok; 4 symbol DIFFs = storage only
  (_compare_profile_sections 3/2, _header_strings 3/2, _format_strings 3/2, _profile_globals 2/3). Surplus = .drectve + 13 pooled literals.
- cachebeta_publics.txt: _compare_profile_sections [0007E840] (=symbols.json file_offset 518208), _header_strings [002DCD30]
  (=3001648), _format_strings [002DCD38] (=3001656) are PUBLIC. No public at 0031DF40 (_profile_globals, 3268416) and no
  _profile_globals public. pdb_storage.py: exactly these 4 disagreements among 133 split symbols. CONFIRMED.
- Link-conflict check (symscan.py over build/base and build/split, all .obj): the 4 names are defined/referenced ONLY by
  profile.obj in both trees; no other source file names them. January's own link had them as unique publics => no LNK2005 risk.
- config/symbol_ownership.json has no profile entry -> Packet A needs no snapshot change there.

## Step 4 - independent split regeneration + audits
- Rebuilt a config copy (config.json/contribs/relocs/splits + HEAD symbols.json with symbols.patch applied in a non-repo temp dir)
  and ran build/tools/csplit.exe -i cachebeta.exe -p scratch/w/review_profile/cfg -o scratch/w/review_profile/split (4 s).
- split_compare: 833 objects, 0 missing, exactly 1 differs (profile.obj), and only the 4 storage classes flip. CONFIRMED.
- object_audit vs regenerated split: cand.obj (final.c) PASS 133/133 symbols, all sections ok;
  storage_only.obj FAIL(1) = only .text _compare_profile_sections 304/288 (0 symbol diffs);
  production base.obj FAIL(4) = comparator .text + 3 storage rows => Packet A source+symbols must land ATOMICALLY. CONFIRMED.
- objdiff 3.3.1 one-unit reports (mk_objdiff.py): now = 96.83% / 42 fn / data 100%; Packet A (regen split vs storage_only.obj)
  = IDENTICAL numbers (data 100%, 0 change); full (cand) = 99.15% / 43 fn, comparator 100%, only _profile_frame_get_value 91.46%
  (inherited, semantic_report hidden_exact). Packet A is metric-neutral.

## Step 5 - /Od attestation re-derived
- odbuild.py fn 0x551e50 re-dumped: byte-identical to worker's od_551e50.txt.
- Identity: refs "cseries\profile.c", !"unreachable", compare_type global, +8 active, +0x5c8 sample_count, +0x5e0 total,
  +0x20 recent, csstrcmp(first->name, second->name) => this is compare_profile_sections.
- 0x5520c0 bytes 72 65 73 75 6c 74 00 = "result"; callee 0x92db80 builds "The variable '" ... "' is being used without being
  initialized." => _RTC_UninitUse. RTC init flag byte [ebp-0x15] cleared at entry, set to 1 before every `result` store,
  tested at the single epilogue. CONFIRMED: the later first-party source had `int result;` without initializer.
- Explicit `result = 0` stores: 0x551fd7 (average mode equal arm) and 0x55205d (total mode equal arm). CONFIRMED.
- Single exit: every arm jumps to 0x55209b; one ret. CONFIRMED.
- Avg compare: comisd [avg0] vs [avg1] ja->-1; then comisd avg1 vs avg0 ja->1: SSE `a<b` = comisd b,a; ja, so it does not
  distinguish `a<b` from `b>a`; January x87 bytes (fxch/test ah,5) decide. final.c keeps production's `<` spelling (context line,
  not a change). No issue.
- Averages are double in /Od (movsd/divsd, int64->double helper call) - final.c uses double; not a `real` rule violation.

## Step 6 - park/semantic pin stability under Packet A
- fnsha.py: _compare_profile_sections target sha d94ba79b... identical in current split AND regenerated split; base sha aebbef78...
  identical in build/base, gate base.obj and storage_only.obj => parked.json measurements stay valid after Packet A (no stale park).
- objcmp.py base.obj vs storage_only.obj: 148/148 sections, only .debug$S differs; symbol diffs = the 3 storage flips. Packet A is
  byte-inert on every code/data section. (gate base.obj vs build/base: only .debug$S => gate == ninja modulo debug.)
- split_compare found no section-info change anywhere => any pinned target sha (semantic_report etc.) remains valid.

## Step 7 - surplus / provider links
- provider_link.py (current split) on cand.obj: 13 pooled literals PASS; _compare_profile_sections/_format_strings/_header_strings
  FAIL "NO JANUARY PROVIDER" (expected: current split has them static) => symbols.json change is REQUIRED with the source.
- provider_link against MY regenerated split: SELECTED-PROVIDER LINK PASS (13 literals). surplus_identity: 0 candidate-only code
  COMDATs. object_audit surplus = .drectve + 13 pooled literals only (pre-existing class, same as production).

## Step 8 - STRIP TESTS / defined alternatives (Packet B)
- S1 s1_init0.c = final.c with `int result = 0;` (explicit else arms kept): 43/1, comparator [sha] (size 288 but alndiff shows
  xor esi,esi live result + push/pop edi, `mov eax,esi` returns, and the default path loses `mov eax,[ebp+8]`). Uninit decl is
  load-bearing. Matches worker P3.
- S2 s2_nocomment.c = final.c minus the BUG comment: 44/44 EXACT => comment byte-inert. CONFIRMED.
- S3 s3_default0.c = uninit decl + defined `result = 0;` after the fatal assertion: 43/1 [sha]. The only defined fallback also fails.
  => the exact body requires the undefined read on the post-system_exit path; it is NOT a decoration that can be stripped. Its
  legitimacy therefore rests entirely on the /Od _RTC_UninitUse("result") attestation + January `mov eax,[ebp+8]` (double attested,
  step 5) and on the OWNER ruling (astra item 8 names this exact construct as excluded; structural owner packet s.4 open).
  Worker correctly does NOT land it. Not re-probed further (worker + two prior lanes measured the same body).

## Step 9 - misc checks
- fake_match_scan final.c: 7 leads = production (inherited QUERY_TIMEBASE rdtsc asm, lines 327-333); nothing new.
- object_admission_rejections.json: no entry for source/cseries/profile. semantic_report: profile fns accepted_ledger/semantic_exact;
  _profile_frame_get_value hidden_exact (as worker said).
- No test/tool pins these symbol names or the storage flags (grep over tools/tests/config: only parked.json, symbols.json and the
  Lane D manifest mention them; the geosphere test's symbols.json hash is a synthetic manifest, it never reads the real file).
- Lane D manifest ruling_queue[45] records the same body as needs-ruling / exact_under_ruling with the same RTC evidence: history
  claim CONFIRMED. Non-static prototype in profile.c prototypes section has in-file precedent (`void find_profile_section(` l.407);
  moving it to profile.h is NOT advisable (25 includers, declaration-count blast) and not required (sole consumer is profile.c).

## VERDICT: APPROVE the worker verdict FUNCTIONS_EXACT_OBJECT_BLOCKED and the two-packet plan.
- Packet A (storage_only.patch + symbols.patch, atomic, then csplit regen via the normal build): evidence = cachebeta.pdb publics
  (3 public at the exact file offsets, _profile_globals absent), byte-inert on every code/data section, parks stay valid (target and
  base sha unchanged), objdiff metrics unchanged, split regen touches only profile.obj's 4 storage classes, object_audit vs regen
  split goes from FAIL(4) to FAIL(1) (only the parked comparator .text). Landable now.
- Packet B: 44/44 strict EXACT and whole-object audit PASS vs regenerated split are real, but the undefined read is load-bearing
  (S1, S3 fail) - it is only admissible through the owner ruling (astra item 8 / structural owner packet s.4). Correctly HELD.
Non-blocking issues for the integrator:
 1. production.patch CONTAINS Packet A's profile.c hunks. After Packet A lands it will not apply; re-diff Packet B body-only
    (final.c vs post-A profile.c) at ruling time, or apply production.patch INSTEAD of storage_only.patch - never both.
 2. Land Packet A source + symbols.json in ONE commit and run a full ninja (split regen + base) before any stable_verdicts
    (stale-base hazard). Expected result: profile 43E/1R unchanged, park validates, 0 regressions.
 3. When Packet B lands: retire config/parked.json entry (lines 224-242, unit source/cseries/profile, _compare_profile_sections)
    in the same commit.
