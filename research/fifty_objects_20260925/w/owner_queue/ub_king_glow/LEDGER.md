# LEDGER - owner_queue/ub_king_glow (wave 5 owner-queue verification)
Tree: HEAD eb33baec (= cdc8ebd3 + ledger-only doc commit; no source/config delta). git status clean at start.
Items: (1) game_engine_king `_find_next_hill` uninitialised `next_hill` return; (2) glow `_get_particle_world_position` two uninitialised array reads.
NOTHING LANDS. Work only under scratch/w/owner_queue/ub_king_glow/.

## S0 read-in
- Read WORKER_BRIEF (s0-s10), held-items table (docs/object_matching_logs/claude_fifty_objects_20260925.md lines 300-330),
  w/king/LEDGER.md, w/review2_game_engine_king/{LEDGER,REVIEW}.md, w/glow/LEDGER.md, w/review2_glow/REVIEW.md, results/wave2 king + fn2_glow.
- History since e9e62b78: game_engine_king.c touched only by f7cd2e72 (game_variant renames); game_engine_king.h, glow.c/.h untouched.
  King storage correction (king_source.patch) was NOT landed (22 callbacks + king_globals still external in the tree).

## S1 old patches vs tip (git apply --check, read-only)
- review2 OWNER_GATED_king_source_bugcomment.patch: applies clean.
- king/{config_status_king, king_source, king_source_struct_move_ALT, symbol_ownership_king_OPTIONAL, symbols_king}.patch: apply clean.
- king/semantic_data_matches_king.patch: FAILS (config/semantic_data_matches.json:470 context moved) -> regenerate.
- glow/production.patch: FAILS (config/config.json:536 context moved) -> regenerate.

## K1 king rebase onto cdc8ebd3
- Copies of tip files -> king/tip/ (hash-object == HEAD blobs; tree clean). king/new/ = tip + review2 OWNER_GATED_king_source_bugcomment.patch
  + king/symbols_king.patch + king/config_status_king.patch + king/symbol_ownership_king_OPTIONAL.patch (git apply --directory into slug copies).
  Result .c == review2 strip/owner_bug_comment.c + exactly the two f7cd2e72 game_variant renames (lines 515, 816). symbols.json:
  23 in-place `, "static": true` edits (_king_globals now at line 22993; the old patch applied with a 1-line offset).
- emu_split.py (csplit -i cachebeta.exe with slug config copy): emu/tip == build/split (833/833 byte-identical) -> build/split is current;
  emu/king (patched symbols.json/config.json) differs from build/split ONLY in source/game/game_engine_king.obj.
- sgate.py (shadow /I first; #error probe in king/probe proves the patched header wins): new vs build/split 29/29 EXACT
  (22 callbacks storage 2/3 as expected before the split change); new vs emu/king 29/29 EXACT, all storage equal.
  Only warning: C4700 next_hill (the preserved read). point_from_line3d guard passed.
- K2 object_audit_s.py (= scratch/tools/object_audit.py + SPLIT_ROOT env + fixed ROOT): new.obj vs emu/king PASS (39 January symbols,
  0 differ; every January section ok incl .data _king_engine 136/136, .bss _king_globals 560/560). Controls: new vs build/split FAIL(23)
  (storage only, split not yet static); production build/base obj vs emu/king FAIL(23); production vs build/split PASS.
- K3 pdb_storage_s.py (reviewer's SPLIT_ROOT variant): new.obj vs emu/king 0 disagreements (39 symbols); production vs emu 23; official
  pdb_storage on the tree: 23 (the 22 callbacks + _king_globals are absent from cachebeta publics).
- K4 surplus_id_s.py (explicit obj; code + rdata): 22 candidate-only external sections, 0 not identical/unprovided (10 real_math.h code
  COMDATs vs action_* providers, 12 literals). provider_link.py new.obj: 22 rows, SELECTED-PROVIDER LINK: PASS (both orders).
  nodup_census for the 10 helpers: 0 NODUP definers in build/base. objcmp production vs new.obj: 59 owner-keyed sections, 0 diffs;
  only the 23 storage rows 2->3.
- K5 January bytes re-read (fdis.py build/split king _find_next_hill): +0x03 push ecx reserves [ebp-4]; +0x52 `mov eax,[ebp-4]` on
  the loop-exhausted path; nothing writes [ebp-4] -> BUG comment offsets verified. /Od 0x5bd630 (review2 od_find_next_hill.txt):
  RTC flag [ebp-0x15] set only on the found path, `_RTC_UninitUse` with string @0x5bd6f5 = "next_hill", result slot [ebp-0xc]
  written only inside the found branch followed by `jmp` to a SINGLE return (single-exit + break topology).
- K6 production at tip already carries the read: gate.py (tree) 29/29; build/report.json king 4865/4865 code, _find_next_hill 100%.
  So the ruling decides object admission (and the BUG-comment wording), not a new function credit.
- K7 INFORMATIONAL probe (not in the packet; not an invented form - /Od-attested topology): probe_se = the /Od single-exit shape
  (`if (...) { next_hill = king_engine_hills[hill_index]; break; }` then one `return next_hill;`) -> 29/29 EXACT vs emu/king, and NO
  C4700 (next_hill is now assigned on the found path; C4701 is /W4 only). probe_se_init (`long next_hill = hill_id;`) -> 28/29
  `_find_next_hill [sha]` -> the uninitialised result stays load-bearing in the /Od shape too. Offered to the owner as an option.
- K8 data entry rebased (mk_king_sdm.py: text append to the tip copy; old patch failed at :470 because b9a8d587 appended entries).
  Reason text = worker's, plus the owner-admitted BUG disclosure clause; "after the Layer 2 provider repair" dropped (history, not
  evidence). section_info_resolved(_king_engine) target==base in all 8 combos {tip, edited symbols.json} x {build/split, emu/king}
  x {production obj, new.obj}: 136 / 23 / 4009f5b6117e9f00..., flags c0400040 both sides.
- K9 emu_chain.py (mini objdiff 3.3.1 report; control mini row == production row True; fresh semantic audit of the production
  report == build/semantic_report.json summary True): BASE Halo complete_units 379, complete_code 890,841, complete_data 714,870,
  matched_data 2,576,122; EMU (emu/king target, new.obj, edited symbols/sdm/ownership, status Matching) Halo complete_units 380
  (+1), complete_code 895,706 (+4,865), complete_data 715,746 (+876), matched_data 2,576,258 (+136), matched_code/functions
  unchanged (king functions were already 29/29); data credit 'game_engine_king:_king_engine (+136)' WITHOUT allow_incomplete_unit;
  ownership snapshot king .bss (3 symbols) validates; revoked []; parks 86/0/0; admission 11/0/2/0, 0 contradicted.
- K10 fake_match_scan on new .c/.h and the ALT .c: 0 leads (production 0). UNDEF scan for the 23 names: 0 in build/base (621) and
  build/split (833). git grep: game_engine_king.h included only by game_engine_king.c; no other source names king_globals.
- K11 patches regenerated as canonical git patches (mkpatch.py; LF, full blob index = HEAD blobs): 01 source (owner BUG form),
  02 symbols, 03 config status, 04 semantic data entry, 05 ownership snapshot (OPTIONAL); alt/01A (/Od single-exit ALT);
  if_declined/king_source_storage_only.patch (= worker c2, storage only, production comment). git apply --check: each OK and
  01-05 combined OK; --whitespace=error-all clean; trial apply of 01-05 onto tip copies == king/new byte-for-byte (CRLF), JSON parses.
- K12 storage_only.obj and probe_se.obj vs new.obj (objcmp): 59 sections 0 diffs, 0 symbol-row diffs; probe_se object_audit vs
  emu/king PASS 39/0; storage_only 29/29 vs emu/king (C4700 next_hill_id, as production).

## G1 glow rebase onto cdc8ebd3
- glow.c/.h untouched since e9e62b78. Old production.patch failed only on config/config.json (b9a8d587 flipped light_volumes.c two lines
  above glow.c). Split the old patch: glow.c part + parked.json part apply clean to tip copies (glow/new); rebased glow.c ==
  scratch/w/glow/cand_final.c byte-for-byte (CRLF). parked.json: the glow park entry removed (now lines 1105-1115; batches removed
  other parks); config.json glow.c NonMatching -> Matching by one-line text edit. symbols.json unchanged (_get_particle_world_position
  already "static": true at line 5061).
- gate.py --source glow/new ... --all --forbid-emitted-symbol _point_from_line3d: 25/25 EXACT (guard passed); _get_particle_world_position
  EXACT 1744. Tip control: 24/1, residual [size 2528!=1744, relocs 55!=43, sha]. sgate (default /W1): no warnings. /W3 /Zs: 0 TU-local
  warnings in candidate and tip (12 header warnings, same set).
- object_audit (official, build/split; no symbols change): PASS, 36 January symbols 0 differ, every January section ok. Candidate-only
  surplus list identical to production's (tip obj). objcmp tip.obj vs new.obj: 56 sections, 1 diff (_get_particle_world_position only),
  0 symbol-row diffs.
- surplus_id_s: 20 candidate-only external sections, 0 not identical/unprovided (10 code COMDATs incl. _real_local_random_range vs
  bored_camera). provider_link new.obj: 20 rows, SELECTED-PROVIDER LINK: PASS. nodup_census: 0 NODUP definers for the 10 helpers.
  pdb_storage (tree and candidate): 36 split symbols, 0 disagreements. No glow entry in object_admission_rejections / semantic_* /
  symbol_ownership. Data: .rdata 392/392 (100%), no .data/.bss -> no data entry needed.
- January bytes re-read (fdis.py -> glow/jan_gpwp.txt): frame sub esp,0xc4; traversal_left_vectors [ebp-0xc4..-0x9c] written only by
  the default-loop indexed stores (+0x21b/+0x23e/+0x259), read only by the four leas feeding spline 3 (+0x637..+0x64c); 3-marker case 1
  (+0x2ea..+0x3a3, fld marker_time_index[1] kept on x87) jumps to +0x5c3 fadd [ebp-0x10] / +0x5c6 fstp [ebp-8] and [ebp-0xc]
  (traversal_time_indices[1]) is read at +0x5d2 without a write on that path. BUG comment offsets verified.
- Strip tests at tip (lab only, mk_glow_strip.py): sA knot repaired only -> 24/1 [sha]; sB `traversal_left_vectors[4] = { 0 }` only ->
  24/1 [size 1776!=1744, sha] -> each defect is independently load-bearing. sC `index < 4` -> 25/25 (NUMBEROF byte-inert; reviewer
  nit). review2 s1_nocasts (all point/vector view casts removed) -> 25/25 but 4+ C4133 incompatible-type warnings -> the casts are
  type-correctness, byte-inert, not decoration.
- emu_chain glow (build/split target, new.obj, parked minus glow entry, status Matching): Halo complete_units 379 -> 380, complete_code
  +6,987, complete_data +392, matched_code +1,729 (objdiff size), matched_functions +1, parks 86 -> 85 (0 stale/invalid), revoked [],
  admission 11/0/2/0 0 contradicted. Control mini row == production row True; fresh semantic audit == build/semantic_report.json.
- patches regenerated (mkpatch.py): 01_glow_source_owner_bug, 02_glow_parked, 03_glow_config_status; git apply --check each + combined
  OK, --whitespace=error-all clean, trial apply onto tip copies == glow/new byte-for-byte, JSON parses.

## B1 both items together
- git apply --check of king 01-05 + glow 01-03 in one invocation: OK (both 03 patches touch config.json at different lines; either order OK).
- emu_chain stacked (BASE_TAG=king, then glow): Halo complete_units 379 -> 381, complete_code 890,841 -> 902,693 (+11,852), complete_data
  714,870 -> 716,138 (+1,268), matched_code +1,729, matched_data +136, matched_functions +1, parks 85/0/0, revoked [], admission
  11/0/2/0, 2 ownership snapshots validate, 38 data credits.
- G2 emu_chain glow_01_only (01 applied, park kept, status NonMatching): require_valid_parked_functions raises "parked-function
  evidence is stale or invalid: source/objects/widgets/glow:_get_particle_world_position" -> 01 and 02 must land together.
- G3 /Od re-check (scratch/w/glow/od_gpwp.txt): RTC descriptor 0x799c44 names traversal_points [ebp-0x38] 48, traversal_up_vectors
  [ebp-0x70] 48, traversal_left_vectors [ebp-0xa8] 48, traversal_time_indices [ebp-0xc0] 16; [ebp-0xa8] is written only via the
  default-loop cross_product3d (fn 0x43e710) out-pointer lea at 0x799887 and read by the spline-3 leas 0x799aaf..0x799ade;
  3-marker case 1 stores mti[1] to time_indices[2] (0x799533) and the midpoint again to time_indices[2] (0x799639) -> [1] never set.
- K13 king RULING.md / MEASURED.md / patches/APPLY_ORDER.txt written. Behaviour note: the no-candidate path needs every hill id ==
  hill_id (single distinct hill, moving hills on); caller king_engine_update stores the result in king_globals.hill_id.
- CORRECTION to G1: traversal_left_vectors spans [ebp-0xc4, ebp-0x94) (4 x 12 B at -0xc4/-0xb8/-0xac/-0xa0; spline-3 leas), not
  "..-0x9c"; the BUG comment's "[ebp-0xc4..-0x94]" is end-exclusive and correct.
- G4 glow RULING.md / MEASURED.md / patches/APPLY_ORDER.txt written.
- R1 precedents added to both RULING.md: ai_script.c (Matching, owner ruling 2026-09-20; missing vsprintf arg reads the unset saved-EDI
  slot, source/ai/ai_script.c:2765) = closest precedent; _actor_emotion_update 8e4309dd (OOB + layout assertions actor_perception.c:856-867);
  NonMatching ai_communication.c:2527 cause_point uninit BUG comment (7a1d0d91).

## Deliverables (final)
- king/: patches/{01..05, alt/01A, if_declined/}, patches/APPLY_ORDER.txt, MEASURED.md, RULING.md, new/ (edited full copies incl.
  new/config/symbols.json, semantic_data_matches.json, symbol_ownership.json, config.json), obj/, emu/king split, chain/king.
- glow/: patches/{01..03}, patches/APPLY_ORDER.txt, MEASURED.md, RULING.md, new/ (edited full copies), obj/, strip/, chain/glow.
- Combined: chain/both (Halo 379 -> 381). Tree: git status clean at end (HEAD eb33baec); no tracked file edited; only slug paths written
  (plus gate.py's own scratch/_gate_<pid>.* temporaries).
