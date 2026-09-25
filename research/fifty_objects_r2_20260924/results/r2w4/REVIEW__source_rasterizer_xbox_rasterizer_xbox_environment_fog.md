# REVIEW source/rasterizer/xbox/rasterizer_xbox_environment_fog

approve: True

amended_patch: None

## checks
All work is under scratch/w/review_critic3_rasterizer_xbox_environment_fog/. Tree at 5d3ca708 is clean: no tracked edits, no ninja/configure/git mutation. Claim taken and released.

(1) Patch checks
- `git apply --check` passes on critic_3/patches/19, also with -v and --whitespace=error.
- The GNU-patch copy in patchroot/ differs from production (ignoring CR) in exactly 7 source lines: the symbol-listing comment `_code_00155860`, the prototype, the definition and 4 call sites.
- It also changes symbols.json:5786, name only. `"static": true` and the line order are unchanged.

(2) Reference census
- `git grep` over all tracked files for the old name, the new name and `code_00155860`.
- Outside the historical docs/ and the frozen research/fifty_objects_20260925 snapshots, the only references are symbols.json:5786 and the .c file, and the patch updates all of them.
- Nothing else names it: no header, no parked.json/semantic_matches/semantic_data_matches/object_admission_rejections/splits/ownership/relocs entry, and nothing in tools or tests.
- The .bss array `fog_screen_active` is a different identifier and is untouched.

(3) Emulated split
- `build/tools/csplit.exe` run on a control config and on the patched config.
- split_ctl equals build/split for all 833 objects.
- In split_P only rasterizer_xbox_environment_fog.obj differs. My own name-resolved COFF compare (rawcmp.py) finds identical bytes and relocations in all 58 sections. The only difference is symbol #67's name; value, section 21, type 0x20 and storage 3 are the same on both sides.

(4) Gate
- Production vs build/split and candidate vs split_P (with_split redirect): 26 EXACT / 1 residual on both sides, with identical rows.
- The renamed function is EXACT at 512 bytes on both sides.
- The residual `__rasterizer_environment_fog_screen_begin` (size 4000 vs 3904, relocs 232 vs 235) is unchanged.

(5) Object compare
- prod.obj vs cand.obj with the rename mapped: all 116 sections and all symbols are identical except .debug$S. Its 5 differing bytes are the gate PID in the object path, and .debug$S holds no function names.
- build/base vs the production gate compile: identical apart from .debug$S.

(6) objdiff-cli 3.3.1 (sha1 3130e428), mini project
- Production, candidate, and build/split vs build/base all give 6273/10174 code, 26/27 functions, 2018/2018 data, 91.724396%.

(7) Section-4 battery, production vs candidate
- object_audit: FAIL(8) on both sides (1 residual section plus 7 D3D thunks with storage 2 vs 3). The only difference is the rename.
- pdb_storage: the same 7 disagreements on both sides. The renamed function is not in the cachebeta publics, so static is correct.
- surplus_identity: 13 surplus COMDATs, 1 not identical (`_real_local_random`), on both sides.
- provider_link, both link orders: FAIL(1) on `_real_local_random` vs effects on both sides. This was already there before the patch.

(8) First-party name evidence, checked independently
- Sept-2001 cachebeta.map static table: `_rasterizer_environment_fog_screen_is_active` sits in the same slot as January's function, between `_local_random_boolean` and `_rasterizer_environment_fog_screen_wind_update` (both named identically in January). Its size to the next symbol is 0x200, the same as January.
- The Aug-2001 map uses the same name. The Sept retail cache.map has `@rasterizer_environment_fog_screen_is_active@0`.
- Raw bytes: I read Sept cachebeta.xbe as data only, never executed it (sept_body.py). There are 0 byte differences over 0x200 bytes outside relocation fields.
- All 42 January relocations resolve at the same offsets to the same Sept symbols (global_window_parameters+N, rasterizer_globals+N, rasterizer_debug_options+N, the same `__real@` constants and assert literals, display_assert, system_exit). The .bss static offsets have the same relative layout.
- Callers (callers.py): January has 4 callers (screen_begin +0x78, screen_draw +0x2c, screen_end +0x31, model_begin +0x22). A Sept xbe E8 scan finds the same 4 at the same offsets, and Aug has the same 4.
- January-hash atlas 4cc87b45: 0x555860 carries this name at the exact tier, and the other 7 fog records match the current symbols.json names at offset minus 0x400000.
- January exe strings contradict nothing.
- HCEX.pdb (DIA2Dump, tool sanity-checked) has neither name. It only has the rasterizer.c public stubs plus initialize/model_begin/model_end, so it contradicts nothing either.

(9) House rules and held classes
- This is a pure identifier rename, so the strip test and the /Od declaration-order check do not apply.
- Rule 7 is satisfied: an authentic first-party name replaces the old one, which the 20260909 ledger itself described as "reconstructed rather than PDB-recovered".
- The unit is not in a held class and has no park or rejection entry.
- No other round-2 claim covers the unit. The canary_frame_statistics shadow copies are identical to production, and no other patch has a symbols.json hunk near line 5786.

Evidence files: LEDGER.md, MANIFEST.md, gate_prod.txt, gate_cand.txt, battery_prod.txt, battery_cand.txt, sept_body_is_active.txt, callers.txt, objdiff/report.json.

## issues
No blocking issues and no amendment needed. The patch is a byte-inert rename with zero credit, and its authentic name is proven by the Sept, Aug and retail maps, raw-byte identity with the Sept xbe including relocation identity, identical callers, and the January atlas.

Notes for the integrator (not defects):
(a) Name-keyed reports (report.json, gate text) will show `_rasterizer_environment_fog_screen_active` turning into `_rasterizer_environment_fog_screen_is_active`. The gate_*/stable.json snapshots key it by section index (`::section:21`), so stable_verdicts still sees the same exact section.
(b) Historical docs/object_matching_logs ledgers and the frozen research/fifty_objects_20260925 snapshots keep the old spelling. That is intended, since they are records.
(c) One lab file from another agent, scratch/w/triage_rasterizer_b/lab/lab_fog_rlr.c, still uses the old name. It is a lab file, not a landing candidate.
(d) The failures that remain in this object were all there before the patch: the screen_begin residual, the 7 D3D-thunk storage mismatches, and `_real_local_random`'s surplus and provider-link failure.
