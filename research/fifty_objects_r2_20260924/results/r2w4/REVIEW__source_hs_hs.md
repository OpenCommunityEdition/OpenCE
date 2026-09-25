# REVIEW source/hs/hs

approve: True

amended_patch: None

## checks
I re-derived everything myself, working only in scratch/w/review_critic3_hs/ at 5d3ca708. The tree was clean at the start and end, and I released the claim.

(1) Name evidence
- Three first-party maps give the same names in the same order: Aug-15-2001 cachebeta.map, Sep-25-2001 cachebeta.map and the Sep-25-2001 retail cache.map (as @radiosity_hack_*_evaluate@12). The names are radiosity_hack_start/save/find_point_evaluate.
- I parsed the Aug and Sept XBE images myself (xbe_defs.py). The .rdata hs_function_definition structs for "radiosity_start", "radiosity_save" and "radiosity_debug_point" point at exactly those three functions (Sept 0xc0b20/40/60, Aug 0xb4590/b0/d0). As a check on the struct decode, the parse slot resolves to _hs_macro_function_parse.
- In January cachebeta.exe (PE parse), the same three structs point at 0x4b14a0/c0/e0. Each address has exactly one absolute reference in the image (its definition slot), and nothing calls them.
- Bodies are the same 20-byte hs_return(thread,NULL) stub in Jan, Sept and Aug. I resolved the call targets to _hs_return in both builds.
- 444 of the 447 Sept hs.obj function names are already used in the January split. The only 3 missing are these. After the patch it is 447/447, and the one January-only function is _scripted_foley_predict_evaluate.
- Nothing contradicts the names. cachebeta.pdb publics have no evaluator names (these are file statics). January strings contain only the command names. The atlas has only the Sept retail rows. HCEX.pdb has neither name (DIA2Dump lookup, sanity-checked on create_weapon).

(2) References
- git grep over all tracked files finds the old names only at the 9 hs.c sites and the 3 symbols.json rows the patch edits. None appear in parked.json, semantic_*.json, rejections, docs or tools/tests.
- The prototype block is still alphabetical. The three listing-comment placeholders were renamed.
- No other patch has a symbols.json hunk near these rows.
- critic_2's hs_compile packet and patch 13 apply in both orders with identical results.

(3) Measurements
- git apply --check passes.
- csplit: a control split equals build/split for all 833 objects. The patched split changes only hs.obj, and it is identical to the control apart from the 3 renames. Without the rename map the compare reports differences, so it does detect changes.
- gate --all: production 447/1 against build/split, candidate 447/1 against the emulated split, same rows. Negative control: 444/1/3 unwritten.
- objcmp, production object vs candidate object: 1410 sections and 3753 symbols identical apart from the renames.
- The same holds with critic_2's hs_library_internal_compile.h put first on the include path. An #error probe proves the compiler used that copy.
- object_audit: FAIL(1) on both sides with the same text. The one failure is the pre-existing _real_random_range_evaluate residual. 1830 January symbols, 0 differ.
- pdb_storage: 0 disagreements. surplus_identity: 0 surplus code COMDATs. provider_link (both orders): PASS, same output on both sides.
- objdiff 3.3.1 (sha1 3130e428): production, candidate and build/base give identical measures.

(4) House rules: the patch only renames; there are no new declarations, casts, macros or scopes. The strip test and /Od declaration order therefore don't apply. This is not a held class.

## issues
No blocking issues; approve as submitted, no amendment needed. Zero credit: the patch doesn't change any bytes, and hs.obj stays 447/1.

Notes for the integrator:
(a) Land it with a csplit regen. The symbols.json edits are in-place name changes on rows 2908-2910 and keep "static": true.
(b) It does not depend on critic_2's hs_compile packet. They apply in either order, and the combined build is identical apart from the renames.
(c) The other `_code_` lines in hs.c's listing comment were already out of date before this patch (they date from the initial commit). They are cosmetic and outside this packet's scope.
(d) Not run here: full ninja, pytest, board-wide stable_verdicts. Board-wide risk is nil: hs.c is compiled only into hs.obj, no header changes, and only hs.obj changes in the split.

Evidence and scripts are in scratch/w/review_critic3_hs/, with details in LEDGER.md and MANIFEST.md.
