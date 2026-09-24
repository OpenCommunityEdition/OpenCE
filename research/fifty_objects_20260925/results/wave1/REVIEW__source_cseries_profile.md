# review source/cseries/profile
approve=True

## checks
All notes are in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_profile\REVIEW.md. The worktree was clean at HEAD e9e62b78, and no tracked file was edited.

1) Patch reproducibility. I applied all three patches to a temp copy outside the repo. Running git apply inside a repo subdirectory silently skips the paths, so a temp copy was needed. production.patch gives a file byte-identical to final.c. storage_only.patch gives storage_only.c. symbols.patch gives symbols.json.edited. All of them keep CRLF, and `git apply --check` returns rc=0 for all three.

2) Independent gates:
- final.c: 44 exact, 0 residual (cand.obj).
- storage_only.c: 43/1, the same as the production baseline (43/1, _compare_profile_sections [size 304!=288, sha]).

3) object_audit of cand.obj against the current split: every January-owned section is ok. There are 4 storage-only symbol diffs (_compare_profile_sections 3/2, _header_strings 3/2, _format_strings 3/2, _profile_globals 2/3). The surplus is .drectve plus 13 pooled literals.

4) Storage evidence:
- cachebeta_publics.txt lists _compare_profile_sections [0007E840], _header_strings [002DCD30] and _format_strings [002DCD38] as publics. These equal the symbols.json file_offsets 518208, 3001648 and 3001656.
- There is no public at 0031DF40 (_profile_globals).
- pdb_storage.py finds exactly these 4 disagreements among 133 symbols.
- symscan.py over every .obj in build/base and build/split: only profile.obj defines or references the 4 names, so there is no LNK2005 risk.
- config/symbol_ownership.json has no profile entry.

5) Independent split regeneration. I built my own config copy with symbols.patch applied and ran build/tools/csplit.exe into scratch/w/review_profile/split. Of 833 objects, exactly 1 differs (profile.obj), and the only change is the 4 storage classes.
- Audit against the regenerated split: cand.obj PASS (133/133 symbols). storage_only.obj FAIL(1), only the comparator .text. Production base FAIL(4). So the source and symbols.json must land atomically.

6) Stability:
- The comparator's target sha (d94ba79b...) is unchanged in the regenerated split. Its base sha (aebbef78...) is unchanged in storage_only.obj. So the park stays valid after Packet A.
- objcmp of base.obj vs storage_only.obj: only .debug$S differs, plus the 3 storage flips.
- objdiff 3.3.1 one-unit reports: Packet A gives numbers identical to now (96.83%, 42 functions, data 100%). The full candidate gives 99.15%, 43 functions, comparator 100%. The remaining _profile_frame_get_value 91.46% is inherited and marked hidden_exact in semantic_report.

7) Provider links:
- provider_link.py against the current split: the 3 names FAIL as "no January provider", which is expected while the split still marks them static.
- Against my regenerated split: SELECTED-PROVIDER LINK PASS (13 literals).
- surplus_identity: 0 code COMDATs.

8) /Od attestation. I re-dumped 0x551e50 and it is byte-identical to the worker's file.
- The bytes at 0x5520c0 spell "result".
- Callee 0x92db80 builds "The variable '...' is being used without being initialized.", so it is _RTC_UninitUse.
- The RTC flag at [ebp-0x15] is set before every `result` store.
- The explicit `result = 0` stores are at 0x551fd7 and 0x55205d, and there is a single epilogue.
- January's bytes independently show `mov eax,[ebp+8]` after system_exit(-1), and [ebp+8] is reused as the fidiv spill. So the shape is attested by both builds.

9) STRIP TESTS:
- S1 (`int result = 0;`, keeping the else arms): 43/1 [sha]. alndiff shows a live esi result plus push/pop edi.
- S2 (BUG comment removed): 44/44 EXACT, so the comment is byte-inert.
- S3 (uninitialised declaration plus a defined `result = 0` after the assert): 43/1 [sha].
- So the undefined read carries the match and cannot be stripped.

10) Other checks:
- fake_match_scan final.c: 7 leads, the same as production (inherited rdtsc asm).
- No object_admission_rejections entry.
- No test or tool pins these names or storage flags.
- The Lane D manifest ruling_queue[45] and opus5 w3 history are confirmed.
- House rules: the body keeps double (/Od uses movsd/divsd), the enum constants, match_assert and the single return. The non-static prototype in profile.c has an in-file precedent (`void find_profile_section(`, line 407).

## issues
No blocking issues. The verdict FUNCTIONS_EXACT_OBJECT_BLOCKED is accurate, and Packet A (storage) is correct and can land now. Packet B is correctly held for the owner's ruling on astra item 8 / claude_structural_owner_packet_20260923.md section 4. The undefined post-system_exit read carries the match: the defined alternatives S1 (`int result = 0;`) and S3 (`result = 0;` after the assert) both stay non-exact. So Packet B is admissible only through that ruling, backed by the double attestation (/Od _RTC_UninitUse("result") plus January's `mov eax,[ebp+8]`).

Non-blocking points for the integrator:

(1) scratch/w/profile/production.patch already contains Packet A's profile.c hunks. After Packet A lands it will no longer apply. At ruling time, re-diff Packet B as a body-only patch: final.c against the post-A profile.c. Otherwise apply production.patch instead of storage_only.patch, never both.

(2) Land Packet A as one commit containing both storage_only.patch (source/cseries/profile.c) and symbols.patch (config/symbols.json, 4 lines edited in place, CRLF). Then run a full ninja so the split and base regenerate before any stable_verdicts are taken (stale-base hazard). Expected result:
- profile stays 43 exact / 1 residual;
- the parked comparator's target and base shas are unchanged, so the park still validates;
- no other object changes (split regen changes only profile.obj);
- objdiff metrics are unchanged.

(3) When Packet B is approved, retire the config/parked.json entry for source/cseries/profile / _compare_profile_sections (about lines 224-242) in the same commit.

(4) Keep the compare_profile_sections prototype in profile.c's prototypes section, as the patch does. Moving it to profile.h is not required, and it would widen the declaration-count blast radius to all 25 includers.
