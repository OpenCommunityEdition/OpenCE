# source/cseries/profile

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_compare_profile_sections"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\profile\\final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\profile\\final.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\profile\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\profile\\storage_only.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\profile\\symbols.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\profile\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\profile\\od_551e50.txt"
]

## production_changes
This lands as two separate packets. Apply both patches with `git apply`; each passes `--check` (rc=0), and CRLF line endings are kept.

PACKET A: storage metadata from cachebeta.pdb publics. It can land NOW. No owner question is involved, and nothing in the object regresses.
(1) Apply scratch/w/profile/storage_only.patch to source/cseries/profile.c. It removes `static` from:
  - the compare_profile_sections prototype (prototypes section, about line 414);
  - the compare_profile_sections definition (about line 1586);
  - `header_strings[...]` (line 433);
  - `format_strings[...]` (line 439).
(2) Apply scratch/w/profile/symbols.patch to config/symbols.json. It edits four lines in place; do not re-serialize the file.
  - line 1630 `_compare_profile_sections`: remove "static": true
  - line 22481 `_header_strings`: remove "static": true
  - line 22482 `_format_strings`: remove "static": true
  - line 22952 `_profile_globals`: add "static": true
(3) Regenerate csplit.
Steps (1) and (2) must land together (atomic). Evidence: cachebeta.pdb lists these PUBLIC symbols at the matching addresses: _compare_profile_sections @0007E840, _header_strings @002DCD30, _format_strings @002DCD38. _profile_globals is not public, and only profile.obj references it. Regenerating the split into scratch changed only profile.obj's four storage classes; the other 832 objects are byte-identical.

PACKET B: the comparator body. It needs the OWNER RULING first; do not land it before that ruling.
scratch/w/profile/production.patch equals Packet A plus these body changes in compare_profile_sections:
  - `int result = 0;` becomes `int result;`;
  - add `else\n\t\t\t\t\tresult = 0;` after the `first_average<second_average` arm;
  - add the same line after the `recent_elapsed_timebase<` arm;
  - add a comment-only `/* BUG (preserved for exact matching): ... */` block in the default arm. The comment changes no bytes.
On landing, retire the park in config/parked.json: the entry with unit source/cseries/profile, function _compare_profile_sections (about lines 224-24x). Retire it in the same commit, because moved parked-function bytes fail PROGRESS.

There are no header edits and no other translation units are touched. _profile_frame_get_value's objdiff score of 91.46% is already credited through build/semantic_report.json (accepted_ledger/hidden_exact, internal jump-table labels), so it needs no semantic_matches entry.

## evidence
BASELINE: gate --all gives 43 exact / 1 residual. _compare_profile_sections is [size 304!=288, sha].

JANUARY BYTES:
- Every mode returns a constant block with no live result register.
- The invalid-mode path runs display_assert(844), then system_exit(-1), then `mov eax,[ebp+8]`. [ebp+8] is the dead first-parameter home, the same slot the average mode reuses as its fidiv spill.
- Production's `result = 0` initializer keeps a live EDI result. That adds push/pop and brings the size to 304.

/Od FIRST-PARTY (halo_cache_symbols.exe fn 0x551e50, dump in scratch/w/profile/od_551e50.txt):
- result is at [ebp-4], and its /RTCu shadow byte is at [ebp-0x15].
- The single epilogue calls _RTC_UninitUse("result").
- The explicit `result = 0` stores in the average and total modes are both present.
- There is a single exit.
- The compiler emits that check only when a path can leave `result` unset, so the original source declares `int result;` with no initializer.
- HCEX.pdb has no record for this function.

PROBES:
- P1: /Od shape. Size and relocations are now equal (288/8); only the second x87 compare differs. /Od's SSE comisd output is consistent with `<`, and January's fxch/test ah,5 bytes decide it.
- P2: `first_average<second_average` gives 44/44 EXACT.
- P3: the same body with `int result = 0;` stays non-exact because of the live register. No defined default value can reproduce `mov eax,[ebp+8]`.
- P4/final: dropping `static` on the 3 PDB publics changes no bytes and still gives 44/44 EXACT.
- P5: the storage-only packet, measured alone, gives 43/1 (same as baseline).

WHOLE-OBJECT AUDIT (final.obj):
- Against the current split: sections all ok, and object_audit fails only on 4 storage rows, which are split metadata.
- I ran csplit.exe with an edited copy of symbols.json into scratch/w/profile/split, then ran object_audit against that output with audit_with_split.py. Result: OBJECT AUDIT PASS. All 133 symbols match (name, storage, section, offset) and every January-owned section is ok.
- fn_equal: 128/128 January-owned sections equal against both the current and the regenerated split.
- provider_link against the regenerated split: PASS. The 13 surplus items are pooled literal COMDATs.
- surplus_identity: 0 code COMDATs.
- report.json: data 100%.
- fake_match_scan: 7 leads, the same as production (inherited QUERY_TIMEBASE asm); nothing new.
- No object_admission_rejections entry exists for this unit.

HISTORY: this exact body was found before and held, in opus5 w3 (p1) and in Lane D refresh (needs-ruling, exact_under_ruling). The Storage/PDB-public correction and the regenerated-split audit are new in this run.

## blockers
Only one blocker remains: an owner policy ruling. astra_90pct_rejected_hypotheses_20260920.md item 8 is a user exclusion that names "_compare_profile_sections's uninitialized return". claude_structural_owner_packet_20260923.md section 4 asks the owner whether to admit the RTC-attested rows as a first class; this function is one of them. No ruling appears in docs/ or memory.

Points for the ruling:
- The uninitialized read only happens after the fatal system_exit(-1), so no reachable path returns an indeterminate value.
- The first-party /Od build attests _RTC_UninitUse("result").
- The idiom already has a precedent: bitmaps _bitmap_format_to_a8r8g8b8 was accepted with it (w2 verifier R5).
- The BUG comment follows the methodology's original-bug policy.

Packet A (the storage metadata) is not blocked.

## reopen_criteria
If the owner lifts item 8 for RTC-attested post-fatal-assert uninitialised results:
- land Packet B (production.patch) together with Packet A;
- retire the park;
- run a full ninja build and the stable sweep.
The object should then be COMPLETE.

If the owner rejects it: keep Packet A landed and keep the natural 304B body parked. No further spelling search is warranted, because probe 3 shows that no defined default value can match. Reopen only on a changed owner policy.

## task notes
This run started fresh: scratch/w/profile/ did not exist, so the earlier cut-off run left nothing to recover for this unit.

New in this run:
(1) cachebeta.pdb publics show the storage metadata is wrong in 4 places:
  - `_compare_profile_sections`, `_header_strings` and `_format_strings` were EXTERNAL in January;
  - `_profile_globals` was static.
  Correcting these is a separate, independently landable fix.
(2) An emulated split regeneration (csplit run into scratch only; no tracked file or build/ touched) proves the whole-object audit PASSES with final.c.

The comparator body itself was already found and held by earlier lanes. It stays behind the owner's item-8 ruling. The helper scripts are in scratch/w/profile/: storage_table.py, xref_census.py, emulate_audit.py, audit_with_split.py, split_compare.py, fn_equal.py and provider_link_regen.py.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\profile\LEDGER.md
