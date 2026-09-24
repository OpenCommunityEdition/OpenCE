# profile / _compare_profile_sections ledger (lane claude/fifty-objects-20260925)

## Baseline (2026-09-23)
- gate.py source/cseries/profile --all: exact 43 / residual 1 / unwritten 0.
- residual: _compare_profile_sections 288 [size 304!=288, sha].
- Prior ledger docs/object_matching_logs/profile_obj_dump_comparator_natural_boundary_20260905.md:
  donor exact 288B body used an UNINITIALIZED return (rejected as UB); natural body has defined zero fallback -> 304B.
  January returns "first argument's low pointer bits after system_exit(-1)" per that ledger (to verify).

## Finding 1: January bytes (tinfo/dis_range)
- January: multiple return blocks with constants (or eax,-1 / xor eax,eax / mov eax,1 / csstrcmp eax), NO result register;
  default path: display_assert(...,0x34c=844,1); system_exit(-1); `mov eax,[ebp+8]`; add esp,0x14; ret.
  [ebp+8] is the dead param slot that the average case also reuses as the fidiv int spill.
- Ours (production): `xor edi,edi` result=0 initializer keeps a live edi result -> +push/pop edi, 304B.
- The only differences are the result register and the default-path return value. A defined return on the
  default path cannot produce `mov eax,[ebp+8]` (it is not any meaningful value).

## Finding 2: /Od first-party readout (halo_cache_symbols.exe fn 0x551e50, file-string ref cseries\profile.c)
- saved: scratch/w/profile/od_551e50.txt
- [ebp-4] = `result` (int); [ebp-0x15] = /RTCu "initialized" flag byte for it; at the single exit
  `cmp byte [ebp-0x15],0; jne; push 0x5520c0 ("result"); call _RTC_UninitUse`.
  => /RTCu instrumentation is emitted only when the compiler sees a path where `result` may be read
  uninitialised: FIRST-PARTY ATTESTATION that the original source declares `int result;` WITHOUT an
  initializer, assigns it in every real branch, leaves it unset on the default !"unreachable" path,
  and returns it once at the bottom (single exit).
- Shape: if (a->active && !b->active) result=-1; else if (b->active && !a->active) result=1;
  else switch(profile_globals.compare_type): case 0 result=csstrcmp(a->name,b->name);
  case 1: avg0 = cnt0==0 ? 0 : total0/cnt0 (ternary temp [ebp-0x24]); avg1 likewise; if (avg0>avg1) result=-1;
    else if (avg1>avg0) result=1; else result=0;  (explicit result=0)
  case 2: if (a->recent>b->recent) result=-1; else if (a->recent<b->recent) result=1; else result=0;
  default: assert !"unreachable" (line 0x358 in later build vs 0x34c January).
- Named double locals avg0 [ebp-0xc], avg1 [ebp-0x14]; user-area: result, 2 doubles.
- HCEX.pdb: DIA2Dump -sym compare_profile_sections returns nothing (tool verified working on unit_update); no HCEX evidence for this fn.

## Probe 1 (cand.c): /Od shape - `int result;` uninitialised + explicit `result = 0` else arms, single exit
- hypothesis: /RTCu attests uninit result; January's constant return blocks come from return-block
  specialisation of a single-exit result with no initializer (no live result register).
- result: size 304->288 (EQUAL), relocs 8, only sha differs. alndiff: only diff = second x87 compare:
  January `fxch st(1); fcomp st(1); test ah,5; jp` (first<second) vs ours `fcomp; test ah,0x41; jne`
  (I spelled second_average>first_average from the /Od comisd). SSE comisd swaps operands for `<`, so
  /Od is consistent with `first_average<second_average`; January bytes decide -> revert to `<`.

## Probe 2 (cand.c): probe 1 + `else if (first_average<second_average)` (January x87 fxch/test ah,5 order)
- result: gate.py --all == exact 44 / residual 0 / unwritten 0. _compare_profile_sections EXACT (288B/8 relocs).
  alndiff leftovers are only symbol vs defined-noncode spelling of pooled literals (gate treats equal).
- The body is the /Od-attested source shape: uninitialised `int result;`, explicit result=0 equal-arms,
  single exit. Only the unreachable post-system_exit(-1) path reads result unset (== /RTCu evidence).

## Probe 3 (var_init0.c): probe 2 but `int result = 0;` (defined default)
- result: residual [sha]; the initializer creates a live result register (xor esi,esi; push edi; mov eax,esi ...).
  => the uninitialised declaration is LOAD-BEARING; no defined default value reproduces January's
  `mov eax,[ebp+8]` (it is the dead first-parameter home, not any meaningful value).

## History check (do not repeat)
- This exact body (uninit result + explicit else result=0 arms) was already measured EXACT by:
  opus5 w3 (profile_obj_opus5_150k_w3_20260914.md, candidate p1) and Lane D refresh
  (claude_lane_d_refresh_manifest_20260922.json, group A "needs-ruling", exact_under_ruling=true) which
  found the same /Od _RTC_UninitUse("result") evidence. Lane D also measured the first/second locals byte-inert.
- Owner exclusion: astra_90pct_rejected_hypotheses_20260920.md item 8 names "_compare_profile_sections's
  uninitialized return". Pending decision: claude_structural_owner_packet_20260923.md section 4 asks to admit
  RTC-attested rows (this fn among them) as a first class. No ruling found in docs/ or memory.
- Precedent: bitmaps.obj::_bitmap_format_to_a8r8g8b8 landed with the identical idiom (w2 verifier R5).

## Whole-object audit on cand.obj (probe 2)
- gate --all: 44/44 EXACT. object_audit: all January-owned sections ok; 1 symbol DIFF:
  _profile_globals storage split 2 / cand 3.
- cachebeta.pdb publics (scratch/w/profile/storage_table.py): 4 split-vs-PDB disagreements:
  _compare_profile_sections (PDB public @0007E840 = this fn's address), _header_strings (@002DCD30),
  _format_strings (@002DCD38) are PUBLIC in January => EXTERNAL; split+source both say static (3).
  _profile_globals is NOT public => static; source static (3) is right, split (2) wrong.
  Pattern check: every other profile static (sections_update, frame_build_string, string_starts_with,
  timer helpers...) is absent from publics and every external present -> PDB is consistent evidence.
- xref census (scratch/w/profile/xref_census.py): _profile_globals referenced only by profile.obj.

## Probe 4 (cand_ext.c): cand.c + drop `static` on compare_profile_sections (prototype+definition),
##   header_strings, format_strings (PDB publics => external in January)
- gate --all: 44/44 EXACT (storage change byte-inert for all code/data).
- storage_table: candidate now == PDB truth on all 4 disputed symbols; remaining disagreements are split
  metadata only (symbols.json).
- object_audit (real split): FAIL(4) = the 4 split storage rows. emulate_audit.py with split storage
  corrected in memory to PDB truth (_compare_profile_sections=2 _header_strings=2 _format_strings=2
  _profile_globals=3): OBJECT AUDIT PASS, 133/133 symbols, all January-owned sections ok.
- report.json: data 100% (.bss/.data/.rdata); _profile_frame_get_value objdiff 91.46% is already credited via
  build/semantic_report.json accepted_ledger (hidden_exact, jump-table internal labels) - no action needed.

## Final candidate scratch/w/profile/final.c (= production + minimal hunks)
- body: `int result;` (no initializer), `else result = 0;` in average/total modes (January xor-eax return
  block + /Od explicit stores), default arm unchanged + BUG (preserved for exact matching) comment.
- storage: drop `static` on compare_profile_sections prototype+definition, header_strings, format_strings.
- gate --all (current split): 44/44 EXACT. Comment byte-inert (all non-debug sections equal to cand_ext.obj).
- fake_match_scan: 7 leads = production (inherited QUERY_TIMEBASE asm); no new lead.
- production.patch + symbols.patch written; both `git apply --check` rc=0.

## Split regen emulation (no tracked file touched)
- csplit.exe -i cachebeta.exe -p scratch/w/profile/cfg (config copy + edited symbols.json) -o scratch/w/profile/split
- split_compare.py: 833 objects, exactly 1 differs (profile.obj), only the 4 symbol storage classes change.
- audit_with_split.py (object_audit vs regenerated split): final.obj OBJECT AUDIT PASS (133/133 symbols,
  every January-owned section ok). Production base vs regenerated split: FAIL(4) => source+symbols.json
  must land ATOMICALLY.
- fn_equal.py: final.obj 128/128 January-owned sections equal vs both current and regenerated split.
- provider_link_regen.py (provider_link against regenerated split): SELECTED-PROVIDER LINK PASS (13 pooled
  literal COMDATs; the 3 names are no longer surplus). surplus_identity: 0 code COMDATs.
- Only blocker: owner ruling on astra item 8 / structural owner packet 20260923 section 4 (RTC-attested
  post-fatal-assert uninitialised result). Everything else in the whole-object audit passes.

## Probe 5 (storage_only.c): production body + storage fix only (no comparator change)
- gate: 43 exact / 1 residual (= baseline, comparator unchanged) -> storage change byte-inert.
- vs regenerated split: OBJECT AUDIT FAIL(1) = only the comparator .text section; 127/128 sections equal.
  => storage packet (storage_only.patch + symbols.patch) is INDEPENDENTLY landable (PDB-truth metadata, no
  house-rule question), zero regressions in profile.obj; no other split object changes (split_compare).
- Precedent for a non-static prototype in profile.c's prototypes section: `void find_profile_section(...)`.

## VERDICT
FUNCTIONS_EXACT_OBJECT_BLOCKED: final.c makes all 44 functions strict EXACT and, with symbols.patch applied and
csplit regenerated, the whole-object audit PASSES (sections, 133 symbols incl. storage, surplus provider links).
Sole blocker: owner ruling lifting astra item 8 for the RTC-attested uninitialised `result`
(claude_structural_owner_packet_20260923.md section 4). Stop rule: no further spelling search warranted -
no defined default value can reproduce January's `mov eax,[ebp+8]` (probe 3), and the body is /Od-attested.
