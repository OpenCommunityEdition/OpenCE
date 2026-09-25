# REVIEW source/main/main

approve: True

amended_patch: None

## checks
All outputs are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic3_main\ (LEDGER.md, MANIFEST.md, *.txt, *.py). Tree 5d3ca708 was clean before and after. No tracked edits. Claim taken and released.
1. Patch shape: `git apply --check` of patches/15_main.patch at the repo root returns rc 0. I applied it to LF index-form copies. The diff against HEAD is exactly:
   - symbols.json line 4107: name only, edited in place, "static": true kept, no reserialisation.
   - main.c: listing-comment row, prototype (line 670), qsort argument (830) and definition (1552).
   - The function body is untouched.
2. Reference sweep: `git grep` finds the old name only in:
   - main.c and config/symbols.json;
   - generated build/report.json and build/semantic_report.json (rebuilt by ninja);
   - historical research/ notes.
   It is absent from parked.json, semantic_matches.json, semantic_data_matches.json, the admission rejections, headers, tools and docs. It is a static, so there is no header prototype.
3. First-party name evidence, checked myself:
   - Three raw linker maps name `_sort_desired_local_player_controllers` in main.obj: Aug-2001 cachebeta.map, Sept-2001 cachebeta.map and Sept-2001 retail cache.map. In all three it is in the Static symbols section, between main_get_difficulty and create_local_players, size 0x40. January has the same slot: 0xef8a0, size 0x40.
   - Body identity (bodyid.py): January's 64 bytes are byte-IDENTICAL to Sept cachebeta.xbe .text+0xed7c0. Retail cache.exe has the same logic without a frame pointer.
   - Caller (callers.py): January's only reference is a dir32 relocation at create_local_players+0x157. The Sept xbe has the comparator's address as an imm32 at the same offset, create_local_players+0x157.
4. Where the current name comes from:
   - The atlas rows for January (4cc87b45 and b4963176) carry `?sort_controllers_ascending@@YAHPBX0@Z` at tier "exact", i.e. propagated, not read from a map.
   - atlas index.csv: the January builds have n_map=0. The only Halo CE builds with a map-tier name are the two Sept-2001 ones, and both give the new name.
   - The mangled name is C++ (`int __cdecl (void const *, void const *)`) from later C++ builds. Reach TU1 carries it at tier "exact" from blamlib game.obj.
   - The atlas METHODOLOGY settles a class that carries two map names by majority vote, which favours the many later-game builds.
   - January main.obj is C, and every symbol in it is C-decorated.
   - The name came into the tree in bulk owner commit 9f74e993.
5. Other sources:
   - HCEX.pdb: no `*sort*controller*`, `*controllers_ascending*` or Halo `*ascending*` symbol, so it is neutral.
   - January cachebeta.exe: neither name appears. The create_local_players assert text `desired_controllers[i]` fits the Sept vocabulary.
   - cachebeta publics: the function is not public, so it was static; the patch keeps it static.
6. Emulated split: I ran build/tools/csplit.exe into my slug.
   - The control split equals build/split for all 833 objects.
   - The candidate split differs only in main.obj, and objcmp shows that object IDENTICAL apart from the rename (storage 3 on both sides, old name gone).
7. Gate on every January function:
   - Production against build/split: 92 exact, 2 residual, 1 unwritten.
   - Candidate against the candidate split (with_split redirect): the same rows apart from the rename. `_sort_desired_local_player_controllers` is EXACT at 64 bytes with 0 relocations.
8. objcmp of the compiled production and candidate objects: IDENTICAL apart from the rename (every non-.debug$S section's bytes and relocations, plus the symbol table). My production compile also matches build/base, except for the .debug$S object path.
9. Data (tinfo --data): the two splits are identical. Production and candidate differ only in the .debug$S gate path.
10. Section-4 battery, identical for production and candidate apart from the rename:
   - object_audit FAIL(6) on both, all pre-existing: the chucky string and main_crash are missing, plus 2 residual DIFF rows. The renamed row is ok, 64/64.
   - pdb_storage: 1 pre-existing disagreement on both.
   - surplus_identity: 2 surplus helpers, 0 not identical.
   - provider_link: PASS in both orders.
11. objdiff 3.3.1 (build/tools/objdiff-cli.exe, sha1 3130e428), mini project with production and candidate: both score 11128/13117 code, 92/95 functions, 2659/4507 data, 99.74965%. The comparator scores 100% under either name, so the change earns zero credit.
12. Strip test and house rules: nothing is added (no casts, parentheses, declarations or scopes), so the /Od declaration-order check does not apply. The pre-existing qsort cast is unchanged. The prototype stays in the object's own .c. One parameter per line.

## issues
No blocking issues; the patch is approved as-is with no amendment. It is a zero-credit, name-only change.

Two notes for the integrator:
1. This undoes a name that owner commit 9f74e993 ("Import 909 authenticated January and HCEA symbol names") imported from the atlas. That name is a C++-mangled name carried over from later C++ builds by majority vote. January has no linker map, and the only Halo CE maps (the two Sept-2001 ones, plus the Aug-2001 map) all give `_sort_desired_local_player_controllers`, with a byte-identical body and the same caller offset. Say this when landing.
2. Bytes cannot rule out a rename between Sept-2001 and January 2002. The Sept name is the closest C-language first-party record, and the other name's mangled signature (`int` return, `void const *` parameters) does not match the C prototype `long (short const *, short const *)`. This is the same evidence level as the lane's accepted Sept-map renames (xbox_texture_cache, units, hardware_geometry).

Out-of-scope follow-up: main.c's symbol-listing comment still has 26 stale `_code_` placeholder rows, e.g. `_code_000ef8e0` for create_local_players, which symbols.json already names.
