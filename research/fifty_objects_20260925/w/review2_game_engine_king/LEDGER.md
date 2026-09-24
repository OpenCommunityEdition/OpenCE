# LEDGER - review2_game_engine_king (adversarial review of king OBJECT_COMPLETE_CANDIDATE)
Base 931ed8dc. Reviewer: independent re-run of gates/audit/storage/surplus/data/patch-apply/house rules.

## R1 patch application (HEAD 931ed8dc, tree clean of tracked edits)
- git apply --check: king_source, ALT, symbols_king, config_status_king, semantic_data_matches_king, symbol_ownership_king_OPTIONAL each OK; combined (rec + 4 config) OK; data_gap king.patch OK.
- Applied via `git apply --directory=<slug>/rec|alt` to COPIES: rec .c/.h == worker c2 (cmp), alt .c/.h == worker c4; patched symbols/config/semantic_data_matches/symbol_ownership == worker full copies (cmp).

## R2 compiles (own harness rgate.py: in-place, /I<shadow> FIRST; #error probe proved the shadow header wins)
- rec (c2 + patched header) vs build/split: 29/29 EXACT; all 27 king fns storage 3, render_dynamic_quad(+_initialize) storage 2. Only warning C4700 next_hill_id (pre-existing).
- alt (c4) 29/29; production 29/29.
- NOTE gate.py --source copies the .c to scratch/_gate_PID.c so it ALWAYS sees the production header (extern+static redeclaration); the worker's "official gate.py" row therefore did NOT test the patched header. My rgate did.
## R3 emulated split (csplit with patched symbols.json/config.json -> <slug>/split)
- 833 objects; byte-compare vs build/split: ONLY source/game/game_engine_king.obj differs.
- rec / alt / production vs emulated split: 29/29 EXACT each.
- Recorded negative opus5_250k_house_clean_checkpoint_20260915 sec5 ("king_globals static ... loses _find_next_hill and _king_engine_initialize_for_new_map") does NOT reproduce at HEAD: relocations resolve to ('defined-noncode','.bss','_king_globals',428/432) identically in January (build + emulated split), rec and prod. Declarations were the same in 0915 (git history) -> the 0915 loss was a tooling/measurement artifact of that era or a different split; current tools say inert.
## R4 object audit / storage / surplus (own runs)
- object_audit (official) rec vs build/split: FAIL(23) storage 2/3 as expected (split not yet static).
- object_audit_r (SPLIT_ROOT copy of official) vs emulated split: rec PASS (39 Jan symbols 0 differ, all Jan sections ok incl .data _king_engine 136/136 and .bss _king_globals 560/560), alt PASS, prod FAIL(23) control.
- cachebeta_publics: only _king_engine (0x2DE488), _render_dynamic_quad (0xA0A90), _render_dynamic_quad_initialize (0xA0A00) public in the A0030..A1460 / 43E948 ranges; no public at 43E948 (_king_globals). => 22 fns + _king_globals file-static CONFIRMED.
- pdb_storage (production): 23 disagreements (split 2 ours 2). pdb_storage_r vs emulated split: rec 0, alt 0, prod 23 (control).
- surplus_id_r (explicit candidate obj; code AND rdata): 22 candidate-only external sections, all IDENTICAL to a January definer (distance3d vs action_vehicle, 'king' literal vs game_engine, '!"unreachable"' vs actor_moving, ...). 0 unprovided.
- provider_link rec.obj: 22 rows PASS, SELECTED-PROVIDER LINK: PASS; alt.obj PASS.
## R5 data / scorer / chain
- section_info_resolved(_king_engine): target==base True for {build/split, emulated split} x {prod, rec, alt, build/base} under BOTH config and patched symbols.json; 136 / 23 / 4009f5b6...65a7 == entry measurements.
- Only raw reloc difference: +0 'king' literal UNDEF (Jan) vs defined .rdata COMDAT (ours); section_infos_equal True. build/report.json king: data 740/876, .data 136 @ 98.24561% -> unmatched 136 == exactly the entry.
- objdiff-cli 3.3.1 (sha1 3130e428) mini report: emulated split + rec gives identical king rows to production (storage inert to the scorer); with metadata.complete -> complete_code 4865 / complete_data 876 / complete_units 1.
- emu_progress_r.py (worker chain re-pointed at MY split/obj/configs): Halo complete units 363->364, complete_code +4,865, complete_data +876, matched_data +136 (data credit without allow_incomplete_unit), ownership snapshot king .bss 3 symbols validates, revoked [], parks 93/0/0, admission 15/0/7/0 0 contradicted. Reproduces worker numbers.
- symbols.json patched copy: 23 lines changed in place, +368 bytes = 23 x len(', "static": true'), same CRLF line count; _king_engine_hill_count/_hills and 5 helpers were already static.
- UNDEF scan (own): 0 refs to any of the 23 names in build/split (833) or build/base (621).
- Header consumers (git grep): game_engine_king.h included only by game_engine_king.c; game_engine_list.c has a local `extern struct game_engine king_engine;` (unchanged, king_engine stays external).
- Patched files keep CRLF throughout (rec .c 1152/1152, .h 81/81; alt .c 1179/1179, .h 54/54).
## R6 owner-gated class check: find_next_hill uninitialised return (NOT raised by the worker)
- January _find_next_hill @0x52: `mov eax,[ebp-4]` then ret; [ebp-4] is the `push ecx` slot, never written -> the no-candidate path returns an uninitialised local. Authentic January uninitialised read.
- /Od 0x5bd630 (later first-party): RTC flag [ebp-0x15] set on assignment, then `_RTC_UninitUse("next_hill")` (string 0x5bd6f5) before `mov eax,[ebp-0xc]` -> double-attested; original local name is `next_hill` (ours: next_hill_id).
- Production source keeps it via uninitialised `long next_hill_id;` + comment (no `BUG:` tag). Build-wide C4700 census (b1/b2 ninja logs): only king next_hill_id and NonMatching rasterizer_xbox_environment_fog -> no Matching object currently carries a C4700 uninitialised read.
- Brief sec2: "Reproducing an authentic January bug needs target+layout proof, a BUG comment and owner approval - report, don't land"; sec7 owner-gated: "reproducing authentic January uninitialised reads". Lane precedent: dead_camera object held (FUNCTIONS_EXACT_OBJECT_BLOCKED) on the same class. No owner ruling for find_next_hill found in docs (only the 20260902 Fable worker rationale).
- Next: strip-test whether a DEFINED result is byte-inert (if so the gate disappears).
## R7 strip tests
- find_next_hill defined-result variants (rec base): s1 `= NONE`, s2 `= 0`, s3 `= hill_id`, s4 `return NONE;`, s5 `return hill_id;` -> ALL 28/29, _find_next_hill residual. The uninitialised local is LOAD-BEARING (owner-gated class, brief s2/s7).
- s6 king_engine_player_update unused `unit` local -> bare `unit_get(...)` call: 29/29 (inert, not a fake lever; pre-existing).
- s7 drop discarded `global_scenario_get();` in post_rasterize: residual -> the call is real January code (relocation), fine.
- The 23 `static` tokens: the plain (non-static) production spelling is ALSO 29/29 -> static is not a byte lever; it is required solely by cachebeta publics (storage truth). Not fake.
- Owner packet variant (strip/owner_bug_comment.c = c2 + original-bug-policy `BUG (preserved for exact matching)` comment + /Od local name `next_hill`): 29/29 vs build/split and emulated split; object_audit_r PASS. Patch: patches/OWNER_GATED_king_source_bugcomment.patch (git apply --check OK; round-trip cmp OK). NOT to land without owner ruling.
- fake_match_scan on rec/alt .c+.h: 0 leads. Source diff (rec vs production .c) = exactly 23 `static` insertions.
## Verdict
- Storage/ownership correction (king_source.patch=c2 + symbols_king.patch [+ optional symbol_ownership_king_OPTIONAL.patch]): VERIFIED, admissible now as a zero-credit ownership correction.
- Whole-object completion (config_status_king.patch + semantic_data_matches_king.patch): REJECT/HOLD - object admission certifies find_next_hill's load-bearing reproduced uninitialised read, which the brief makes owner-gated; no owner ruling on record; existing comment is not in original-bug-policy form; worker did not disclose it. Reopen = owner ruling admitting the preserved uninit result (then land OWNER_GATED patch + status + data entry).
