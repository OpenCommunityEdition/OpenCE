# REVIEW source/rasterizer/xbox/rasterizer_xbox_profile

approve: True

amended_patch: (none; approve as submitted: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\patches\09_rasterizer_xbox_profile.patch. Optional companion: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_rasterizer_xbox_profile\09b_optional_rejection_prose.patch)

## checks
Worked only in scratch/w/review_critic_3_rasterizer_xbox_profile/. The task's slug string was garbled, and review_critic_3_profile was already taken by the cseries/profile review, so I used a new slug and claim label. HEAD is 5d3ca708; the task named 7b9de585, which is an ancestor. The hygiene V1R profile packet already landed in 42125a52 (git apply -R --check passes), and patch 09 builds on it. The claim is released and the tree is clean apart from the pre-existing untracked research/.

(1) Name provenance, checked without critic_3's tools:
- The raw Sept-2001 cachebeta.map "Static symbols" section lists exactly 4 statics for rasterizer_xbox_profile.obj: _profile_assert 0x80, _callback_function 0x110, _frame_callback_function 0x90 and _rasterizer_profile_enabled 0x20. They sit directly before _rasterizer_profile_initialize, in the same order and sizes as January's 0x15ECD0, 0x15ED50, 0x15EE60 and 0x15EEF0.
- A second, independent Sept link (retail cache.map, /Gr) has the same 4 statics in the same order: @profile_assert@12, _callback_function, _frame_callback_function and @rasterizer_profile_enabled@0. Those decorations match the source parameter lists.
- The Aug-2001 map has _profile_assert and _callback_function. The January atlas (4cc87b45) has the first 3 names at their addresses, tier exact.
- septcmp.py compared January split bytes with Sept xbe bytes, masking only January's relocation fields. All 4 functions have 0 differing bytes. Every REL32 resolves to the same callee (_display_assert, _system_exit, _error, _QueryPerformanceCounter@4). The DIR32 .bss offsets keep identical deltas, and _rasterizer_debug_options +2/+76 resolves correctly in the Sept map.
- No literal in January's cachebeta.exe contradicts any of the names. HCEX.pdb has none of the 8 names; DIA2Dump was sanity-checked with create_weapon.
- The current names appear in no atlas build, so they were invented. The new names collide with nothing in tracked files, in symbols.json or in the other 20 patches.

(2) Byte-inertness:
- The patch is a pure token rename: reverse-mapping cand.c reproduces production exactly. The symbols.json diff is exactly the name values on lines 5877-5880; order and "static": true are untouched, and the JSON is valid.
- Own csplit (build/tools/csplit.exe): split_ctl equals build/split in 833 of 833 objects. split_09 differs only in rasterizer_xbox_profile.obj, and after mapping names its 59 sections and 145 symbols are identical, storage 3 kept.
- Gate: production vs build/split is 16/0/0; the candidate vs split_09 is 16/0/0, with the same rows once names are mapped. The candidate against the unregenerated build/split gives 6 exact, 6 reloc-identity and 4 unwritten, so the csplit regen must land in the same step.
- objcmp: prod.obj equals build/base (76 sections, 167 symbols). Mapped prod vs cand shows 0 section and 0 symbol differences, data included.

(3) Admission battery, run with a with_split redirect: object_audit PASS on both (67 January symbols, 0 differ); pdb_storage 0/0; surplus_identity 0 COMDATs; provider_link PASS in both orders. The outputs are identical once names are mapped. objdiff 3.3.1 (sha1 3130e428) gives identical measures on both sides: code 2511/2511, 16/16 functions, data 2102/2234. That means zero credit.

(4) git apply --check: 09 alone passes at HEAD; the series 01,03..21 passes; the 09 symbols.json hunk applies after 01,02,03..08 (GNU patch dry-run).

(5) House rules and held classes: only names change, so the strip test and /Od declaration-order checks do not apply. Authentic names replace invented ones. The rejection's "new private names need owner policy" clause does not apply, because these names are first-party, not invented. The held NONE-branch varargs defect, the volatile qualifiers and the invented aggregates are untouched, and there is no parked.json or semantic entry for this unit.

## issues
No blocking issues. I approve patch 09 as submitted. It earns zero credit and no object completes. The integrator must regenerate csplit (ninja, where symbols.json is a csplit input) in the same step; without that, the candidate against the old split scores only 6 EXACT (6 reloc-identity, 4 unwritten).

Minor, optional: after the rename, the active rejection in config/object_admission_rejections.json still names the function in its prose ("The rasterizer_profile_check NONE branch"). The entry is keyed on _rasterizer_profile_globals, and tools/object_admission_policy.py validates only fields and duplicates, so this is cosmetic. An optional companion patch updates the prose: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_rasterizer_xbox_profile\09b_optional_rejection_prose.patch. It passes git apply --check alone and together with 09, and the policy loader still accepts the file.

Follow-up outside this packet: January emits the 4 statics before rasterizer_profile_initialize, and by the definition-order law that means January defined them first. Production defines them in a bottom "private code" block, so its emission order differs. In lab/lab_order.c I moved the private block above the public code in January's order: the gate is 16/16 EXACT against split_09, object_audit passes, and the emission order equals January's. The later /Od build was restructured, so it attests neither layout. This is a separate source-layout question for the owner or house rules.

Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_rasterizer_xbox_profile\ (LEDGER.md, MANIFEST.md, septcmp.py, objcmp.py, gate_prod.txt, gate_cand.txt, battery/, objdiff/, lab/).
