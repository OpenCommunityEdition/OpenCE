# review source/game/game_engine_king
approve=False

## per_object
[
 {
  "unit": "source/game/game_engine_king: storage/ownership correction (king_source.patch=c2 + symbols_king.patch + optional symbol_ownership_king_OPTIONAL.patch)",
  "approve": true,
  "issues": "Verified independently, and there are no issues. The change is byte-inert: every section except .debug$S is identical to production, and only 23 storage rows move 2->3. The storage is PDB-proven by cachebeta publics. Against the emulated split, object_audit PASSes and pdb_storage shows 0 disagreements. There are 0 UNDEF consumers, the header's only consumer is game_engine_king.c, and every patch passes git apply --check. Land as a zero-credit ownership correction with config.json left NonMatching. If the owner ruling arrives first, substitute OWNER_GATED_king_source_bugcomment.patch for king_source.patch."
 },
 {
  "unit": "source/game/game_engine_king: whole-object completion (config_status_king.patch NonMatching->Matching + semantic_data_matches_king.patch _king_engine entry)",
  "approve": false,
  "issues": "Owner-gated. Admitting the object would certify _find_next_hill's load-bearing reproduced uninitialised result. January +0x52 is 'mov eax,[ebp-4]', and /Od has _RTC_UninitUse(\"next_hill\"). All five defined-result strip variants go residual. Brief sec2/sec7 require owner approval plus a policy-form BUG comment, and neither exists. The data entry itself is otherwise correct: 136/23/4009f5b6...65a7, the '$' defect, and the chain gives +1 object. Reopen: an owner ruling admitting the preserved uninitialised result. Then apply scratch/w/review2_game_engine_king/patches/OWNER_GATED_king_source_bugcomment.patch (instead of king_source.patch) + symbols_king.patch + config_status_king.patch + semantic_data_matches_king.patch (no allow_incomplete_unit) + the optional ownership snapshot."
 }
]

## checks
All checks were run independently in C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review2_game_engine_king/. See LEDGER.md and REVIEW.md there.
(1) git apply --check at HEAD 931ed8dc: each of the 6 worker patches, the combined set and data_gap king.patch all pass. Applied copies are identical (cmp) to the worker's c2/c4 files and to the full config copies.
(2) Own in-place compile harness rgate.py, with the shadow /I placed first. A #error probe proved the patched header is used. Note: gate.py --source copies the file to scratch/_gate_PID.c, so it always sees the production header. Results vs build/split: c2 29/29 EXACT, c4 29/29, production 29/29.
(3) Own csplit run with the patched symbols.json. Of 833 objects, only game_engine_king.obj changes. c2, c4 and production are each 29/29 against the emulated split.
(4) object_audit (split-root copy) against the emulated split: c2 PASS and c4 PASS (39 January symbols, 0 differ). The production control FAILs with 23.
(5) cachebeta_publics: only _king_engine, _render_dynamic_quad and _render_dynamic_quad_initialize are public, and there is no public at 0x43E948. This confirms the 22 callbacks and _king_globals are file-static. pdb_storage against the emulated split: c2 0, c4 0, production 23.
(6) Surplus identity with the explicit candidate object, code plus rdata: 22 surplus sections, all IDENTICAL to a January definer. provider_link: PASS on c2 and c4.
(7) section_info_resolved(_king_engine): target==base in every split/object/symbols.json combination, 136/23/4009f5b6...65a7, matching the entry. objdiff 3.3.1 (sha1 3130e428) mini report shows the storage change is scorer-inert. The 136 unmatched bytes are exactly the '$'-literal defect section.
(8) Emulated progress and admission chain on my artifacts: Halo complete units 363->364, +4,865 code / +876 data / +136 matched, ownership snapshot OK, revoked [], parks 93/0/0, admission 15/0/7/0. This reproduces the worker's numbers.
(9) Every section of the c2 object matches production except .debug$S; only the 23 storage rows go 2->3.
(10) UNDEF scan over build/split (833 objects) and build/base (621): 0 references. git grep: the header's only consumer is game_engine_king.c.
(11) symbols.json: 23 in-place line edits, +368 B, same CRLF count.
(12) fake_match_scan: 0 leads. The .c diff is exactly 23 'static' insertions.
(13) Strip tests: (a) Production without 'static' is also exact, so static buys no bytes; it is required only by PDB storage. (b) Five defined find_next_hill results (=NONE, =0, =hill_id, return NONE, return hill_id) all leave _find_next_hill residual. (c) The unused 'unit' local in player_update is inert. (d) The discarded global_scenario_get() call is a real January call.
(14) January disassembly of _find_next_hill, the /Od 0x5bd630 RTC decode, and a C4700 census from the b1/b2 ninja logs.
(15) Owner-packet variant (c2 plus a policy-form BUG comment and the /Od local name next_hill): 29/29 against both splits, object_audit PASS. git apply --check passes and the patch round-trips.

## issues
BLOCKING (owner-gated class, not disclosed by the worker): object completeness depends on a load-bearing reproduction of an authentic January uninitialised read in _find_next_hill.
- January +0x52: 'mov eax,[ebp-4]; ret'. [ebp-4] is the never-written push-ecx slot, so the no-candidate path returns garbage.
- The later /Od build (0x5bd630) calls _RTC_UninitUse("next_hill"), so the bug is double-attested.
- Production reproduces it with an uninitialised 'long next_hill_id;'. All five defined alternatives lose the function, so the uninitialised local is load-bearing.

Why this blocks self-admission:
- Brief sec2: 'Reproducing an authentic January bug needs target+layout proof, a BUG comment and owner approval - report, don't land'.
- Brief sec7 lists 'reproducing authentic January uninitialised reads' as owner-gated. The lane's dead_camera object was held on the same class.
- No owner ruling for find_next_hill exists in docs. The 20260902 Fable text is only a worker rationale.
- The existing comment is not in the docs/matching_methodology.md 'BUG (preserved for exact matching) ... A corrected build should ...' form.
- A C4700 census finds only this unit and NonMatching rasterizer_xbox_environment_fog. King would become the first Matching object with a load-bearing uninitialised read.

What to land:
- Land the storage correction now as zero credit: king_source.patch (c2) + symbols_king.patch, and optionally symbol_ownership_king_OPTIONAL.patch (the snapshot gate does not require completeness). Keep config.json NonMatching.
- HOLD config_status_king.patch and semantic_data_matches_king.patch until the owner rules. The data entry raises on an incomplete unit.
- If the owner admits the bug, use scratch/w/review2_game_engine_king/patches/OWNER_GATED_king_source_bugcomment.patch INSTEAD of king_source.patch. It is c2 plus a policy-form BUG comment and the /Od name next_hill, 29/29, audit PASS. Then add the status and data entry.

Minor (non-blocking, and not part of this diff):
- The prior 20260915 'static king_globals loses 2 fns' negative does not reproduce at HEAD. It was a tooling artifact of that era.
- The worker's 'official gate.py c2' row did not test the patched header, but my in-place compile did.
- Pre-existing: game_engine_list.c has a consumer-local extern for king_engine.
- Pre-existing: the unknown40/unknown4C variant field names in game_engine.h are placeholders.
